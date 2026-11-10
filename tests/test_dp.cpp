#include <cstdlib>

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

TEST_CASE("remove_vertical_seam shrinks width by one") {
    Image img(3, 2);
    Image out = remove_vertical_seam(img, Seam{1, 1});
    CHECK(out.width == 2);
    CHECK(out.height == 2);
}
