#include <algorithm>

#include "seam.h"

namespace seam {

double seam_cost(const EnergyMap& e, const Seam& s) {
    double c = 0;
    for (int y = 0; y < e.height; ++y) c += e.at(s[y], y);
    return c;
}

Seam find_seam_dp(const EnergyMap& e) {
    const int W = e.width, H = e.height;
    // M[y*W+x] = min cost of a seam from the top row ending at (x, y).
    std::vector<double> M(size_t(W) * H);
    for (int x = 0; x < W; ++x) M[x] = e.at(x, 0);
    for (int y = 1; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            double best = M[size_t(y - 1) * W + x];
            if (x > 0) best = std::min(best, M[size_t(y - 1) * W + x - 1]);
            if (x < W - 1) best = std::min(best, M[size_t(y - 1) * W + x + 1]);
            M[size_t(y) * W + x] = e.at(x, y) + best;
        }
    }
    Seam s(H);
    int x = 0;
    for (int i = 1; i < W; ++i)
        if (M[size_t(H - 1) * W + i] < M[size_t(H - 1) * W + x]) x = i;  // strict < => leftmost
    s[H - 1] = x;
    for (int y = H - 1; y > 0; --y) {
        int bx = x;
        double best = M[size_t(y - 1) * W + x];
        if (x > 0 && M[size_t(y - 1) * W + x - 1] <= best) {  // prefer left on ties
            best = M[size_t(y - 1) * W + x - 1];
            bx = x - 1;
        }
        if (x < W - 1 && M[size_t(y - 1) * W + x + 1] < best) bx = x + 1;
        x = bx;
        s[y - 1] = x;
    }
    return s;
}

Image remove_vertical_seam(const Image& img, const Seam& s) {
    Image out(img.width - 1, img.height);
    for (int y = 0; y < img.height; ++y) {
        int ox = 0;
        for (int x = 0; x < img.width; ++x) {
            if (x == s[y]) continue;
            for (int c = 0; c < 3; ++c) out.px(ox, y)[c] = img.px(x, y)[c];
            ++ox;
        }
    }
    return out;
}

}  // namespace seam
