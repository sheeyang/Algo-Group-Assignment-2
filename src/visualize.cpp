/**
 * @brief Seam painting, coordinate-map updates and deterministic frame replay.
 * @file visualize.cpp
 */
#include "visualize.h"

#include <stdexcept>

namespace seam {

/**
 * @brief Set one already-validated RGB pixel to a chosen overlay colour.
 * @param img RGB image with storage matching its dimensions.
 * @param x Zero-based column coordinate.
 * @param y Zero-based row coordinate.
 * @param c RGB colour to assign to the pixel.
 * @pre x and y are inside img.
 */
static void paint(Image& img, int x, int y, Rgb c) {
    uint8_t* p = img.px(x, y);
    p[0] = c.r; p[1] = c.g; p[2] = c.b;
}

/**
 * @brief Copy an image and paint one vertical or horizontal seam.
 * @param img RGB image with storage matching its dimensions.
 * @param s Selected seam in current image coordinates.
 * @param vertical True for a vertical seam; false for horizontal coordinates.
 * @param color RGB seam highlight colour.
 * @return Overlay image, leaving the input unchanged.
 * @throws std::invalid_argument For an inconsistent seam length or out-of-range position.
 * @note Connectivity is not checked. Copying the image costs O(W H).
 */
Image draw_seam(const Image& img, const Seam& s, bool vertical, Rgb color) {
    if (int(s.size()) != (vertical ? img.height : img.width))
        throw std::invalid_argument("draw_seam: seam length does not match the image");
    Image out = img;
    for (int i = 0; i < int(s.size()); ++i) {
        if (s[i] < 0 || s[i] >= (vertical ? img.width : img.height))
            throw std::invalid_argument("draw_seam: seam outside the image");
        if (vertical) paint(out, s[i], i, color);
        else paint(out, i, s[i], color);
    }
    return out;
}

/**
 * @brief Initialise an identity mapping from current pixels to original coordinates.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @throws std::invalid_argument For nonpositive dimensions.
 */
SeamTracker::SeamTracker(int width, int height)
    : w0_(width), h0_(height), cw_(width), ch_(height), map_(size_t(width) * height),
      order_(size_t(width) * height, 0) {
    if (width < 1 || height < 1) throw std::invalid_argument("SeamTracker: empty image");
    for (size_t i = 0; i < map_.size(); ++i) map_[i] = int32_t(i);
}

/**
 * @brief Map a seam to original pixels and shrink the current coordinate map.
 * @param s Selected seam in current image coordinates.
 * @param vertical True for a vertical seam; false for horizontal coordinates.
 * @pre The seam is connected and expressed in the current image coordinates.
 * @throws std::invalid_argument For incorrect length or out-of-range entries.
 * @par Complexity
 * O(current width * current height) time; records removal order for overview rendering.
 */
void SeamTracker::record(const Seam& s, bool vertical) {
    if (int(s.size()) != (vertical ? ch_ : cw_))
        throw std::invalid_argument("SeamTracker::record: seam length does not match the current image");
    for (int v : s)
        if (v < 0 || v >= (vertical ? cw_ : ch_))
            throw std::invalid_argument("SeamTracker::record: seam outside the current image");
    const int step = steps() + 1;
    if (vertical) {
        std::vector<int32_t> next(size_t(cw_ - 1) * ch_);
        for (int y = 0; y < ch_; ++y) {
            int ox = 0;
            for (int x = 0; x < cw_; ++x) {
                const int32_t orig = map_[size_t(y) * cw_ + x];
                if (x == s[y]) order_[orig] = step;       // this original pixel leaves the image
                else next[size_t(y) * (cw_ - 1) + ox++] = orig;
            }
        }
        map_.swap(next);
        --cw_;
    } else {
        std::vector<int32_t> next(size_t(cw_) * (ch_ - 1));
        for (int x = 0; x < cw_; ++x) {
            int oy = 0;
            for (int y = 0; y < ch_; ++y) {
                const int32_t orig = map_[size_t(y) * cw_ + x];
                if (y == s[x]) order_[orig] = step;
                else next[size_t(oy++) * cw_ + x] = orig;
            }
        }
        map_.swap(next);
        --ch_;
    }
    history_.push_back({s, vertical});
}

/**
 * @brief Paint all removed original pixels by seam orientation.
 * @param original Original image, with dimensions matching tracker construction.
 * @param vcolor Colour for removed vertical pixels.
 * @param hcolor Colour for removed horizontal pixels.
 * @return Original-sized overlay using vertical and horizontal colours.
 * @throws std::invalid_argument If original dimensions differ from tracker dimensions.
 */
Image SeamTracker::overview(const Image& original, Rgb vcolor, Rgb hcolor) const {
    if (original.width != w0_ || original.height != h0_)
        throw std::invalid_argument("SeamTracker::overview: original has the wrong size");
    Image out = original;
    for (int y = 0; y < h0_; ++y)
        for (int x = 0; x < w0_; ++x)
            if (int k = order_[size_t(y) * w0_ + x]) paint(out, x, y, history_[k - 1].vertical ? vcolor : hcolor);
    return out;
}

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
Image replay(const Image& start, const std::vector<SeamRecord>& history, int from, int to) {
    if (from < 0 || to < from || to > int(history.size())) throw std::out_of_range("replay: bad step range");
    Image cur = start;
    for (int i = from; i < to; ++i) {
        if (history[i].vertical) cur = remove_vertical_seam(cur, history[i].seam);
        else cur = transpose(remove_vertical_seam(transpose(cur), history[i].seam));
    }
    return cur;
}

/**
 * @brief Reconstruct the image before removal k and highlight that seam.
 * @param original Original image, with dimensions matching tracker construction.
 * @param history Seams in the coordinates of their successive pre-removal images.
 * @param k Zero-based removal step to display.
 * @param color RGB seam highlight colour.
 * @return Pre-removal frame with seam k painted.
 * @throws std::out_of_range If k does not index a stored seam.
 */
Image render_step(const Image& original, const std::vector<SeamRecord>& history, int k, Rgb color) {
    if (k < 0 || k >= int(history.size())) throw std::out_of_range("render_step: no such step");
    return draw_seam(replay(original, history, 0, k), history[k].seam, history[k].vertical, color);
}

}  // namespace seam
