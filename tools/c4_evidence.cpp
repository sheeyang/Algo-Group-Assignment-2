/**
 * @brief Validated DP/Dinic report images and repeatable benchmark evidence.
 * @file c4_evidence.cpp
 */
// C4 report evidence from the actual seamlib algorithms, with per-step checks.
// Build the c4_evidence CMake target and run from the repository root.
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "bench.h"
#include "visualize.h"

using namespace seam;

/**
 * @brief Validate a report seam and sum its original energy.
 * @param energy Energy map defining seam costs and dimensions.
 * @param seam Candidate vertical seam in the energy map coordinates.
 * @return Seam cost after validity checks.
 * @throws std::runtime_error For invalid length, columns or connectivity.
 */
static double checked_cost(const EnergyMap& energy, const Seam& seam) {
    if (seam.size() != size_t(energy.height)) throw std::runtime_error("Wrong seam length");
    for (int y = 0; y < energy.height; ++y)
        if (seam[y] < 0 || seam[y] >= energy.width ||
            (y && std::abs(seam[y] - seam[y - 1]) > 1))
            throw std::runtime_error("Invalid seam");
    return seam_cost(energy, seam);
}

/**
 * @brief Validated DP/Dinic report images and repeatable benchmark evidence.
 * @return Zero on normal completion; program-specific validation errors are reported or propagate.
 * @note Run benchmark/evidence tools from the repository root.
 */
int main() {
    try {
#ifndef NDEBUG
        throw std::runtime_error("Use a Release build for evidence measurements");
#endif
        const std::filesystem::path figures = "docs/report/figures/c4";
        std::filesystem::create_directories(figures);
        std::filesystem::create_directories("results");
        std::ofstream checks("results/c4-seam-checks.csv");
        std::ofstream summary("results/c4-first-seams.csv");
        checks.exceptions(std::ios::failbit | std::ios::badbit);
        summary.exceptions(std::ios::failbit | std::ios::badbit);
        checks << "image,step,width,height,dinic_cost,dp_reference_cost,difference\n"
               << std::setprecision(15);
        summary << "image,width,height,dp_cost,dinic_cost,positions_equal,final_pixels_equal\n"
                << std::setprecision(15);
        for (const std::string name : {"portrait", "building"}) {
            const Image input = load_image("testdata/c3/" + name + "-small.png");
            const EnergyMap energy = sobel_energy(input);
            const Seam dp_first = find_seam_dp(energy), dinic_first = find_seam_graphcut_dinic(energy);
            const double dp_cost = checked_cost(energy, dp_first);
            const double dinic_cost = checked_cost(energy, dinic_first);
            if (std::abs(dp_cost - dinic_cost) > 1e-6)
                throw std::runtime_error("First-seam cost mismatch");
            const auto save = [&](const Image& image, const std::string& suffix) {
                save_image(image, (figures / (name + "-" + suffix + ".png")).string());
            };
            save(input, "input");
            save(draw_seam(input, dp_first, true), "dp-first");
            save(draw_seam(input, dinic_first, true), "dinic-first");
            SeamTracker dp_tracker(input.width, input.height), dinic_tracker(input.width, input.height);
            const Image dp = resize(input, input.width - 10, input.height, Algorithm::DP,
                [&](const Image&, const Seam& seam, bool vertical) { dp_tracker.record(seam, vertical); });
            int step = 0;
            const Image dinic_image = resize(input, input.width - 10, input.height, Algorithm::GraphCutDinic,
                [&](const Image& before, const Seam& seam, bool vertical) {
                    const EnergyMap current = sobel_energy(before);
                    const double actual = checked_cost(current, seam);
                    const double expected = seam_cost(current, find_seam_dp(current));
                    if (std::abs(actual - expected) > 1e-6)
                        throw std::runtime_error("Dinic/DP cost mismatch on the current image");
                    checks << name << ',' << ++step << ',' << before.width << ',' << before.height
                           << ',' << actual << ',' << expected << ',' << actual - expected << '\n';
                    dinic_tracker.record(seam, vertical);
                });
            if (step != 10 || dp.width != input.width - 10 || dinic_image.width != dp.width ||
                dp.height != input.height || dinic_image.height != input.height)
                throw std::runtime_error("Wrong output size or number of seams");
            save(dp, "dp-result");
            save(dinic_image, "dinic-result");
            save(dp_tracker.overview(input), "dp-overview");
            save(dinic_tracker.overview(input), "dinic-overview");
            summary << name << ',' << input.width << ',' << input.height << ',' << dp_cost
                    << ',' << dinic_cost << ',' << (dp_first == dinic_first)
                    << ',' << (dp.rgb == dinic_image.rgb) << '\n';
            std::ofstream timing("results/c4-benchmark-" + name + ".csv");
            timing.exceptions(std::ios::failbit | std::ios::badbit);
            write_csv(run_benchmark(input, {Algorithm::DP, Algorithm::GraphCutDinic}, {1.0}, 10, 3), timing);
            std::cout << name << ": ten Dinic seams checked against DP; figures saved.\n";
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
