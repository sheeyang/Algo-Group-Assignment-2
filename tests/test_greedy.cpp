#include <cstdlib>
#include <random>
#include <stdexcept>

#include "doctest.h"
#include "resize.h"
#include "seam.h"

using namespace seam;

static EnergyMap make(int w, int h, std::vector<double> v) {
    EnergyMap m;
    m.width = w;
    m.height = h;
    m.e = std::move(v);
    return m;
}

// One column per row, inside the image, neighbouring rows at most one column apart (GLOSSARY.md: Seam).
static bool is_valid_seam(const Seam& s, int w, int h) {
    if (int(s.size()) != h) return false;
    for (int y = 0; y < h; ++y) {
        if (s[y] < 0 || s[y] >= w) return false;
        if (y > 0 && std::abs(s[y] - s[y - 1]) > 1) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------------------------
// The trap: greedy is strictly worse than DP (the example for the report, R2)
// ---------------------------------------------------------------------------------------------

TEST_CASE("Greedy gets trapped and pays strictly more than DP") {
    // Greedy starts at the 1 in column 0 (cheapest top pixel). From column 0 it can only reach
    // columns 0 and 1, which are 8 in both lower rows, so it pays 1 + 8 + 8 = 17.
    // DP sees the cheap column on the right: 4 + 1 + 1 = 6 (columns 1, 2, 2).
    EnergyMap e = make(3, 3, {
        1, 4, 4,
        8, 8, 1,
        8, 8, 1,
    });
    Seam g = find_seam_greedy(e);
    Seam d = find_seam_dp(e);
    CHECK(g == Seam{0, 0, 0});
    CHECK(d == Seam{1, 2, 2});
    CHECK(seam_cost(e, g) == doctest::Approx(17.0));
    CHECK(seam_cost(e, d) == doctest::Approx(6.0));
}

TEST_CASE("Greedy follows the hand-checked path") {
    // Start: column 2 (the 0). Row 1 from 2: {9, 9} at columns 1, 2 -> leftmost, column 1.
    // Row 2 from 1: {5, 1, 9} at columns 0, 1, 2 -> column 1. Cost 0 + 9 + 1 = 10 (DP finds 3).
    EnergyMap e = make(3, 3, {
        5, 1, 0,
        1, 9, 9,
        5, 1, 9,
    });
    Seam g = find_seam_greedy(e);
    CHECK(g == Seam{2, 1, 1});
    CHECK(seam_cost(e, g) == doctest::Approx(10.0));
    CHECK(seam_cost(e, find_seam_dp(e)) == doctest::Approx(3.0));
}

TEST_CASE("Greedy equals DP when the locally cheapest path is also the best") {
    // A single cheap column: both must take it.
    EnergyMap e = make(4, 4, {
        9, 9, 0, 9,
        9, 9, 0, 9,
        9, 9, 0, 9,
        9, 9, 0, 9,
    });
    CHECK(find_seam_greedy(e) == Seam{2, 2, 2, 2});
    CHECK(find_seam_greedy(e) == find_seam_dp(e));
}

// ---------------------------------------------------------------------------------------------
// Edge cases: tiny maps and borders
// ---------------------------------------------------------------------------------------------

TEST_CASE("Greedy on a width-1 map must take the only column") {
    EnergyMap e = make(1, 4, {7, 3, 9, 1});
    CHECK(find_seam_greedy(e) == Seam{0, 0, 0, 0});
}

TEST_CASE("Greedy on a height-1 map picks the cheapest pixel") {
    EnergyMap e = make(5, 1, {4, 2, 9, 2, 7});
    CHECK(find_seam_greedy(e) == Seam{1});  // leftmost of the two 2s
}

TEST_CASE("Greedy on a 1x1 map") {
    CHECK(find_seam_greedy(make(1, 1, {42})) == Seam{0});
}

TEST_CASE("Greedy at the left and right borders only considers columns inside the image") {
    // Right border: from column 2 the candidates are columns 1 and 2 only.
    EnergyMap right = make(3, 2, {
        9, 9, 0,
        0, 5, 3,
    });
    CHECK(find_seam_greedy(right) == Seam{2, 2});  // column 0 (the 0) is out of reach
    // Left border: from column 0 the candidates are columns 0 and 1 only.
    EnergyMap left = make(3, 2, {
        0, 9, 9,
        5, 3, 0,
    });
    CHECK(find_seam_greedy(left) == Seam{0, 1});  // column 2 (the 0) is out of reach
}

TEST_CASE("Greedy rejects an empty or inconsistent energy map") {
    CHECK_THROWS_AS(find_seam_greedy(EnergyMap{}), std::invalid_argument);
    CHECK_THROWS_AS(find_seam_greedy(make(0, 3, {})), std::invalid_argument);
    CHECK_THROWS_AS(find_seam_greedy(make(3, 0, {})), std::invalid_argument);
    CHECK_THROWS_AS(find_seam_greedy(make(2, 2, {1, 2, 3})), std::invalid_argument);
}

// ---------------------------------------------------------------------------------------------
// Ties: leftmost wins, for the start pixel and for every step
// ---------------------------------------------------------------------------------------------

TEST_CASE("Tie: all-equal energy gives the leftmost column everywhere") {
    EnergyMap e = make(4, 3, std::vector<double>(12, 5.0));
    CHECK(find_seam_greedy(e) == Seam{0, 0, 0});
}

TEST_CASE("Tie in a step: left beats straight beats right") {
    // Start at column 1 (the 0). Row 1: all three candidates equal -> column 0.
    EnergyMap all3 = make(3, 2, {
        9, 0, 9,
        4, 4, 4,
    });
    CHECK(find_seam_greedy(all3) == Seam{1, 0});
    // Straight and right tie, left is dearer -> straight.
    EnergyMap straight_right = make(3, 2, {
        9, 0, 9,
        5, 4, 4,
    });
    CHECK(find_seam_greedy(straight_right) == Seam{1, 1});
    // Left and right tie, straight is dearer -> left.
    EnergyMap left_right = make(3, 2, {
        9, 0, 9,
        4, 5, 4,
    });
    CHECK(find_seam_greedy(left_right) == Seam{1, 0});
    // Only right is cheapest -> right.
    EnergyMap only_right = make(3, 2, {
        9, 0, 9,
        5, 5, 4,
    });
    CHECK(find_seam_greedy(only_right) == Seam{1, 2});
}

// ---------------------------------------------------------------------------------------------
// Random maps: greedy is always valid and never cheaper than DP
// ---------------------------------------------------------------------------------------------

TEST_CASE("Random maps: greedy seam is valid and its cost is never below DP") {
    std::mt19937 rng(20261113);  // fixed seed: the test is deterministic
    std::uniform_int_distribution<int> dim(1, 12), val(0, 9);
    int strictly_worse = 0;
    const int maps = 1000;
    for (int t = 0; t < maps; ++t) {
        const int w = dim(rng), h = dim(rng);
        std::vector<double> v(size_t(w) * h);
        for (double& x : v) x = val(rng);  // integer-valued, so costs compare exactly
        EnergyMap e = make(w, h, v);

        Seam g = find_seam_greedy(e);
        REQUIRE(is_valid_seam(g, w, h));
        const double gc = seam_cost(e, g), dc = seam_cost(e, find_seam_dp(e));
        CHECK(gc >= dc);
        if (gc > dc) ++strictly_worse;
    }
    // Greedy is not optimal: on random maps it must lose to DP on a good share of them.
    CHECK(strictly_worse > maps / 10);
    MESSAGE("greedy strictly worse than DP on " << strictly_worse << " of " << maps << " random maps");
}

TEST_CASE("Random larger maps: greedy seam is always valid") {
    std::mt19937 rng(11);
    std::uniform_real_distribution<double> val(0.0, 1000.0);
    for (int t = 0; t < 20; ++t) {
        const int w = 50 + t * 7, h = 40 + t * 5;
        std::vector<double> v(size_t(w) * h);
        for (double& x : v) x = val(rng);
        CHECK(is_valid_seam(find_seam_greedy(make(w, h, v)), w, h));
    }
}

// ---------------------------------------------------------------------------------------------
// End to end: resize with the greedy finder
// ---------------------------------------------------------------------------------------------

TEST_CASE("Greedy resize reaches the exact target size in both directions") {
    std::mt19937 rng(3);
    std::uniform_int_distribution<int> val(0, 255);
    Image img(24, 18);
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x)
            for (int c = 0; c < 3; ++c) img.px(x, y)[c] = static_cast<unsigned char>(val(rng));

    int vertical = 0, horizontal = 0;
    Image out = resize(img, 15, 11, Algorithm::Greedy, [&](const Image& before, const Seam& s, bool v) {
        // Each removed seam is valid in the coordinates of the image it was removed from.
        if (v) {
            ++vertical;
            CHECK(is_valid_seam(s, before.width, before.height));
        } else {
            ++horizontal;
            CHECK(is_valid_seam(s, before.height, before.width));
        }
    });
    CHECK(out.width == 15);
    CHECK(out.height == 11);
    CHECK(vertical == 24 - 15);
    CHECK(horizontal == 18 - 11);
}
