#include <stdexcept>
#include <vector>

#include "doctest.h"
#include "resize.h"

using namespace seam;

// Deterministic textured image, so the energy map is varied and seams are not all ties.
static Image make_image(int w, int h) {
    Image img(w, h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            img.px(x, y)[0] = uint8_t((x * x * 13 + y * 29 + 5) % 256);
            img.px(x, y)[1] = uint8_t((x * 41 + y * y * 7 + 11) % 256);
            img.px(x, y)[2] = uint8_t((x * 3 + y * 5 + x * y * 17) % 256);
        }
    return img;
}

namespace {
struct Step {
    int before_w, before_h;
    bool vertical;
    Seam seam;
    Image before;
};

std::vector<Step> record(const Image& img, int tw, int th, Image* result = nullptr) {
    std::vector<Step> steps;
    Image out = resize(img, tw, th, Algorithm::DP, [&](const Image& before, const Seam& s, bool vertical) {
        steps.push_back({before.width, before.height, vertical, s, before});
    });
    if (result) *result = out;
    return steps;
}
}  // namespace

TEST_CASE("resize to the same size is a no-op: same pixels, no callback calls") {
    Image img = make_image(8, 6);
    Image out;
    auto steps = record(img, 8, 6, &out);
    CHECK(steps.empty());
    CHECK(out.width == 8);
    CHECK(out.height == 6);
    CHECK(out.rgb == img.rgb);
}

TEST_CASE("resize hits the exact target size, and calls back once per removed seam") {
    struct Case { int w, h, tw, th; };
    const Case cases[] = {
        {8, 6, 5, 6},  // width only
        {8, 6, 8, 3},  // height only
        {8, 6, 5, 3},  // combined
        {8, 6, 6, 1},  // combined, height goes to 1
        {8, 6, 1, 6},  // width to 1
        {8, 6, 1, 1},  // down to a single pixel
        {1, 1, 1, 1},  // already a single pixel
    };
    for (const Case& c : cases) {
        INFO(c.w << "x" << c.h << " -> " << c.tw << "x" << c.th);
        Image out;
        auto steps = record(make_image(c.w, c.h), c.tw, c.th, &out);
        CHECK(out.width == c.tw);
        CHECK(out.height == c.th);
        CHECK(out.rgb.size() == size_t(c.tw) * c.th * 3);
        CHECK(int(steps.size()) == (c.w - c.tw) + (c.h - c.th));
    }
}

TEST_CASE("resize alternates vertical and horizontal seams, then finishes the longer axis") {
    // Remove 3 columns and 2 rows: V H V H V.
    auto steps = record(make_image(8, 6), 5, 4);
    REQUIRE(steps.size() == 5);
    const bool expect[5] = {true, false, true, false, true};
    for (size_t i = 0; i < 5; ++i) CHECK(steps[i].vertical == expect[i]);

    // Width-only target: vertical seams only. Height-only: horizontal seams only.
    for (const Step& s : record(make_image(8, 6), 4, 6)) CHECK(s.vertical);
    for (const Step& s : record(make_image(8, 6), 8, 2)) CHECK(!s.vertical);
}

TEST_CASE("each callback sees the image shrink by one in the removed dimension") {
    auto steps = record(make_image(8, 6), 5, 4);
    int w = 8, h = 6;
    for (const Step& s : steps) {
        CHECK(s.before_w == w);
        CHECK(s.before_h == h);
        // Seam is valid in the coordinates of `before`: a vertical seam has one entry per row,
        // a horizontal seam one entry per column.
        if (s.vertical) {
            CHECK(int(s.seam.size()) == h);
            for (int c : s.seam) CHECK((c >= 0 && c < w));
            --w;
        } else {
            CHECK(int(s.seam.size()) == w);
            for (int r : s.seam) CHECK((r >= 0 && r < h));
            --h;
        }
    }
    CHECK(w == 5);
    CHECK(h == 4);
}

TEST_CASE("width-only resize equals removing the DP seam repeatedly by hand") {
    Image img = make_image(9, 5);
    Image expected = img;
    for (int i = 0; i < 4; ++i)
        expected = remove_vertical_seam(expected, find_seam_dp(sobel_energy(expected)));
    Image got = resize(img, 5, 5, Algorithm::DP);
    CHECK(got.width == expected.width);
    CHECK(got.height == expected.height);
    CHECK(got.rgb == expected.rgb);
}

TEST_CASE("a horizontal seam on I is the vertical seam on transpose(I)") {
    Image img = make_image(7, 6);
    auto steps = record(img, 7, 3);  // height-only: every step is a horizontal seam
    REQUIRE(steps.size() == 3);
    for (const Step& s : steps) {
        REQUIRE(!s.vertical);
        // Computed independently of resize(): transpose, find a vertical seam, read it back.
        Seam expected = find_seam_dp(sobel_energy(transpose(s.before)));
        CHECK(s.seam == expected);
    }
}

TEST_CASE("height-only resize equals the transpose / width-only / transpose route") {
    Image img = make_image(7, 6);
    Image direct = resize(img, 7, 3, Algorithm::DP);
    Image via_transpose = transpose(resize(transpose(img), 3, 7, Algorithm::DP));
    CHECK(direct.width == via_transpose.width);
    CHECK(direct.height == via_transpose.height);
    CHECK(direct.rgb == via_transpose.rgb);
}

TEST_CASE("resize leaves the input image untouched") {
    Image img = make_image(6, 5);
    Image copy = img;
    resize(img, 3, 2, Algorithm::DP);
    CHECK(img.width == copy.width);
    CHECK(img.height == copy.height);
    CHECK(img.rgb == copy.rgb);
}

TEST_CASE("resize rejects invalid targets") {
    Image img = make_image(6, 5);
    CHECK_THROWS_AS(resize(img, 0, 5, Algorithm::DP), std::invalid_argument);
    CHECK_THROWS_AS(resize(img, 6, 0, Algorithm::DP), std::invalid_argument);
    CHECK_THROWS_AS(resize(img, -1, 3, Algorithm::DP), std::invalid_argument);
    CHECK_THROWS_AS(resize(img, 3, -1, Algorithm::DP), std::invalid_argument);
    CHECK_THROWS_AS(resize(img, 7, 5, Algorithm::DP), std::invalid_argument);  // wider than the image
    CHECK_THROWS_AS(resize(img, 6, 6, Algorithm::DP), std::invalid_argument);  // taller than the image
    CHECK_THROWS_AS(resize(img, 7, 3, Algorithm::DP), std::invalid_argument);  // one axis fine, one not
}
