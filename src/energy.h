#pragma once
#include <vector>

#include "image.h"

namespace seam {

// Row-major grid of per-pixel energy (see GLOSSARY.md: Energy map).
struct EnergyMap {
    int width = 0;
    int height = 0;
    std::vector<double> e;
    double& at(int x, int y) { return e[size_t(y) * width + x]; }
    double at(int x, int y) const { return e[size_t(y) * width + x]; }
};

// Backward energy: |Gx| + |Gy| of the Sobel filter on grayscale. Borders are clamped.
EnergyMap sobel_energy(const Image& img);

}  // namespace seam
