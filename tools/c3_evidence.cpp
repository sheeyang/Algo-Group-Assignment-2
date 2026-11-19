/**
 * @file c3_evidence.cpp
 * @brief Generate reproducible C3 report figures and numerical evidence.
 * @details Uses seamlib's real finders and visualisation code on the portrait
 * and building fixtures. Run from the repository root using the companion
 * generate-c3-evidence.ps1 script; generated evidence replaces previous files.
 */
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "bench.h"
#include "visualize.h"

namespace fs = std::filesystem;
using namespace seam;

/**
 * @brief Check a seam before accepting it as report evidence.
 * @param energy Map defining the required height and valid column range.
 * @param s Candidate vertical seam.
 * @throws std::runtime_error For an invalid length, column or disconnected step.
 * @par Complexity
 * O(H) time and O(1) additional storage.
 */
static void validate(const EnergyMap& energy, const Seam& s) {
    if (s.size() != size_t(energy.height)) throw std::runtime_error("Wrong seam length");
    for (int y = 0; y < energy.height; ++y)
        if (s[y] < 0 || s[y] >= energy.width ||
            (y && std::abs(s[y] - s[y - 1]) > 1))
            throw std::runtime_error("Invalid seam");
}

/**
 * @brief Export two DP/EK comparisons and five-repeat median benchmarks.
 * @return Zero on normal completion (implicit return from main).
 * @pre Working directory is the repository root and both small PNGs are 64x43.
 * @throws std::runtime_error If input dimensions, seam validity, same-image
 * cost agreement (absolute tolerance 1e-6), or output dimensions fail checks.
 * @note Image/resize exceptions also propagate. Writes PNGs under
 * docs/report/figures/c3 and CSVs under results. Benchmark timings exclude
 * figure generation and compare fresh ten-seam runs, not one-seam searches.
 */
int main() {
    const fs::path figures = "docs/report/figures/c3";
    fs::create_directories(figures);
    fs::create_directories("results");
    std::ofstream checks("results/c3-seam-checks.csv");
    checks << "image,step,width,height,ek_cost,dp_reference_cost,difference\n";
    checks << std::setprecision(15);
    std::ofstream summary("results/c3-first-seams.csv");
    summary << "image,width,height,dp_cost,ek_cost,positions_equal,final_pixels_equal\n";
    summary << std::setprecision(15);

    for (const std::string name : {"portrait", "building"}) {
        const Image input = load_image("testdata/c3/" + name + "-small.png");
        if (input.width != 64 || input.height != 43)
            throw std::runtime_error("Expected a 64x43 input");
        const auto energy = sobel_energy(input);
        const Seam dp_first = find_seam_dp(energy), ek_first = find_seam_graphcut_ek(energy);
        validate(energy, dp_first);
        validate(energy, ek_first);
        if (std::abs(seam_cost(energy, dp_first) - seam_cost(energy, ek_first)) > 1e-6)
            throw std::runtime_error("First-seam cost mismatch");
        auto save = [&](const Image& img, const std::string& suffix) {
            save_image(img, (figures / (name + "-" + suffix + ".png")).string());
        };
        save(input, "input");
        save(draw_seam(input, dp_first, true), "dp-first");
        save(draw_seam(input, ek_first, true), "ek-first");

        SeamTracker dp_tracker(64, 43), ek_tracker(64, 43);
        const Image dp = resize(input, 54, 43, Algorithm::DP,
            [&](const Image&, const Seam& s, bool vertical) { dp_tracker.record(s, vertical); });
        int step = 0;
        const Image ek = resize(input, 54, 43, Algorithm::GraphCutEK,
            [&](const Image& before, const Seam& s, bool vertical) {
                const auto current_energy = sobel_energy(before);
                validate(current_energy, s);
                const double actual = seam_cost(current_energy, s);
                const double reference = seam_cost(current_energy, find_seam_dp(current_energy));
                if (std::abs(actual - reference) > 1e-6)
                    throw std::runtime_error("EK/DP cost mismatch on EK's current image");
                checks << name << ',' << ++step << ',' << before.width << ',' << before.height
                       << ',' << actual << ',' << reference << ',' << actual-reference << '\n';
                ek_tracker.record(s, vertical);
            });
        if (dp.width != 54 || ek.width != 54 || dp.height != 43 || ek.height != 43 || step != 10)
            throw std::runtime_error("Wrong resize dimensions or step count");
        save(dp, "dp-result");
        save(ek, "ek-result");
        save(dp_tracker.overview(input), "dp-overview");
        save(ek_tracker.overview(input), "ek-overview");
        summary << name << ",64,43," << seam_cost(energy, dp_first) << ','
                << seam_cost(energy, ek_first) << ',' << (dp_first == ek_first)
                << ',' << (dp.rgb == ek.rgb) << '\n';

        // Median of five fresh, independent ten-seam runs, with no image I/O
        // or figure rendering inside the timed region.
        std::ofstream bench("results/c3-benchmark-" + name + ".csv");
        write_csv(run_benchmark(input, {Algorithm::DP, Algorithm::GraphCutEK},
                                {1.0}, 10, 5), bench);
        std::cout << name << ": 10 EK seams validated against DP; evidence saved.\n";
    }
}
