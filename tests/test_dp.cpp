#include <algorithm>
#include <cstdlib>
#include <random>
#include <stdexcept>

#include "doctest.h"
#include "seam.h"

using namespace seam;

static EnergyMap make(int w, int h, std::vector<double> v) {
    EnergyMap m;
    m.width = w;
    m.height = h;
    m.e = std::move(v);
    return m;
}

// A seam is valid when it has one column per row, every column is inside the image and
// neighbouring rows differ by at most one column (GLOSSARY.md: Seam).
static bool is_valid_seam(const Seam& s, int w, int h) {
    if (int(s.size()) != h) return false;
    for (int y = 0; y < h; ++y) {
        if (s[y] < 0 || s[y] >= w) return false;
        if (y > 0 && std::abs(s[y] - s[y - 1]) > 1) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------------------------
// Hand-checked cases
// ---------------------------------------------------------------------------------------------

TEST_CASE("DP finds the hand-checked optimal seam") {
    // Column 2 starts at 0 but is a trap (9 next). Unique optimum is columns 1,0,1: 1+1+1 = 3.
    EnergyMap e = make(3, 3, {
        5, 1, 0,
        1, 9, 9,
        5, 1, 9,
    });
    Seam s = find_seam_dp(e);
    CHECK(s == Seam{1, 0, 1});
    CHECK(seam_cost(e, s) == doctest::Approx(3.0));
}

TEST_CASE("DP seams are connected") {
    EnergyMap e = make(4, 4, {
        3, 8, 2, 7,
        6, 1, 9, 4,
        2, 5, 3, 8,
        9, 4, 1, 6,
    });
    Seam s = find_seam_dp(e);
    REQUIRE(s.size() == 4);
    for (size_t y = 1; y < s.size(); ++y) CHECK(std::abs(s[y] - s[y - 1]) <= 1);
}

// ---------------------------------------------------------------------------------------------
// Edge cases: tiny maps
// ---------------------------------------------------------------------------------------------

TEST_CASE("DP on a width-1 map must take the only column") {
    EnergyMap e = make(1, 4, {7, 3, 9, 1});
    CHECK(find_seam_dp(e) == Seam{0, 0, 0, 0});
}

TEST_CASE("DP on a height-1 map picks the cheapest pixel") {
    EnergyMap e = make(5, 1, {4, 2, 9, 2, 7});
    CHECK(find_seam_dp(e) == Seam{1});  // leftmost of the two 2s
}

TEST_CASE("DP on a 1x1 map") {
    EnergyMap e = make(1, 1, {42});
    CHECK(find_seam_dp(e) == Seam{0});
    CHECK(seam_cost(e, Seam{0}) == doctest::Approx(42.0));
}

TEST_CASE("DP rejects an empty or inconsistent energy map") {
    CHECK_THROWS_AS(find_seam_dp(EnergyMap{}), std::invalid_argument);
    CHECK_THROWS_AS(find_seam_dp(make(0, 3, {})), std::invalid_argument);
    CHECK_THROWS_AS(find_seam_dp(make(3, 0, {})), std::invalid_argument);
    CHECK_THROWS_AS(find_seam_dp(make(2, 2, {1, 2, 3})), std::invalid_argument);  // 3 values, needs 4
}

// ---------------------------------------------------------------------------------------------
// Tie-breaking (GLOSSARY.md: Optimal seam): leftmost, judged from the bottom row upward
// ---------------------------------------------------------------------------------------------

TEST_CASE("Tie: all-equal energy gives the leftmost column everywhere") {
    EnergyMap e = make(3, 3, {
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,
    });
    CHECK(find_seam_dp(e) == Seam{0, 0, 0});
}

TEST_CASE("Tie in the last row goes to the leftmost cell") {
    // Both bottom corners cost 1 in total; column 0 must win.
    EnergyMap e = make(3, 2, {
        1, 9, 1,
        0, 9, 0,
    });
    CHECK(find_seam_dp(e)[1] == 0);
}

TEST_CASE("Tie while backtracking: left beats straight beats right") {
    SUBCASE("all three parents tie: left wins") {
        EnergyMap e = make(3, 2, {
            1, 1, 1,
            9, 0, 9,
        });
        CHECK(find_seam_dp(e) == Seam{0, 1});
    }
    SUBCASE("left and right tie, straight is dearer: left wins") {
        EnergyMap e = make(3, 2, {
            1, 5, 1,
            9, 0, 9,
        });
        CHECK(find_seam_dp(e) == Seam{0, 1});
    }
    SUBCASE("straight and right tie at the left border: straight wins") {
        EnergyMap e = make(2, 2, {
            1, 1,
            0, 9,
        });
        CHECK(find_seam_dp(e) == Seam{0, 0});
    }
    SUBCASE("right wins only when strictly cheaper") {
        EnergyMap e = make(3, 2, {
            5, 5, 1,
            9, 0, 9,
        });
        CHECK(find_seam_dp(e) == Seam{2, 1});
    }
    SUBCASE("right wins when strictly cheaper than a tied left and straight") {
        EnergyMap e = make(3, 2, {
            4, 4, 1,
            9, 0, 9,
        });
        CHECK(find_seam_dp(e) == Seam{2, 1});
    }
}

// ---------------------------------------------------------------------------------------------
// Optimality proof by exhaustion
// ---------------------------------------------------------------------------------------------

// Enumerate every seam of a W x H map (no pruning).
static void enumerate_seams(const EnergyMap& e, int y, Seam& cur, std::vector<Seam>& out) {
    if (y == e.height) {
        out.push_back(cur);
        return;
    }
    for (int x = 0; x < e.width; ++x) {
        if (y > 0 && std::abs(x - cur[y - 1]) > 1) continue;
        cur[y] = x;
        enumerate_seams(e, y + 1, cur, out);
    }
}

TEST_CASE("Brute force: DP cost equals the minimum over every seam, and ties follow the rule") {
    std::mt19937 rng(20260401);  // fixed seed: the test is deterministic
    int maps_checked = 0;
    for (int w = 1; w <= 6; ++w) {
        for (int h = 1; h <= 6; ++h) {
            for (int trial = 0; trial < 50; ++trial) {
                // Small value range for half of the maps so that ties are frequent.
                // Integer-valued energies keep every sum exact, so costs compare with ==.
                const int hi = (trial % 2 == 0) ? 3 : 20;
                std::uniform_int_distribution<int> val(0, hi);
                EnergyMap e;
                e.width = w;
                e.height = h;
                e.e.resize(size_t(w) * h);
                for (double& v : e.e) v = val(rng);

                std::vector<Seam> all;
                Seam cur(h);
                enumerate_seams(e, 0, cur, all);
                REQUIRE(!all.empty());

                double best_cost = seam_cost(e, all[0]);
                for (const Seam& s : all) best_cost = std::min(best_cost, seam_cost(e, s));

                // Among minimum-cost seams, the rule picks the one that is smallest when compared
                // from the bottom row upward.
                const Seam* expected = nullptr;
                for (const Seam& s : all) {
                    if (seam_cost(e, s) != best_cost) continue;
                    if (!expected ||
                        std::lexicographical_compare(s.rbegin(), s.rend(), expected->rbegin(), expected->rend()))
                        expected = &s;
                }

                Seam got = find_seam_dp(e);
                INFO("w=" << w << " h=" << h << " trial=" << trial);
                REQUIRE(is_valid_seam(got, w, h));
                CHECK(seam_cost(e, got) == best_cost);
                CHECK(got == *expected);
                ++maps_checked;
            }
        }
    }
    CHECK(maps_checked == 6 * 6 * 50);  // 1800 maps, well above the required 200
}

TEST_CASE("Random larger maps: the returned seam is always valid") {
    std::mt19937 rng(7);
    std::uniform_real_distribution<double> val(0.0, 1000.0);
    for (int trial = 0; trial < 50; ++trial) {
        EnergyMap e;
        e.width = 1 + int(rng() % 40);
        e.height = 1 + int(rng() % 40);
        e.e.resize(size_t(e.width) * e.height);
        for (double& v : e.e) v = val(rng);
        CHECK(is_valid_seam(find_seam_dp(e), e.width, e.height));
    }
}

// ---------------------------------------------------------------------------------------------
// seam_cost and remove_vertical_seam
// ---------------------------------------------------------------------------------------------

TEST_CASE("seam_cost sums the energy under the seam") {
    EnergyMap e = make(3, 3, {
        1, 2, 3,
        4, 5, 6,
        7, 8, 9,
    });
    CHECK(seam_cost(e, Seam{0, 0, 0}) == doctest::Approx(1 + 4 + 7));
    CHECK(seam_cost(e, Seam{1, 2, 1}) == doctest::Approx(2 + 6 + 8));
}

TEST_CASE("remove_vertical_seam shrinks width by one") {
    Image img(3, 2);
    Image out = remove_vertical_seam(img, Seam{1, 1});
    CHECK(out.width == 2);
    CHECK(out.height == 2);
}

TEST_CASE("remove_vertical_seam deletes exactly the seam pixels and keeps the rest in order") {
    // 4x2 image whose red channel is a unique id per pixel (y*4 + x).
    Image img(4, 2);
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 4; ++x) {
            img.px(x, y)[0] = uint8_t(y * 4 + x);
            img.px(x, y)[1] = uint8_t(100 + y * 4 + x);
            img.px(x, y)[2] = uint8_t(200 + y * 4 + x);
        }
    Image out = remove_vertical_seam(img, Seam{1, 2});  // remove (1,0) and (2,1)
    REQUIRE(out.width == 3);
    REQUIRE(out.height == 2);
    const int expect_red[2][3] = {{0, 2, 3}, {4, 5, 7}};
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 3; ++x) {
            CHECK(out.px(x, y)[0] == expect_red[y][x]);
            CHECK(out.px(x, y)[1] == 100 + expect_red[y][x]);  // all three channels move together
            CHECK(out.px(x, y)[2] == 200 + expect_red[y][x]);
        }
}

TEST_CASE("remove_vertical_seam on a width-1 image leaves zero columns") {
    Image img(1, 3);
    Image out = remove_vertical_seam(img, Seam{0, 0, 0});
    CHECK(out.width == 0);
    CHECK(out.height == 3);
    CHECK(out.rgb.empty());
}

TEST_CASE("remove_vertical_seam rejects malformed seams") {
    Image img(3, 2);
    CHECK_THROWS_AS(remove_vertical_seam(img, Seam{1}), std::invalid_argument);        // too short
    CHECK_THROWS_AS(remove_vertical_seam(img, Seam{1, 1, 1}), std::invalid_argument);  // too long
    CHECK_THROWS_AS(remove_vertical_seam(img, Seam{3, 1}), std::invalid_argument);     // column == width
    CHECK_THROWS_AS(remove_vertical_seam(img, Seam{0, -1}), std::invalid_argument);    // negative column
    CHECK_THROWS_AS(remove_vertical_seam(Image{}, Seam{}), std::invalid_argument);     // no columns
}
