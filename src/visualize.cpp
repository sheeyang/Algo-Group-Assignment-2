#include "visualize.h"

#include <stdexcept>

namespace seam {

static void paint(Image& img, int x, int y, Rgb c) {
    uint8_t* p = img.px(x, y);
    p[0] = c.r; p[1] = c.g; p[2] = c.b;
}

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

SeamTracker::SeamTracker(int width, int height)
    : w0_(width), h0_(height), cw_(width), ch_(height), map_(size_t(width) * height),
      order_(size_t(width) * height, 0) {
    if (width < 1 || height < 1) throw std::invalid_argument("SeamTracker: empty image");
    for (size_t i = 0; i < map_.size(); ++i) map_[i] = int32_t(i);
}

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

Image SeamTracker::overview(const Image& original, Rgb vcolor, Rgb hcolor) const {
    if (original.width != w0_ || original.height != h0_)
        throw std::invalid_argument("SeamTracker::overview: original has the wrong size");
    Image out = original;
    for (int y = 0; y < h0_; ++y)
        for (int x = 0; x < w0_; ++x)
            if (int k = order_[size_t(y) * w0_ + x]) paint(out, x, y, history_[k - 1].vertical ? vcolor : hcolor);
    return out;
}

Image replay(const Image& start, const std::vector<SeamRecord>& history, int from, int to) {
    if (from < 0 || to < from || to > int(history.size())) throw std::out_of_range("replay: bad step range");
    Image cur = start;
    for (int i = from; i < to; ++i) {
        if (history[i].vertical) cur = remove_vertical_seam(cur, history[i].seam);
        else cur = transpose(remove_vertical_seam(transpose(cur), history[i].seam));
    }
    return cur;
}

Image render_step(const Image& original, const std::vector<SeamRecord>& history, int k, Rgb color) {
    if (k < 0 || k >= int(history.size())) throw std::out_of_range("render_step: no such step");
    return draw_seam(replay(original, history, 0, k), history[k].seam, history[k].vertical, color);
}

}  // namespace seam
