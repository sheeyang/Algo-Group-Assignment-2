/**
 * @brief Energy-map storage and backward Sobel energy interface.
 * @file energy.h
 */
#pragma once
#include <vector>

#include "image.h"

namespace seam {

/** @brief Owned row-major grid of per-pixel energies, indexed by (x,y). */
struct EnergyMap {
    int width = 0;  ///< Number of energy-map columns.
    int height = 0;  ///< Number of energy-map rows.
    std::vector<double> e;  ///< Values at offset y * width + x.
    /**
     * @brief Access a row-major energy value without bounds checking.
     * @param x Zero-based column coordinate.
     * @param y Zero-based row coordinate.
     * @return The energy value, or a writable reference for the mutable overload.
     * @pre Coordinates lie inside the map and storage matches its dimensions.
     */
    double& at(int x, int y) { return e[size_t(y) * width + x]; }
    /**
     * @brief Access a row-major energy value without bounds checking.
     * @param x Zero-based column coordinate.
     * @param y Zero-based row coordinate.
     * @return The energy value, or a writable reference for the mutable overload.
     * @pre Coordinates lie inside the map and storage matches its dimensions.
     */
    double at(int x, int y) const { return e[size_t(y) * width + x]; }
};

// Backward energy: |Gx| + |Gy| of the Sobel filter on grayscale. Borders are clamped.
/**
 * @brief Compute backward energy for every RGB pixel.
 * @param img RGB image with storage matching its dimensions.
 * @return A same-sized energy map of abs(Gx) + abs(Gy).
 * @pre img has consistent RGB storage and nonnegative dimensions.
 * @par Complexity
 * O(W H) time and output storage; borders are clamped.
 */
EnergyMap sobel_energy(const Image& img);

}  // namespace seam
