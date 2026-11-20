/**
 * @brief Budget-aware repeated seam benchmarks and consistent median selection.
 * @file bench.cpp
 */
#include "bench.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>

namespace seam {

/**
 * @brief Convert an algorithm enum to its stable CLI/CSV name.
 * @param a Seam-finding algorithm to dispatch.
 * @return Static name string, such as dp, greedy, ek or dinic.
 * @throws std::invalid_argument For an unknown enum value.
 */
const char* algorithm_name(Algorithm a) {
    switch (a) {
        case Algorithm::DP: return "dp";
        case Algorithm::Greedy: return "greedy";
        case Algorithm::GraphCutEK: return "ek";
        case Algorithm::GraphCutDinic: return "dinic";
    }
    throw std::invalid_argument("algorithm_name: unknown algorithm");
}

/**
 * @brief Parse a case-sensitive CLI algorithm name.
 * @param name Case-sensitive algorithm name.
 * @return Matching algorithm enum.
 * @throws std::invalid_argument For an unknown name.
 */
Algorithm parse_algorithm(const std::string& name) {
    for (Algorithm a : {Algorithm::DP, Algorithm::Greedy, Algorithm::GraphCutEK, Algorithm::GraphCutDinic})
        if (name == algorithm_name(a)) return a;
    throw std::invalid_argument("unknown algorithm: " + name + " (expected dp, greedy, ek or dinic)");
}

/**
 * @brief Box-average source blocks into a smaller benchmark input.
 * @param img RGB image with storage matching its dimensions.
 * @param scale Ordinary image scale in (0,1].
 * @return Owned image with rounded scaled dimensions, each at least one pixel.
 * @throws std::invalid_argument For an empty image or scale outside (0,1].
 * @pre RGB storage matches dimensions.
 * @note This is ordinary scaling, not seam carving; the input is unchanged.
 */
Image scale_image(const Image& img, double scale) {
    if (img.width < 1 || img.height < 1) throw std::invalid_argument("scale_image: image is empty");
    if (!(scale > 0 && scale <= 1)) throw std::invalid_argument("scale_image: scale must be in (0, 1]");
    const int W = img.width, H = img.height;
    const int nw = std::max(1, int(std::lround(scale * W)));
    const int nh = std::max(1, int(std::lround(scale * H)));
    if (nw == W && nh == H) return img;

    // Box filter: output pixel (ox, oy) is the average of the source block it covers, columns
    // [x0, x1) and rows [y0, y1). Every block holds at least one source pixel because nw <= W.
    Image out(nw, nh);
    for (int oy = 0; oy < nh; ++oy) {
        const int y0 = int(static_cast<long long>(oy) * H / nh);
        const int y1 = std::max(y0 + 1, int(static_cast<long long>(oy + 1) * H / nh));
        for (int ox = 0; ox < nw; ++ox) {
            const int x0 = int(static_cast<long long>(ox) * W / nw);
            const int x1 = std::max(x0 + 1, int(static_cast<long long>(ox + 1) * W / nw));
            for (int c = 0; c < 3; ++c) {
                long long sum = 0;
                for (int y = y0; y < y1; ++y)
                    for (int x = x0; x < x1; ++x) sum += img.px(x, y)[c];
                const long long n = static_cast<long long>(x1 - x0) * (y1 - y0);
                out.px(ox, oy)[c] = static_cast<uint8_t>((sum + n / 2) / n);  // rounded mean
            }
        }
    }
    return out;
}

namespace {

using Clock = std::chrono::steady_clock;

/**
 * @brief Measure elapsed wall-clock time from a steady-clock timestamp.
 * @param t0 Steady-clock start timestamp.
 * @return Elapsed milliseconds.
 */
double ms_since(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

/**
 * @brief Dispatch an energy map to the requested seam finder.
 * @param e Input row-major energy map.
 * @param a Seam-finding algorithm to dispatch.
 * @return Selected vertical seam; finder exceptions propagate.
 */
Seam find(const EnergyMap& e, Algorithm a) {
    switch (a) {
        case Algorithm::DP: return find_seam_dp(e);
        case Algorithm::Greedy: return find_seam_greedy(e);
        case Algorithm::GraphCutEK: return find_seam_graphcut_ek(e);
        case Algorithm::GraphCutDinic: return find_seam_graphcut_dinic(e);
    }
    throw std::invalid_argument("unknown algorithm");
}

/** @brief One timed resize attempt, possibly stopped at a seam boundary. */
struct Run {
    double ms = 0, find_ms = 0, cost = 0;
    int done = 0;  // seams removed; less than requested when the run was stopped by the budget
};

// One timed run: remove `seams` vertical seams from `start`. This is the vertical half of the
// resize loop (resize.cpp), written out so the seam-finding time can be measured on its own.
// With budget_ms > 0 the run stops early once it has taken longer than that.
/**
 * @brief Time a fresh sequence of vertical seam removals.
 * @param start Image copied at the start of each timed resize.
 * @param a Seam-finding algorithm to dispatch.
 * @param seams Requested vertical seam removals.
 * @param budget_ms Cooperative time limit in milliseconds; nonpositive disables it.
 * @return Completed seam count, total cost and whole-run/finder milliseconds.
 * @pre start is valid and seams is within 1..width-1.
 * @note A positive budget is checked after each completed seam; it cannot interrupt max-flow.
 */
Run run_once(const Image& start, Algorithm a, int seams, double budget_ms) {
    Run r;
    const Clock::time_point t0 = Clock::now();
    Image cur = start;
    for (int k = 0; k < seams; ++k) {
        EnergyMap e = sobel_energy(cur);
        const Clock::time_point f0 = Clock::now();
        Seam s = find(e, a);
        r.find_ms += ms_since(f0);
        r.cost += seam_cost(e, s);  // O(height); negligible next to the energy map
        cur = remove_vertical_seam(cur, s);
        ++r.done;
        if (budget_ms > 0 && k + 1 < seams && ms_since(t0) > budget_ms) break;
    }
    r.ms = ms_since(t0);
    return r;
}

/**
 * @brief Format image dimensions for benchmark diagnostic messages.
 * @param img RGB image with storage matching its dimensions.
 * @return Width and height joined by x.
 */
std::string size_str(const Image& img) {
    return std::to_string(img.width) + "x" + std::to_string(img.height);
}

}  // namespace

/**
 * @brief Run algorithms at ascending image scales and select median complete runs.
 * @param img RGB image with storage matching its dimensions.
 * @param algos Algorithms to measure.
 * @param scales Requested input scales in (0,1].
 * @param seams Requested vertical seam removals.
 * @param repeats Number of fresh repeated runs.
 * @param budget_ms Cooperative time limit in milliseconds; nonpositive disables it.
 * @param skipped Optional destination for skip/budget diagnostic messages.
 * @return One row per completed algorithm/scale; lower median for even repeat counts.
 * @throws std::invalid_argument For empty inputs/lists, invalid scales or nonpositive seam/repeat
 * counts.
 * @note Time budgets are cooperative at seam boundaries. Over-budget algorithms skip larger sizes.
 * A partially completed run produces a note, not a CSV row. find_ms and cost come from the same
 * selected total-time run.
 */
std::vector<BenchRow> run_benchmark(const Image& img, const std::vector<Algorithm>& algos,
                                    const std::vector<double>& scales, int seams, int repeats,
                                    double budget_ms, std::vector<std::string>* skipped) {
    if (img.width < 1 || img.height < 1) throw std::invalid_argument("run_benchmark: image is empty");
    if (algos.empty()) throw std::invalid_argument("run_benchmark: no algorithms given");
    if (scales.empty()) throw std::invalid_argument("run_benchmark: no scales given");
    for (double s : scales)
        if (!(s > 0 && s <= 1)) throw std::invalid_argument("run_benchmark: every scale must be in (0, 1]");
    if (seams < 1) throw std::invalid_argument("run_benchmark: seams must be at least 1");
    if (repeats < 1) throw std::invalid_argument("run_benchmark: repeats must be at least 1");

    auto note = [&](const std::string& msg) {
        if (skipped) skipped->push_back(msg);
    };

    // Smallest size first, so the time budget can stop an algorithm before its slowest runs.
    std::vector<double> order = scales;
    std::sort(order.begin(), order.end());

    std::set<Algorithm> unavailable;   // threw "not implemented"
    std::set<Algorithm> over_budget;   // a smaller size already exceeded budget_ms
    std::vector<BenchRow> rows;
    for (double scale : order) {
        const Image scaled = scale_image(img, scale);
        const int n = std::min(seams, scaled.width - 1);  // the last column can never be removed
        if (n < 1) {
            note("all algorithms skipped at " + size_str(scaled) + ": width 1, no seam can be removed");
            continue;
        }
        for (Algorithm a : algos) {
            const std::string name = algorithm_name(a);
            if (unavailable.count(a)) continue;  // already reported once
            if (over_budget.count(a)) {
                note(name + " skipped at " + size_str(scaled) + ": a smaller size exceeded the time budget");
                continue;
            }

            std::vector<Run> runs;
            try {
                for (int i = 0; i < repeats; ++i) {
                    runs.push_back(run_once(scaled, a, n, budget_ms));
                    if (budget_ms > 0 && runs.back().ms > budget_ms) {
                        over_budget.insert(a);  // stop repeating; larger sizes are skipped
                        break;
                    }
                }
            } catch (const std::invalid_argument&) {
                throw;  // a real error (bad input), not a missing algorithm
            } catch (const std::out_of_range&) {
                throw;
            } catch (const std::logic_error& ex) {
                // The stubs of unimplemented seam finders throw a plain std::logic_error.
                unavailable.insert(a);
                note(name + " skipped: " + ex.what());
                continue;
            }

            // A run cut short by the budget is not a measurement of `n` seams, so it gets no row.
            // The note keeps its numbers, so the time per seam can still be quoted.
            if (runs.back().done < n) {
                std::ostringstream msg;
                msg << name << " stopped at " << size_str(scaled) << ": over the " << budget_ms
                    << " ms budget after " << runs.back().done << " of " << n << " seams ("
                    << runs.back().ms << " ms)";
                note(msg.str());
                continue;
            }

            // Median by total time; the find time and cost come from that same run, so the row is
            // one consistent measurement. With an even count this is the lower of the two middle runs.
            std::sort(runs.begin(), runs.end(), [](const Run& x, const Run& y) { return x.ms < y.ms; });
            const Run& med = runs[(runs.size() - 1) / 2];
            rows.push_back(BenchRow{name, scaled.width, scaled.height, n, med.ms, med.find_ms, med.cost});
        }
    }
    return rows;
}

/**
 * @brief Serialise benchmark rows with a header and three decimal places.
 * @param rows Completed benchmark rows.
 * @param out Destination output stream.
 * @note Formatting is prepared in a separate stream; caller formatting flags are preserved. Output
 * stream failure state remains for the caller to inspect.
 */
void write_csv(const std::vector<BenchRow>& rows, std::ostream& out) {
    // Format in a separate stream so the caller's stream flags are left alone.
    std::ostringstream s;
    s << std::fixed << std::setprecision(3);
    s << "algorithm,width,height,seams,ms,find_ms,total_cost\n";
    for (const BenchRow& r : rows)
        s << r.algorithm << ',' << r.width << ',' << r.height << ',' << r.seams << ',' << r.ms << ','
          << r.find_ms << ',' << r.total_cost << '\n';
    out << s.str();
}

}  // namespace seam
