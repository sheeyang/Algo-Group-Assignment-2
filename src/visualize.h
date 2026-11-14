#pragma once
// Step-by-step visualisation (bonus 7a). No GUI code, no file I/O.
#include <cstdint>
#include <vector>

#include "image.h"
#include "seam.h"

namespace seam {

struct Rgb {
    uint8_t r, g, b;
};

// One removed seam, in the coordinates of the image it was found on (the StepCallback arguments).
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
class SeamTracker {
public:
    SeamTracker(int width, int height);

    // Record the next seam (coordinates of the current, already shrunk image) and shrink the map.
    void record(const Seam& s, bool vertical);

    int steps() const { return static_cast<int>(history_.size()); }
    const std::vector<SeamRecord>& history() const { return history_; }

    // 0 if the original pixel (x, y) was never removed, otherwise the 1-based step that removed it.
    int removed_at(int x, int y) const { return order_[size_t(y) * w0_ + x]; }

    // Original image with every removed pixel painted: vertical seams in vcolor, horizontal in hcolor.
    Image overview(const Image& original, Rgb vcolor = {255, 0, 0}, Rgb hcolor = {0, 200, 255}) const;

private:
    int w0_, h0_, cw_, ch_;
    std::vector<int32_t> map_;    // size cw_*ch_, original linear index of each current pixel
    std::vector<int32_t> order_;  // size w0_*h0_, removal step (0 = kept)
    std::vector<SeamRecord> history_;
};

// Image as it was just before step `to` (0-based), replaying steps from..to-1 on `start`, which must be
// the image before step `from`. Needs only the stored seams, no energy computation. The GUI can keep
// a checkpoint every N steps instead of every frame.
Image replay(const Image& start, const std::vector<SeamRecord>& history, int from, int to);

// Frame for step k (0-based): the image before removal k with seam k painted.
Image render_step(const Image& original, const std::vector<SeamRecord>& history, int k,
                  Rgb color = {255, 0, 0});

}  // namespace seam
