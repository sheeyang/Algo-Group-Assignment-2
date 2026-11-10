#include "doctest.h"
#include "energy.h"

using namespace seam;

// Fill the whole image with one grey level (R = G = B = v).
static void fill_grey(Image& img, int x0, int x1, int y0, int y1, uint8_t v) {
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
            for (int c = 0; c < 3; ++c) img.px(x, y)[c] = v;
}

// Sobel kernels (see energy.cpp):   Gx = [-1 0 1; -2 0 2; -1 0 1]   Gy = [-1 -2 -1; 0 0 0; 1 2 1]
// energy = |Gx| + |Gy|. For a grey image with R = G = B = v the grayscale value is v (the three
// weights 0.299 + 0.587 + 0.114 sum to 1).

TEST_CASE("Sobel energy: a flat image has zero energy everywhere") {
    Image img(4, 3);
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 4; ++x) {
            img.px(x, y)[0] = 100;
            img.px(x, y)[1] = 150;
            img.px(x, y)[2] = 200;
        }
    EnergyMap m = sobel_energy(img);
    REQUIRE(m.width == 4);
    REQUIRE(m.height == 3);
    REQUIRE(m.e.size() == 12);
    for (double v : m.e) CHECK(v == doctest::Approx(0.0));
}

TEST_CASE("Sobel energy: a vertical edge is high only next to the edge") {
    // Columns 0-1 black, columns 2-3 white. Hand calculation at x = 1: the column to the left is
    // 0 and the column to the right is 255, in all three kernel rows, so
    // Gx = (1 + 2 + 1) * 255 = 1020. Gy = 0 because every row is identical. Same at x = 2.
    // At x = 0 and x = 3 the missing neighbour is clamped to the pixel itself, so both sides
    // of the kernel see the same value: Gx = 0.
    Image img(4, 3);
    fill_grey(img, 2, 4, 0, 3, 255);
    EnergyMap m = sobel_energy(img);
    for (int y = 0; y < 3; ++y) {
        CHECK(m.at(0, y) == doctest::Approx(0.0));
        CHECK(m.at(1, y) == doctest::Approx(1020.0));
        CHECK(m.at(2, y) == doctest::Approx(1020.0));
        CHECK(m.at(3, y) == doctest::Approx(0.0));
    }
}

TEST_CASE("Sobel energy: a horizontal edge is high only next to the edge") {
    // Rows 0-1 black, rows 2-3 white: the transpose of the case above, so the result is symmetric.
    Image img(3, 4);
    fill_grey(img, 0, 3, 2, 4, 255);
    EnergyMap m = sobel_energy(img);
    const double expect[4] = {0.0, 1020.0, 1020.0, 0.0};
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 3; ++x) CHECK(m.at(x, y) == doctest::Approx(expect[y]));
}

TEST_CASE("Sobel energy: border pixels clamp missing neighbours to the nearest pixel") {
    // 2x1 image, grey 0 then 100. There is only one row, so the rows above and below clamp to it
    // and Gy cancels. At x = 0 the left neighbour clamps to x = 0 itself (0) and the right is 100:
    // Gx = (1 + 2 + 1) * (100 - 0) = 400. At x = 1 the right neighbour clamps to itself (100) and
    // the left is 0: also 400.
    Image img(2, 1);
    fill_grey(img, 1, 2, 0, 1, 100);
    EnergyMap m = sobel_energy(img);
    CHECK(m.at(0, 0) == doctest::Approx(400.0));
    CHECK(m.at(1, 0) == doctest::Approx(400.0));
}

TEST_CASE("Sobel energy: a 1x1 image has zero energy") {
    Image img(1, 1);
    img.px(0, 0)[0] = 255;
    EnergyMap m = sobel_energy(img);
    REQUIRE(m.e.size() == 1);
    CHECK(m.at(0, 0) == doctest::Approx(0.0));
}

TEST_CASE("Sobel energy: a single white pixel in a black 3x3 image") {
    // White centre (1,1). The centre's own kernel sees only black neighbours, so its energy is 0.
    // Every other pixel touches the white centre through one kernel entry:
    //  - (0,0): Gx = +255 (weight 1, from (1,1)), Gy = +255 -> 510
    //  - (1,0): Gy = 2 * 255 = 510, Gx = 0 (left and right columns are black) -> 510
    // By the symmetry of the image the four corners are 510 and the four edge cells are 510.
    Image img(3, 3);
    fill_grey(img, 1, 2, 1, 2, 255);
    EnergyMap m = sobel_energy(img);
    CHECK(m.at(1, 1) == doctest::Approx(0.0));
    CHECK(m.at(0, 0) == doctest::Approx(510.0));
    CHECK(m.at(2, 0) == doctest::Approx(510.0));
    CHECK(m.at(0, 2) == doctest::Approx(510.0));
    CHECK(m.at(2, 2) == doctest::Approx(510.0));
    CHECK(m.at(1, 0) == doctest::Approx(510.0));
    CHECK(m.at(0, 1) == doctest::Approx(510.0));
    CHECK(m.at(2, 1) == doctest::Approx(510.0));
    CHECK(m.at(1, 2) == doctest::Approx(510.0));
}

TEST_CASE("Sobel energy: grayscale uses the luma weights 0.299 / 0.587 / 0.114") {
    // 2x1 image: left pixel black, right pixel a pure channel at 255. Energy = 4 * grayscale of that
    // pixel (the Gx calculation from the clamping test).
    struct Case { uint8_t r, g, b; double weight; };
    const Case cases[] = {{255, 0, 0, 0.299}, {0, 255, 0, 0.587}, {0, 0, 255, 0.114}};
    for (const Case& c : cases) {
        Image img(2, 1);
        img.px(1, 0)[0] = c.r;
        img.px(1, 0)[1] = c.g;
        img.px(1, 0)[2] = c.b;
        EnergyMap m = sobel_energy(img);
        CHECK(m.at(0, 0) == doctest::Approx(4 * 255 * c.weight));
        CHECK(m.at(1, 0) == doctest::Approx(4 * 255 * c.weight));
    }
}

TEST_CASE("Sobel energy: energy of the transposed image is the transposed energy map") {
    // The kernels are transposes of each other and |Gx| + |Gy| is symmetric, so a horizontal seam
    // found on the transposed image is a faithful horizontal seam of the original.
    Image img(5, 4);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 5; ++x)
            for (int c = 0; c < 3; ++c) img.px(x, y)[c] = uint8_t((x * 37 + y * 91 + c * 53) % 256);
    Image t = transpose(img);
    EnergyMap a = sobel_energy(img), b = sobel_energy(t);
    REQUIRE(b.width == a.height);
    REQUIRE(b.height == a.width);
    for (int y = 0; y < a.height; ++y)
        for (int x = 0; x < a.width; ++x) CHECK(a.at(x, y) == doctest::Approx(b.at(y, x)));
}
