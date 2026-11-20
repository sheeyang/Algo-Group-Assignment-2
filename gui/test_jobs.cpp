/**
 * @brief Headless regression tests for GUI worker results, cancellation and lifetime.
 * @file test_jobs.cpp
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "jobs.h"

#include <sstream>
#include <thread>

namespace {
/**
 * @brief Create a deterministic RGB fixture for GUI-worker tests.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @return Owned image with the requested dimensions.
 */
seam::Image fixture(int width = 13, int height = 9) {
    seam::Image image(width, height);
    for (size_t i = 0; i < image.rgb.size(); ++i)
        image.rgb[i] = static_cast<uint8_t>((i * 37 + i / 7) % 256);
    return image;
}

/**
 * @brief Poll a GUI worker with a bounded test deadline.
 * @param jobs Worker controller to poll until its result is ready.
 * @return The consumed JobResult.
 * @throws std::runtime_error If the worker misses the test deadline.
 */
seam::gui::JobResult finish(seam::gui::Jobs& jobs) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    do {
        if (auto result = jobs.take_ready()) return std::move(*result);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (std::chrono::steady_clock::now() < deadline);
    throw std::runtime_error("GUI worker did not finish before the test deadline");
}
}  // namespace

/** @test GUI resize matches seamlib and records mixed-direction seams and cost. */
TEST_CASE("GUI resize matches seamlib and records mixed-direction seams and cost") {
    const auto image = fixture();
    double expected_cost = 0;
    const auto expected = seam::resize(image, 10, 7, seam::Algorithm::DP,
        [&](const seam::Image& before, const seam::Seam& seam, bool vertical) {
            expected_cost += seam::seam_cost(seam::sobel_energy(
                vertical ? before : seam::transpose(before)), seam);
        });
    seam::gui::Jobs jobs;
    jobs.start_resize(image, 10, 7, seam::Algorithm::DP);
    CHECK(jobs.busy());
    CHECK_THROWS_AS(jobs.start_resize(image, 11, 8, seam::Algorithm::Greedy), std::logic_error);
    const auto result = finish(jobs);
    REQUIRE(result.error.empty());
    CHECK_FALSE(result.cancelled);
    CHECK_FALSE(jobs.busy());
    CHECK(result.image.width == 10);
    CHECK(result.image.height == 7);
    CHECK(result.image.rgb == expected.rgb);
    CHECK(result.total_cost == doctest::Approx(expected_cost));
    CHECK(result.history.size() == 5);
    CHECK(jobs.completed() == jobs.total());
    CHECK(seam::replay(image, result.history, 0, 5).rgb == expected.rgb);
    CHECK(result.overview.width == image.width);
    CHECK(result.overview.height == image.height);
    CHECK_FALSE(jobs.take_ready().has_value());
}

/** @test GUI cancellation publishes no partial resize and the worker can be reused. */
TEST_CASE("GUI cancellation publishes no partial resize and the worker can be reused") {
    seam::gui::Jobs jobs;
    jobs.start_resize(fixture(512, 384), 1, 1, seam::Algorithm::DP);
    jobs.cancel();
    const auto cancelled = finish(jobs);
    CHECK(cancelled.cancelled);
    CHECK(cancelled.image.rgb.empty());
    CHECK(cancelled.error.empty());
    jobs.start_resize(fixture(), 13, 9, seam::Algorithm::DP);
    const auto unchanged = finish(jobs);
    CHECK_FALSE(unchanged.cancelled);
    CHECK(unchanged.image.rgb == fixture().rgb);
    CHECK(unchanged.history.empty());
}

/** @test GUI worker returns algorithm and input errors instead of throwing on the UI thread. */
TEST_CASE("GUI worker returns algorithm and input errors instead of throwing on the UI thread") {
    seam::gui::Jobs jobs;
    jobs.start_resize(fixture(), 0, 5, seam::Algorithm::DP);
    CHECK_FALSE(finish(jobs).error.empty());
    jobs.start_resize(fixture(), 12, 9, static_cast<seam::Algorithm>(99));
    CHECK_FALSE(finish(jobs).error.empty());
    jobs.start_resize(fixture(), 12, 9, seam::Algorithm::Greedy);
    CHECK(finish(jobs).error.empty());
}

/** @test GUI benchmark preserves seamlib measurements and exports those same rows. */
TEST_CASE("GUI benchmark preserves seamlib measurements and exports those same rows") {
    const auto image = fixture();
    seam::gui::BenchmarkOptions options;
    options.scales = {0.5, 1};
    options.seams = 2;
    options.repeats = 1;
    options.budget_ms = 0;
    seam::gui::Jobs jobs;
    jobs.start_benchmark(image, options);
    const auto result = finish(jobs);
    REQUIRE(result.error.empty());
    REQUIRE(result.rows.size() == 4);
    const auto reference = seam::run_benchmark(image, options.algorithms, options.scales, 2, 1);
    for (const auto& row : result.rows) {
        bool found = false;
        for (const auto& expected : reference)
            if (row.algorithm == expected.algorithm && row.width == expected.width) {
                found = true;
                CHECK(row.height == expected.height);
                CHECK(row.seams == expected.seams);
                CHECK(row.total_cost == doctest::Approx(expected.total_cost));
            }
        CHECK(found);
    }
    std::ostringstream csv;
    seam::write_csv(result.rows, csv);
    CHECK(csv.str().find("algorithm,width,height,seams,ms,find_ms,total_cost\n") == 0);
    CHECK(jobs.completed() == 2);
    options.algorithms.clear();
    CHECK_THROWS_AS(jobs.start_benchmark(image, options), std::invalid_argument);
    CHECK_FALSE(jobs.busy());
}

/** @test GUI worker destruction cancels and joins an active resize. */
TEST_CASE("GUI worker destruction cancels and joins an active resize") {
    seam::gui::Jobs jobs;
    jobs.start_resize(fixture(512, 384), 1, 1, seam::Algorithm::DP);
}
