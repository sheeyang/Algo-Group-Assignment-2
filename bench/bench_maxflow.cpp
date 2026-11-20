/**
 * @brief Measured DP/EK/Dinic searches with phase counts and cooperative time budgets.
 * @file bench_maxflow.cpp
 */
// C4: reproducible first-seam timings and actual max-flow operation counts.
// Run from the repository root: bench_maxflow <output-directory> [--large]
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "bench.h"

using namespace seam;
using Clock = std::chrono::steady_clock;

namespace {
// This standalone program runs serially. The three-argument adapters let the
// shared network builder call the measured overloads without changing MaxFlowFn.
MaxFlowStats last_stats;
double last_solver_ms = 0;

/**
 * @brief Measure elapsed wall-clock time from a steady-clock timestamp.
 * @param start Steady-clock timestamp marking measurement start.
 * @return Elapsed milliseconds.
 */
double milliseconds(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

/**
 * @brief Run EK while recording its solver-only duration and path counts.
 * @param graph Residual network updated by the measured solver.
 * @param source Source node index.
 * @param sink Sink node index.
 * @return Additional maximum flow.
 * @note Updates file-local measurement state; benchmark execution is serial.
 */
long long measured_ek(FlowGraph& graph, int source, int sink) {
    const auto start = Clock::now();
    const auto flow = edmonds_karp(graph, source, sink, &last_stats);
    last_solver_ms = milliseconds(start);
    return flow;
}

/**
 * @brief Run Dinic while recording its solver-only duration and phase/path counts.
 * @param graph Residual network updated by the measured solver.
 * @param source Source node index.
 * @param sink Sink node index.
 * @return Additional maximum flow.
 * @note Updates file-local measurement state; benchmark execution is serial.
 */
long long measured_dinic(FlowGraph& graph, int source, int sink) {
    const auto start = Clock::now();
    const auto flow = dinic(graph, source, sink, &last_stats);
    last_solver_ms = milliseconds(start);
    return flow;
}

/** @brief One validated search's total/solver milliseconds, cost and counters. */
struct Measurement {
    double ms, solver_ms, cost;
    MaxFlowStats stats;
};

/**
 * @brief Time one seam search and validate its cost against a DP reference.
 * @param energy Energy map defining seam costs and dimensions.
 * @param algorithm Seam-finding algorithm.
 * @param reference DP seam cost on the identical input energy map.
 * @return End-to-end finder duration, solver duration, seam cost and counters.
 * @throws std::runtime_error If seam validity or reference-cost agreement fails.
 */
Measurement measure(const EnergyMap& energy, Algorithm algorithm, double reference) {
    last_stats = {};
    last_solver_ms = 0;
    const auto start = Clock::now();
    const Seam seam = algorithm == Algorithm::DP ? find_seam_dp(energy)
        : find_seam_graphcut(energy,
            algorithm == Algorithm::GraphCutEK ? measured_ek : measured_dinic);
    const double elapsed = milliseconds(start);
    if (seam.size() != size_t(energy.height)) throw std::runtime_error("Wrong seam length");
    for (int y = 0; y < energy.height; ++y)
        if (seam[y] < 0 || seam[y] >= energy.width ||
            (y && std::abs(seam[y] - seam[y - 1]) > 1))
            throw std::runtime_error("Invalid seam");
    const double cost = seam_cost(energy, seam);
    if (std::abs(cost - reference) > 1e-6)
        throw std::runtime_error("Max-flow seam cost differs from DP");
    return {elapsed, last_solver_ms, cost, last_stats};
}

/**
 * @brief Write the max-flow benchmark CSV header and set numeric precision.
 * @param out Destination output stream.
 */
void header(std::ostream& out) {
    out << "image,algorithm,width,height,time_ms,solver_ms,phases,augmentations,"
           "seam_cost,dp_reference_cost,cost_difference,repeats,notes\n";
    out << std::fixed << std::setprecision(9);
}

// The budget is cooperative: a running max-flow call must finish. If any run
// exceeds it, stop repeating and skip that algorithm's subsequent larger sizes.
/**
 * @brief Repeat a seam measurement, emit the median run and apply a cooperative budget.
 * @param out Destination output stream.
 * @param image Image label written into measurement output.
 * @param energy Energy map defining seam costs and dimensions.
 * @param algorithm Seam-finding algorithm.
 * @param repeats Number of fresh repeated runs.
 * @param budget_ms Cooperative time limit in milliseconds; nonpositive disables it.
 * @return True if a measured run exceeded the budget and larger sizes should be skipped.
 * @note Timings exclude energy calculation. A running solver is not interrupted.
 */
bool benchmark(std::ostream& out, const std::string& image, const EnergyMap& energy,
               Algorithm algorithm, int repeats, double budget_ms) {
    const double reference = seam_cost(energy, find_seam_dp(energy));
    std::vector<Measurement> runs;
    bool over_budget = false;
    for (int i = 0; i < repeats; ++i) {
        runs.push_back(measure(energy, algorithm, reference));
        if (budget_ms > 0 && runs.back().ms > budget_ms) {
            over_budget = true;
            break;
        }
    }
    std::sort(runs.begin(), runs.end(), [](const Measurement& a, const Measurement& b) {
        return a.ms < b.ms;
    });
    const auto& median = runs[(runs.size() - 1) / 2];
    out << image << ',' << algorithm_name(algorithm) << ',' << energy.width << ',' << energy.height
        << ',' << median.ms << ',' << median.solver_ms << ',' << median.stats.phases
        << ',' << median.stats.augmentations << ',' << median.cost << ',' << reference
        << ',' << median.cost - reference << ',' << runs.size() << ','
        << (over_budget ? "measured; exceeded budget; larger sizes skipped" : "measured") << '\n';
    out.flush();
    std::cout << algorithm_name(algorithm) << ' ' << energy.width << 'x' << energy.height
              << ": " << median.ms << " ms; phases=" << median.stats.phases
              << "; augmentations=" << median.stats.augmentations << std::endl;
    return over_budget;
}

/**
 * @brief Nearest-sample the landscape into a 1000x700 acceptance input.
 * @param source Source node index.
 * @return Owned RGB test image.
 * @pre source is nonempty with consistent RGB storage.
 */
Image large_image(const Image& source) {
    Image out(1000, 700);
    for (int y = 0; y < out.height; ++y)
        for (int x = 0; x < out.width; ++x)
            for (int c = 0; c < 3; ++c)
                out.px(x, y)[c] = source.px(x * source.width / out.width,
                                           y * source.height / out.height)[c];
    return out;
}
}  // namespace

/**
 * @brief Measured DP/EK/Dinic searches with phase counts and cooperative time budgets.
 * @param argc Argument count, including the executable name.
 * @param argv Null-terminated command-line argument strings.
 * @return Zero on normal completion; program-specific validation errors are reported or propagate.
 * @note Run benchmark/evidence tools from the repository root.
 */
int main(int argc, char** argv) {
    try {
#ifndef NDEBUG
        throw std::runtime_error("Use a Release build for evidence measurements");
#endif
        if (argc > 3 || (argc == 3 && std::string(argv[2]) != "--large"))
            throw std::invalid_argument("usage: bench_maxflow <output-directory> [--large]");
        const std::filesystem::path output = argc > 1 ? argv[1] : "results";
        std::filesystem::create_directories(output);
        const Image source = load_image("testdata/landscape-tower.png");
        std::ofstream csv(output / "maxflow_benchmark.csv");
        csv.exceptions(std::ios::failbit | std::ios::badbit);
        header(csv);
        const std::vector<Algorithm> algorithms = {
            Algorithm::DP, Algorithm::GraphCutDinic, Algorithm::GraphCutEK};
        bool stopped[3] = {false, false, false};
        for (double scale : {0.04, 0.08, 0.12, 0.16, 0.25, 0.5, 1.0}) {
            const Image image = scale_image(source, scale);
            const EnergyMap energy = sobel_energy(image);
            for (size_t a = 0; a < algorithms.size(); ++a) {
                if (stopped[a]) {
                    csv << "landscape-tower," << algorithm_name(algorithms[a]) << ','
                        << image.width << ',' << image.height
                        << ",,,,,,,,,skipped; smaller size exceeded 5000 ms\n";
                    continue;
                }
                stopped[a] = benchmark(csv, "landscape-tower", energy, algorithms[a], 3, 5000);
            }
        }
        if (argc == 3) {
            std::ofstream large(output / "maxflow_large.csv");
            large.exceptions(std::ios::failbit | std::ios::badbit);
            header(large);
            const EnergyMap energy = sobel_energy(large_image(source));
            for (Algorithm algorithm : {Algorithm::DP, Algorithm::GraphCutDinic})
                benchmark(large, "landscape-tower-nearest-1000x700", energy, algorithm, 1, 0);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
