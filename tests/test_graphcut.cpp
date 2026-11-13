#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

#include "doctest.h"
#include "seam.h"
#include "resize.h"

using namespace seam;

static void check_against_dp(const EnergyMap& e) {
    const Seam s = find_seam_graphcut_ek(e);
    REQUIRE(s.size() == size_t(e.height));
    for (int y = 0; y < e.height; ++y) {
        REQUIRE(s[y] >= 0);
        REQUIRE(s[y] < e.width);
        if (y > 0) CHECK(std::abs(s[y] - s[y - 1]) <= 1);
    }
    CHECK(seam_cost(e, s) == doctest::Approx(seam_cost(e, find_seam_dp(e))));
}

TEST_CASE("EK graph cut finds the hand-checked seam") {
    EnergyMap e{3, 3, {5, 1, 0, 1, 9, 9, 5, 1, 9}};
    CHECK(find_seam_graphcut_ek(e) == Seam{1, 0, 1});
    check_against_dp(e);
}

TEST_CASE("Virtual column allows the cheapest seam in the last column") {
    EnergyMap e{3, 3, {9, 9, 1, 9, 9, 1, 9, 9, 1}};
    CHECK(find_seam_graphcut_ek(e) == Seam{2, 2, 2});
    check_against_dp(e);
}

TEST_CASE("EK graph cut handles tiny maps, zeros and ties") {
    check_against_dp(EnergyMap{1, 1, {42}});
    check_against_dp(EnergyMap{1, 4, {7, 3, 9, 1}});
    check_against_dp(EnergyMap{4, 1, {9, 8, 7, 0}});
    check_against_dp(EnergyMap{3, 3, std::vector<double>(9, 0)});
    check_against_dp(EnergyMap{4, 4, std::vector<double>(16, 5)});
}

TEST_CASE("EK graph cut agrees with DP on 500 random integer energy maps") {
    std::mt19937 rng(20261113);
    for (int trial = 0; trial < 500; ++trial) {
        const int w = 1 + int(rng() % 7), h = 1 + int(rng() % 7);
        EnergyMap e{w, h, std::vector<double>(size_t(w) * h)};
        for (double& v : e.e) v = double(rng() % (trial % 2 ? 30 : 3));
        INFO("trial=" << trial << " width=" << w << " height=" << h);
        check_against_dp(e);
    }
}

TEST_CASE("EK graph cut preserves thousandth-valued energies") {
    check_against_dp(EnergyMap{3, 3, {1.234, 0.125, 9.001,
                                     0.256, 9.002, 9.003,
                                     9.004, 0.111, 9.005}});
}

TEST_CASE("EK resize removes both vertical and horizontal seams") {
    Image img(6, 5);
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x) {
            auto* pixel = img.px(x, y);
            pixel[0] = static_cast<uint8_t>(x * 30 + y * 7);
            pixel[1] = static_cast<uint8_t>(y * 40);
            pixel[2] = static_cast<uint8_t>(x * 20);
        }
    int vertical = 0, horizontal = 0;
    const Image out = resize(img, 4, 3, Algorithm::GraphCutEK,
        [&](const Image& before, const Seam& s, bool is_vertical) {
            const EnergyMap energy = sobel_energy(is_vertical ? before : transpose(before));
            CHECK(seam_cost(energy, s) ==
                  doctest::Approx(seam_cost(energy, find_seam_dp(energy))));
            if (is_vertical) ++vertical;
            else ++horizontal;
        });
    CHECK(out.width == 4);
    CHECK(out.height == 3);
    CHECK(vertical == 2);
    CHECK(horizontal == 2);
}

TEST_CASE("EK graph cut rejects invalid maps and unsafe capacities") {
    CHECK_THROWS_AS(find_seam_graphcut_ek(EnergyMap{}), std::invalid_argument);
    CHECK_THROWS_AS(find_seam_graphcut_ek(EnergyMap{2, 2, {1}}), std::invalid_argument);
    CHECK_THROWS_AS(find_seam_graphcut_ek(EnergyMap{1, 1, {-1}}), std::invalid_argument);
    CHECK_THROWS_AS(find_seam_graphcut_ek(EnergyMap{1, 1,
                       {std::numeric_limits<double>::infinity()}}), std::invalid_argument);
    CHECK_THROWS_AS(find_seam_graphcut_ek(EnergyMap{1, 1,
                       {std::numeric_limits<double>::quiet_NaN()}}), std::invalid_argument);
    CHECK_THROWS_AS(find_seam_graphcut_ek(EnergyMap{1, 1, {1e30}}), std::overflow_error);
    CHECK_THROWS_AS(find_seam_graphcut_ek(EnergyMap{2, 1, {2e15, 2e15}}), std::overflow_error);
    CHECK_THROWS_AS(find_seam_graphcut(EnergyMap{1, 1, {1}}, nullptr), std::invalid_argument);
}
