#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "image.h"

#include <stdexcept>

#include "stb_image.h"
#include "stb_image_write.h"

namespace seam {

static bool ends_with(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

Image load_image(const std::string& path) {
    int w, h, n;
    uint8_t* data = stbi_load(path.c_str(), &w, &h, &n, 3);
    if (!data) throw std::runtime_error("cannot load image: " + path);
    Image img(w, h);
    img.rgb.assign(data, data + size_t(w) * h * 3);
    stbi_image_free(data);
    return img;
}

void save_image(const Image& img, const std::string& path) {
    const bool jpg = ends_with(path, ".jpg") || ends_with(path, ".jpeg");
    int ok = jpg ? stbi_write_jpg(path.c_str(), img.width, img.height, 3, img.rgb.data(), 95)
                 : stbi_write_png(path.c_str(), img.width, img.height, 3, img.rgb.data(), img.width * 3);
    if (!ok) throw std::runtime_error("cannot write image: " + path);
}

Image transpose(const Image& img) {
    Image out(img.height, img.width);
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x)
            for (int c = 0; c < 3; ++c) out.px(y, x)[c] = img.px(x, y)[c];
    return out;
}

}  // namespace seam
