#include "energy.h"

#include <algorithm>
#include <cmath>

namespace seam {

static double gray(const Image& img, int x, int y) {
    x = std::clamp(x, 0, img.width - 1);
    y = std::clamp(y, 0, img.height - 1);
    const uint8_t* p = img.px(x, y);
    return 0.299 * p[0] + 0.587 * p[1] + 0.114 * p[2];
}

EnergyMap sobel_energy(const Image& img) {
    EnergyMap m;
    m.width = img.width;
    m.height = img.height;
    m.e.assign(size_t(m.width) * m.height, 0.0);
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            double gx = -gray(img, x - 1, y - 1) - 2 * gray(img, x - 1, y) - gray(img, x - 1, y + 1) +
                        gray(img, x + 1, y - 1) + 2 * gray(img, x + 1, y) + gray(img, x + 1, y + 1);
            double gy = -gray(img, x - 1, y - 1) - 2 * gray(img, x, y - 1) - gray(img, x + 1, y - 1) +
                        gray(img, x - 1, y + 1) + 2 * gray(img, x, y + 1) + gray(img, x + 1, y + 1);
            m.at(x, y) = std::abs(gx) + std::abs(gy);
        }
    }
    return m;
}

}  // namespace seam
