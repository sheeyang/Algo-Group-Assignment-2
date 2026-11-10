#include <cstdio>
#include <filesystem>
#include <stdexcept>

#include "doctest.h"
#include "image.h"

using namespace seam;

// Image where every pixel has a distinct, recoverable value.
static Image make_image(int w, int h) {
    Image img(w, h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            for (int c = 0; c < 3; ++c) img.px(x, y)[c] = uint8_t((x * 17 + y * 31 + c * 7) % 256);
    return img;
}

TEST_CASE("Image constructor sizes the buffer and zero-fills") {
    Image img(4, 3);
    CHECK(img.width == 4);
    CHECK(img.height == 3);
    REQUIRE(img.rgb.size() == 4 * 3 * 3);
    for (uint8_t v : img.rgb) CHECK(v == 0);
}

TEST_CASE("transpose swaps the dimensions and moves pixel (x,y) to (y,x)") {
    Image img = make_image(5, 3);
    Image t = transpose(img);
    REQUIRE(t.width == 3);
    REQUIRE(t.height == 5);
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x)
            for (int c = 0; c < 3; ++c) CHECK(t.px(y, x)[c] == img.px(x, y)[c]);
}

TEST_CASE("transpose twice is the identity") {
    for (auto [w, h] : {std::pair{5, 3}, {1, 4}, {4, 1}, {1, 1}, {7, 7}}) {
        Image img = make_image(w, h);
        Image back = transpose(transpose(img));
        INFO("w=" << w << " h=" << h);
        CHECK(back.width == img.width);
        CHECK(back.height == img.height);
        CHECK(back.rgb == img.rgb);
    }
}

TEST_CASE("load_image throws on a missing file") {
    CHECK_THROWS_AS(load_image("this/file/does/not/exist.png"), std::runtime_error);
}

TEST_CASE("PNG save then load round-trips exactly") {
    Image img = make_image(6, 4);
    const std::string path =
        (std::filesystem::temp_directory_path() / "seam_test_roundtrip.png").string();
    save_image(img, path);
    Image back = load_image(path);
    std::remove(path.c_str());
    CHECK(back.width == img.width);
    CHECK(back.height == img.height);
    CHECK(back.rgb == img.rgb);
}

TEST_CASE("save_image throws when the path cannot be written") {
    Image img = make_image(2, 2);
    CHECK_THROWS_AS(save_image(img, "this/dir/does/not/exist/out.png"), std::runtime_error);
}
