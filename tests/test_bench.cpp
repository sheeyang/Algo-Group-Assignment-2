#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bench.h"
#include "doctest.h"

using namespace seam;

static Image random_image(int w, int h, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> val(0, 255);
    Image img(w, h);
    for (auto& b : img.rgb) b = static_cast<uint8_t>(val(rng));
    return img;
}

static bool starts_with(const std::string& s, const std::string& prefix) {
    return s.compare(0, prefix.size(), prefix) == 0;
}

// ---------------------------------------------------------------------------------------------
// Algorithm names
// ---------------------------------------------------------------------------------------------

TEST_CASE("algorithm_name and parse_algorithm are inverses") {
    for (Algorithm a : {Algorithm::DP, Algorithm::Greedy, Algorithm::GraphCutEK, Algorithm::GraphCutDinic})
        CHECK(parse_algorithm(algorithm_name(a)) == a);
    CHECK(std::string(algorithm_name(Algorithm::DP)) == "dp");
    CHECK(std::string(algorithm_name(Algorithm::Greedy)) == "greedy");
    CHECK_THROWS_AS(parse_algorithm("DP"), std::invalid_argument);  // names are lower case
    CHECK_THROWS_AS(parse_algorithm(""), std::invalid_argument);
}

// ---------------------------------------------------------------------------------------------
// scale_image
// ---------------------------------------------------------------------------------------------

TEST_CASE("scale_image produces the rounded target size") {
    Image img = random_image(40, 30, 1);
    Image half = scale_image(img, 0.5);
    CHECK(half.width == 20);
    CHECK(half.height == 15);
    Image third = scale_image(img, 1.0 / 3);
    CHECK(third.width == 13);  // round(13.33)
    CHECK(third.height == 10);
    Image tiny = scale_image(img, 0.001);
    CHECK(tiny.width == 1);  // never below 1x1
    CHECK(tiny.height == 1);
}

TEST_CASE("scale_image at scale 1 returns the same pixels") {
    Image img = random_image(7, 5, 2);
    CHECK(scale_image(img, 1.0).rgb == img.rgb);
}

TEST_CASE("scale_image averages the pixels each output pixel covers") {
    Image img(2, 2);
    const uint8_t v[4] = {0, 10, 20, 31};
    for (int i = 0; i < 4; ++i)
        for (int c = 0; c < 3; ++c) img.px(i % 2, i / 2)[c] = v[i];
    Image one = scale_image(img, 0.5);
    REQUIRE(one.width == 1);
    REQUIRE(one.height == 1);
    CHECK(int(one.px(0, 0)[0]) == 15);  // (0 + 10 + 20 + 31) / 4 = 15.25, rounded
}

TEST_CASE("scale_image rejects bad input") {
    Image img = random_image(4, 4, 3);
    CHECK_THROWS_AS(scale_image(img, 0.0), std::invalid_argument);
    CHECK_THROWS_AS(scale_image(img, -0.5), std::invalid_argument);
    CHECK_THROWS_AS(scale_image(img, 1.5), std::invalid_argument);
    CHECK_THROWS_AS(scale_image(Image{}, 0.5), std::invalid_argument);
}

// ---------------------------------------------------------------------------------------------
// run_benchmark
// ---------------------------------------------------------------------------------------------

TEST_CASE("run_benchmark gives one row per algorithm and scale, smallest scale first") {
    Image img = random_image(30, 20, 4);
    auto rows = run_benchmark(img, {Algorithm::DP, Algorithm::Greedy}, {1.0, 0.5}, 5, 3);
    REQUIRE(rows.size() == 4);
    CHECK(rows[0].algorithm == "dp");
    CHECK(rows[1].algorithm == "greedy");
    CHECK(rows[0].width == 15);
    CHECK(rows[0].height == 10);
    CHECK(rows[2].width == 30);
    CHECK(rows[2].height == 20);
    for (const BenchRow& r : rows) {
        CHECK(r.seams == 5);
        CHECK(r.ms >= r.find_ms);
        CHECK(r.find_ms >= 0);
        CHECK(r.total_cost > 0);
    }
}

TEST_CASE("run_benchmark total_cost equals the sum of the removed seams' costs") {
    // Recompute the DP run by hand: energy map, DP seam, add its cost, remove, repeat.
    Image img = random_image(25, 18, 5);
    const int seams = 6;
    double expected = 0;
    Image cur = img;
    for (int k = 0; k < seams; ++k) {
        EnergyMap e = sobel_energy(cur);
        Seam s = find_seam_dp(e);
        expected += seam_cost(e, s);
        cur = remove_vertical_seam(cur, s);
    }
    auto rows = run_benchmark(img, {Algorithm::DP}, {1.0}, seams, 1);
    REQUIRE(rows.size() == 1);
    CHECK(rows[0].total_cost == doctest::Approx(expected));
}

TEST_CASE("run_benchmark: on the first seam greedy never beats DP, and graph cut ties it") {
    // With one seam every algorithm sees the same energy map, so the costs are directly comparable.
    for (unsigned seed = 10; seed < 20; ++seed) {
        Image img = random_image(12, 10, seed);
        auto rows = run_benchmark(img, {Algorithm::DP, Algorithm::Greedy, Algorithm::GraphCutEK}, {1.0}, 1, 1);
        REQUIRE(rows.size() == 3);
        const double dp = rows[0].total_cost, greedy = rows[1].total_cost, ek = rows[2].total_cost;
        CHECK(greedy >= dp);
        // Graph cut rounds capacities to 1/1000 (seam.h), so allow that much per row.
        CHECK(ek == doctest::Approx(dp).epsilon(1e-3));
    }
}

TEST_CASE("run_benchmark caps the seam count at width - 1") {
    auto rows = run_benchmark(random_image(4, 3, 6), {Algorithm::DP}, {1.0}, 10, 1);
    REQUIRE(rows.size() == 1);
    CHECK(rows[0].seams == 3);
}

TEST_CASE("run_benchmark skips a size where no seam can be removed") {
    std::vector<std::string> skipped;
    auto rows = run_benchmark(random_image(3, 3, 7), {Algorithm::DP}, {0.2}, 2, 1, 0, &skipped);
    CHECK(rows.empty());
    REQUIRE(skipped.size() == 1);
    CHECK(skipped[0].find("1x1") != std::string::npos);
}

TEST_CASE("run_benchmark reports every algorithm as a row or a skip, never crashes") {
    // Algorithms still under construction throw std::logic_error; they must be skipped, not fatal.
    // Written so it stays true when they are implemented.
    const std::vector<Algorithm> all = {Algorithm::DP, Algorithm::Greedy, Algorithm::GraphCutEK,
                                        Algorithm::GraphCutDinic};
    std::vector<std::string> skipped;
    auto rows = run_benchmark(random_image(16, 12, 8), all, {0.5, 1.0}, 2, 1, 0, &skipped);
    for (Algorithm a : all) {
        const std::string name = algorithm_name(a);
        int n = 0;
        for (const BenchRow& r : rows) n += r.algorithm == name;
        bool noted = false;
        for (const std::string& s : skipped) noted = noted || starts_with(s, name + " ");
        CHECK_MESSAGE((n == 2 || (n == 0 && noted)), name << ": " << n << " rows");
    }
}

TEST_CASE("run_benchmark: exceeding the time budget skips the larger sizes") {
    std::vector<std::string> skipped;
    // A budget of 1 nanosecond: every run is over it. With one seam the run is still complete, so
    // the smallest size gets a row, and the larger size is skipped.
    auto rows = run_benchmark(random_image(20, 16, 9), {Algorithm::DP}, {1.0, 0.5}, 1, 3, 1e-6, &skipped);
    REQUIRE(rows.size() == 1);
    CHECK(rows[0].width == 10);
    REQUIRE(skipped.size() == 1);
    CHECK(starts_with(skipped[0], "dp skipped at 20x16"));
}

TEST_CASE("run_benchmark: a run over the time budget is stopped early and gets no row") {
    std::vector<std::string> skipped;
    // Over budget after the first of 4 seams: the run stops, so there is no full measurement.
    auto rows = run_benchmark(random_image(20, 16, 9), {Algorithm::DP}, {1.0, 0.5}, 4, 3, 1e-6, &skipped);
    CHECK(rows.empty());
    REQUIRE(skipped.size() == 2);
    CHECK(starts_with(skipped[0], "dp stopped at 10x8"));
    CHECK(skipped[0].find("after 1 of 4 seams") != std::string::npos);
    CHECK(starts_with(skipped[1], "dp skipped at 20x16"));
}

TEST_CASE("run_benchmark rejects bad arguments") {
    Image img = random_image(8, 8, 10);
    CHECK_THROWS_AS(run_benchmark(Image{}, {Algorithm::DP}, {1.0}, 1, 1), std::invalid_argument);
    CHECK_THROWS_AS(run_benchmark(img, {}, {1.0}, 1, 1), std::invalid_argument);
    CHECK_THROWS_AS(run_benchmark(img, {Algorithm::DP}, {}, 1, 1), std::invalid_argument);
    CHECK_THROWS_AS(run_benchmark(img, {Algorithm::DP}, {1.2}, 1, 1), std::invalid_argument);
    CHECK_THROWS_AS(run_benchmark(img, {Algorithm::DP}, {1.0}, 0, 1), std::invalid_argument);
    CHECK_THROWS_AS(run_benchmark(img, {Algorithm::DP}, {1.0}, 1, 0), std::invalid_argument);
}

// ---------------------------------------------------------------------------------------------
// write_csv
// ---------------------------------------------------------------------------------------------

TEST_CASE("write_csv writes a header and one line per row") {
    std::vector<BenchRow> rows = {
        {"dp", 400, 266, 20, 12.5, 3.25, 1234.5},
        {"greedy", 400, 266, 20, 9.0, 0.125, 2000.0},
    };
    std::ostringstream out;
    write_csv(rows, out);
    CHECK(out.str() ==
          "algorithm,width,height,seams,ms,find_ms,total_cost\n"
          "dp,400,266,20,12.500,3.250,1234.500\n"
          "greedy,400,266,20,9.000,0.125,2000.000\n");

    std::ostringstream empty;
    write_csv({}, empty);
    CHECK(empty.str() == "algorithm,width,height,seams,ms,find_ms,total_cost\n");
}
