#include <algorithm>
#include <stdexcept>

#include "seam.h"

namespace seam {

double seam_cost(const EnergyMap& e, const Seam& s) {
    double c = 0;
    for (int y = 0; y < e.height; ++y) c += e.at(s[y], y);
    return c;
}

// DP seam finder (GLOSSARY.md). Finds an optimal vertical seam in O(W*H) time.
//
// Subproblem: M[y][x] = the cheapest cost of any seam that starts anywhere in the top row
// and ends at pixel (x, y). Such a seam reaches (x, y) from one of its three parents in the
// row above, (x-1, y-1), (x, y-1) or (x+1, y-1), so
//
//     M[0][x] = e[0][x]
//     M[y][x] = e[y][x] + min(M[y-1][x-1], M[y-1][x], M[y-1][x+1])      (y >= 1)
//
// where a parent that falls outside the image (x-1 < 0 or x+1 >= W) is simply not a candidate.
// This is correct because of optimal substructure: the prefix of a cheapest seam ending at
// (x, y) must itself be a cheapest seam ending at its parent, otherwise swapping in a cheaper
// prefix would give a cheaper seam to (x, y).
//
// The optimal seam ends at the cheapest cell of the last row; we then walk back up.
//
// Tie-breaking (GLOSSARY.md: Optimal seam): among equal-cost seams we return the one with the
// smallest column in the last row, then in the row above, and so on (leftmost, judged from the
// bottom upward). The table fill is unaffected by ties because it stores only costs; the rule is
// applied in the two places where a choice is made: the last-row minimum and each backtracking step.
Seam find_seam_dp(const EnergyMap& e) {
    const int W = e.width, H = e.height;
    if (W < 1 || H < 1) throw std::invalid_argument("find_seam_dp: energy map is empty");
    if (e.e.size() != size_t(W) * H) throw std::invalid_argument("find_seam_dp: energy map size mismatch");

    // Full table, row-major, M[y*W+x]. (A single seam only needs the previous row, but backtracking
    // needs every row, so the whole table is kept: O(W*H) space.)
    std::vector<double> M(size_t(W) * H);
    for (int x = 0; x < W; ++x) M[x] = e.at(x, 0);
    for (int y = 1; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            double best = M[size_t(y - 1) * W + x];                                  // parent straight above
            if (x > 0) best = std::min(best, M[size_t(y - 1) * W + x - 1]);          // upper-left parent
            if (x < W - 1) best = std::min(best, M[size_t(y - 1) * W + x + 1]);      // upper-right parent
            M[size_t(y) * W + x] = e.at(x, y) + best;
        }
    }

    // Seam end: cheapest cell in the last row. Strict `<` keeps the leftmost one on a tie.
    Seam s(H);
    int x = 0;
    for (int i = 1; i < W; ++i)
        if (M[size_t(H - 1) * W + i] < M[size_t(H - 1) * W + x]) x = i;
    s[H - 1] = x;

    // Backtrack: at (x, y) move to whichever parent has the smallest M. Candidates are tried in
    // the order left, straight, right. The `<=` against the straight parent lets the left one win
    // a tie with it; the strict `<` for the right one means it only wins when strictly cheaper than
    // both. Together that picks the leftmost minimum.
    for (int y = H - 1; y > 0; --y) {
        int bx = x;
        double best = M[size_t(y - 1) * W + x];
        if (x > 0 && M[size_t(y - 1) * W + x - 1] <= best) {
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
    if (img.width < 1) throw std::invalid_argument("remove_vertical_seam: image has no columns");
    if (int(s.size()) != img.height)
        throw std::invalid_argument("remove_vertical_seam: seam length must equal image height");
    for (int y = 0; y < img.height; ++y)
        if (s[y] < 0 || s[y] >= img.width)
            throw std::invalid_argument("remove_vertical_seam: seam column outside the image");

    Image out(img.width - 1, img.height);
    for (int y = 0; y < img.height; ++y) {
        int ox = 0;  // write position in the output row; skips the seam pixel
        for (int x = 0; x < img.width; ++x) {
            if (x == s[y]) continue;
            for (int c = 0; c < 3; ++c) out.px(ox, y)[c] = img.px(x, y)[c];
            ++ox;
        }
    }
    return out;
}

}  // namespace seam
