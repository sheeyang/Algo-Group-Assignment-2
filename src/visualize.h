/**
 * @brief Seam overlays, original-coordinate tracking and replay interfaces.
 * @file visualize.h
 */
#pragma once
// Step-by-step visualisation (bonus 7a). No GUI code, no file I/O.
#include <cstdint>
#include <vector>

#include "image.h"
#include "seam.h"

namespace seam {

/** @brief Three 8-bit colour channels used to paint seam overlays. */
struct Rgb {
    uint8_t r, g, b;
};

/**
 * @brief A removal expressed in that step's pre-removal image coordinates.
 * @details Stored seams cannot be painted directly onto the original image
 * after earlier removals; SeamTracker maintains the required coordinate map.
 */
struct SeamRecord {
    Seam seam;
    bool vertical;
};

// Copy of img with the seam painted in `color`. Vertical: pixel (seam[y], y) for every row y.
// Horizontal: pixel (x, seam[x]) for every column x (seam[x] is the row removed in column x).
Image draw_seam(const Image& img, const Seam& s, bool vertical, Rgb color = {255, 0, 0});

// Maps every seam back to ORIGINAL coordinates. Seams are found on shrinking images, so the tracker
// keeps an index map the same shape as the current image and shrinks it exactly like the image:
// map[y][x] = linear index (y0*W0+x0) of the original pixel now at (x, y).
// Usage:  SeamTracker t(w, h);  resize(..., [&](const Image&, const Seam& s, bool v){ t.record(s, v); });
/**
 * @brief Track original pixel identities while current image dimensions shrink.
 * @details Stores removal order and history, allowing an overview to be drawn
 * without recomputing seams. Construction and storage require O(W H) space.
 */
class SeamTracker {
public:
    /**
     * @brief Initialise an identity mapping from current pixels to original coordinates.
     * @param width Image width in pixels.
     * @param height Image height in pixels.
     * @throws std::invalid_argument For nonpositive dimensions.
     */
    SeamTracker(int width, int height);

    // Record the next seam (coordinates of the current, already shrunk image) and shrink the map.
    /**
     * @brief Map a seam to original pixels and shrink the current coordinate map.
     * @param s Selected seam in current image coordinates.
     * @param vertical True for a vertical seam; false for horizontal coordinates.
     * @pre The seam is connected and expressed in the current image coordinates.
     * @throws std::invalid_argument For incorrect length or out-of-range entries.
     * @par Complexity
     * O(current width * current height) time; records removal order for overview rendering.
     */
    void record(const Seam& s, bool vertical);

    /**
     * @brief Read the number of recorded removals.
     * @return Count of recorded seams.
     */
    int steps() const { return static_cast<int>(history_.size()); }
    /**
     * @brief Access the ordered seam records without copying them.
     * @return Const reference to history, invalidated by tracker destruction.
     */
    const std::vector<SeamRecord>& history() const { return history_; }

    // 0 if the original pixel (x, y) was never removed, otherwise the 1-based step that removed it.
    /**
     * @brief Look up when an original pixel was removed.
     * @param x Zero-based column coordinate.
     * @param y Zero-based row coordinate.
     * @return One-based removal step, or zero if the pixel has not been removed.
     * @pre x and y lie within the original image; access is unchecked.
     */
    int removed_at(int x, int y) const { return order_[size_t(y) * w0_ + x]; }

    // Original image with every removed pixel painted: vertical seams in vcolor, horizontal in hcolor.
    Image overview(const Image& original, Rgb vcolor = {255, 0, 0}, Rgb hcolor = {0, 200, 255}) const;

private:
    int w0_, h0_, cw_, ch_;
    std::vector<int32_t> map_;    ///< size cw_*ch_, original linear index of each current pixel
    std::vector<int32_t> order_;  ///< size w0_*h0_, removal step (0 = kept)
    std::vector<SeamRecord> history_;
};

// Image as it was just before step `to` (0-based), replaying steps from..to-1 on `start`, which must be
// the image before step `from`. Needs only the stored seams, no energy computation. The GUI can keep
// a checkpoint every N steps instead of every frame.
/**
 * @brief Apply stored removals in the half-open step interval [from,to).
 * @param start Image copied at the start of each timed resize.
 * @param history Seams in the coordinates of their successive pre-removal images.
 * @param from First step to apply (inclusive).
 * @param to First step not applied (exclusive).
 * @return Image immediately before step to.
 * @pre start is the image before step from and history coordinates match.
 * @throws std::out_of_range For an invalid step interval.
 * @note Only stored seams are replayed; energy is not recomputed.
 */
Image replay(const Image& start, const std::vector<SeamRecord>& history, int from, int to);

// Frame for step k (0-based): the image before removal k with seam k painted.
Image render_step(const Image& original, const std::vector<SeamRecord>& history, int k,
                  Rgb color = {255, 0, 0});

}  // namespace seam
