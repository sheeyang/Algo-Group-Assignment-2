// seamcli: headless seam carving and benchmarks. Run `seamcli --help` for the usage below.
//
// Exit codes: 0 success, 1 runtime error (unreadable image, failed write, algorithm error),
// 2 bad command line.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "bench.h"
#include "resize.h"

namespace {

const char* kUsage =
    "usage:\n"
    "  seamcli <in> <out> <target_w> <target_h> [dp|greedy|ek|dinic]\n"
    "  seamcli <in> <out> [--algo A] [--width W] [--height H]\n"
    "  seamcli <in> <out> [--algo A] --percent P\n"
    "  seamcli --bench <image> [--out results.csv] [--algos dp,greedy,ek,dinic]\n"
    "          [--scales 0.125,0.25,0.5,1] [--seams 20] [--repeats 3] [--budget-ms 10000]\n"
    "  seamcli --help\n"
    "\n"
    "resize options:\n"
    "  --algo A         seam finder: dp (default), greedy, ek (graph cut, Edmonds-Karp), dinic\n"
    "  --width W        target width in pixels, 1..current (default: unchanged)\n"
    "  --height H       target height in pixels, 1..current (default: unchanged)\n"
    "  --percent P      shrink both sides to P% of the original (0 < P <= 100);\n"
    "                   cannot be combined with --width or --height\n"
    "\n"
    "benchmark options (remove vertical seams from scaled copies of <image>):\n"
    "  --out FILE       write the CSV to FILE (default: standard output)\n"
    "  --algos LIST     comma-separated algorithms (default: dp,greedy,ek,dinic)\n"
    "  --scales LIST    comma-separated scales in (0, 1] (default: 0.125,0.25,0.5,1)\n"
    "  --seams N        vertical seams removed per run (default: 20)\n"
    "  --repeats N      runs per algorithm and size; the median is reported (default: 3)\n"
    "  --budget-ms MS   stop an algorithm once one run takes longer than MS and skip its\n"
    "                   larger sizes; 0 = no limit (default: 10000)\n"
    "\n"
    "CSV columns: algorithm,width,height,seams,ms,find_ms,total_cost\n";

// A command-line mistake: printed with the usage hint, exit code 2.
struct UsageError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Whole-string number parsing: "12x" or "" is an error, not 12 or 0.
int to_int(const std::string& opt, const std::string& s) {
    size_t used = 0;
    int v = 0;
    try {
        v = std::stoi(s, &used);
    } catch (const std::exception&) {
        used = 0;
    }
    if (used == 0 || used != s.size()) throw UsageError(opt + " expects an integer, got '" + s + "'");
    return v;
}

double to_double(const std::string& opt, const std::string& s) {
    size_t used = 0;
    double v = 0;
    try {
        v = std::stod(s, &used);
    } catch (const std::exception&) {
        used = 0;
    }
    if (used == 0 || used != s.size() || !std::isfinite(v))
        throw UsageError(opt + " expects a number, got '" + s + "'");
    return v;
}

std::vector<std::string> split_commas(const std::string& opt, const std::string& s) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, ',')) {
        if (part.empty()) throw UsageError(opt + " has an empty item in '" + s + "'");
        parts.push_back(part);
    }
    if (parts.empty()) throw UsageError(opt + " needs at least one item");
    return parts;
}

seam::Algorithm to_algorithm(const std::string& opt, const std::string& s) {
    try {
        return seam::parse_algorithm(s);
    } catch (const std::invalid_argument&) {
        throw UsageError(opt + ": unknown algorithm '" + s + "' (expected dp, greedy, ek or dinic)");
    }
}

// The parsed command line: positional arguments in order, and each --option with its value.
struct Args {
    std::vector<std::string> positional;
    std::map<std::string, std::string> opts;
    bool has(const std::string& k) const { return opts.count(k) != 0; }
};

Args parse(int argc, char** argv) {
    static const std::vector<std::string> known = {"--algo",  "--width",  "--height", "--percent",
                                                   "--bench", "--out",    "--algos",  "--scales",
                                                   "--seams", "--repeats", "--budget-ms"};
    Args a;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg.size() < 2 || arg.compare(0, 2, "--") != 0) {
            a.positional.push_back(arg);
            continue;
        }
        bool ok = false;
        for (const std::string& k : known) ok = ok || arg == k;
        if (!ok) throw UsageError("unknown option " + arg);
        if (i + 1 >= argc) throw UsageError(arg + " needs a value");
        if (a.has(arg)) throw UsageError(arg + " given twice");
        a.opts[arg] = argv[++i];
    }
    return a;
}

// Rejects options that belong to the other mode, so a typo is not silently ignored.
void reject(const Args& a, const std::vector<std::string>& opts, const std::string& mode) {
    for (const std::string& k : opts)
        if (a.has(k)) throw UsageError(k + " cannot be used " + mode);
}

int run_resize(const Args& a) {
    reject(a, {"--out", "--algos", "--scales", "--seams", "--repeats", "--budget-ms"}, "when resizing");
    const auto& p = a.positional;
    if (p.size() != 2 && p.size() != 4 && p.size() != 5)
        throw UsageError("expected <in> <out>, then options or <target_w> <target_h> [algorithm]");
    const bool sized_by_position = p.size() >= 4;
    if (sized_by_position && (a.has("--width") || a.has("--height") || a.has("--percent")))
        throw UsageError("give the target size either by position or with --width/--height/--percent, not both");
    if (p.size() == 5 && a.has("--algo")) throw UsageError("give the algorithm either by position or with --algo, not both");
    if (a.has("--percent") && (a.has("--width") || a.has("--height")))
        throw UsageError("--percent cannot be combined with --width or --height");

    seam::Algorithm algo = seam::Algorithm::DP;
    if (p.size() == 5) algo = to_algorithm("algorithm", p[4]);
    if (a.has("--algo")) algo = to_algorithm("--algo", a.opts.at("--algo"));

    // Parse every number before loading the image, so command-line mistakes are reported first.
    int w = -1, h = -1;  // -1: keep the current size
    double percent = -1;
    if (sized_by_position) {
        w = to_int("target_w", p[2]);
        h = to_int("target_h", p[3]);
    }
    if (a.has("--width")) w = to_int("--width", a.opts.at("--width"));
    if (a.has("--height")) h = to_int("--height", a.opts.at("--height"));
    if (a.has("--percent")) {
        percent = to_double("--percent", a.opts.at("--percent"));
        if (!(percent > 0 && percent <= 100)) throw UsageError("--percent must be in (0, 100]");
    }

    const seam::Image in = seam::load_image(p[0]);
    if (percent > 0) {
        w = std::max(1, int(std::lround(in.width * percent / 100)));
        h = std::max(1, int(std::lround(in.height * percent / 100)));
    }
    if (w == -1) w = in.width;
    if (h == -1) h = in.height;
    if (w < 1 || h < 1 || w > in.width || h > in.height)
        throw UsageError("target " + std::to_string(w) + "x" + std::to_string(h) + " must be between 1x1 and " +
                         std::to_string(in.width) + "x" + std::to_string(in.height) + " (seam carving only shrinks)");

    const auto t0 = std::chrono::steady_clock::now();
    const seam::Image out = seam::resize(in, w, h, algo);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    seam::save_image(out, p[1]);
    std::cout << in.width << "x" << in.height << " -> " << out.width << "x" << out.height << " ("
              << seam::algorithm_name(algo) << ", " << std::fixed << std::setprecision(1) << ms << " ms)\n";
    return 0;
}

int run_bench(const Args& a) {
    reject(a, {"--algo", "--width", "--height", "--percent"}, "with --bench");
    if (!a.positional.empty()) throw UsageError("--bench takes no other positional arguments");

    std::vector<seam::Algorithm> algos = {seam::Algorithm::DP, seam::Algorithm::Greedy, seam::Algorithm::GraphCutEK,
                                          seam::Algorithm::GraphCutDinic};
    if (a.has("--algos")) {
        algos.clear();
        for (const std::string& s : split_commas("--algos", a.opts.at("--algos")))
            algos.push_back(to_algorithm("--algos", s));
    }
    std::vector<double> scales = {0.125, 0.25, 0.5, 1.0};
    if (a.has("--scales")) {
        scales.clear();
        for (const std::string& s : split_commas("--scales", a.opts.at("--scales"))) {
            const double v = to_double("--scales", s);
            if (!(v > 0 && v <= 1)) throw UsageError("--scales: every scale must be in (0, 1], got " + s);
            scales.push_back(v);
        }
    }
    const int seams = a.has("--seams") ? to_int("--seams", a.opts.at("--seams")) : 20;
    const int repeats = a.has("--repeats") ? to_int("--repeats", a.opts.at("--repeats")) : 3;
    const double budget = a.has("--budget-ms") ? to_double("--budget-ms", a.opts.at("--budget-ms")) : 10000;
    if (seams < 1) throw UsageError("--seams must be at least 1");
    if (repeats < 1) throw UsageError("--repeats must be at least 1");
    if (budget < 0) throw UsageError("--budget-ms must be 0 (no limit) or more");

    const seam::Image img = seam::load_image(a.opts.at("--bench"));
    std::cerr << "benchmarking " << a.opts.at("--bench") << " (" << img.width << "x" << img.height << "), " << seams
              << " seams, " << repeats << " repeats...\n";
    std::vector<std::string> skipped;
    const auto rows = seam::run_benchmark(img, algos, scales, seams, repeats, budget, &skipped);
    for (const std::string& s : skipped) std::cerr << "note: " << s << "\n";

    if (a.has("--out")) {
        std::ofstream f(a.opts.at("--out"));
        if (!f) throw std::runtime_error("cannot write " + a.opts.at("--out"));
        seam::write_csv(rows, f);
        if (!f) throw std::runtime_error("failed writing " + a.opts.at("--out"));
        std::cerr << rows.size() << " rows written to " << a.opts.at("--out") << "\n";
    } else {
        seam::write_csv(rows, std::cout);
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << kUsage;
            return 0;
        }
    }
    try {
        const Args a = parse(argc, argv);
        return a.has("--bench") ? run_bench(a) : run_resize(a);
    } catch (const UsageError& ex) {
        std::cerr << "error: " << ex.what() << "\n(run seamcli --help for usage)\n";
        return 2;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }
}
