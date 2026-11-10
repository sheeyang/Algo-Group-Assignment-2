#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace seam {

// Row-major 8-bit RGB image.
struct Image {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> rgb;  // size == width * height * 3

    Image() = default;
    Image(int w, int h) : width(w), height(h), rgb(size_t(w) * h * 3, 0) {}

    uint8_t* px(int x, int y) { return &rgb[(size_t(y) * width + x) * 3]; }
    const uint8_t* px(int x, int y) const { return &rgb[(size_t(y) * width + x) * 3]; }
};

Image load_image(const std::string& path);                   // throws std::runtime_error
void save_image(const Image& img, const std::string& path);  // .png or .jpg
Image transpose(const Image& img);                           // swap x/y (for horizontal seams)

}  // namespace seam
