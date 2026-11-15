#include <stdexcept>

#include "seam.h"

namespace seam {

// Greedy seam finder (GLOSSARY.md: Greedy seam finder). Builds a vertical seam one row at a time.
//
//     seam[0] = column of the cheapest pixel in the top row
//     seam[y] = the cheapest of columns seam[y-1]-1, seam[y-1], seam[y-1]+1 in row y   (y >= 1)
//
// A column outside the image (x-1 < 0 or x+1 >= W) is simply not a candidate, so the seam stays
// connected and in bounds for any width, including 1.
//
// Every choice is final and looks only at the next row (no look-ahead and no backtracking). That
// is what makes it fast, and also why it is not optimal: a cheap pixel now can lead into a region
// where every reachable pixel is expensive, while a slightly dearer start would have reached a
// cheap path. Its seam cost is therefore always >= the DP seam cost (tests/test_greedy.cpp shows a
// case where it is strictly worse).
//
// Cost: O(W) to scan the top row plus 3 comparisons per row, so O(W + H) time for one seam, and no
// memory besides the seam itself. Compare find_seam_dp, which fills a W*H table: O(W*H) time and space.
//
// Ties go to the leftmost column, both for the start pixel and for each step (GLOSSARY.md), so the
// result is deterministic.
Seam find_seam_greedy(const EnergyMap& e) {
    const int W = e.width, H = e.height;
    if (W < 1 || H < 1) throw std::invalid_argument("find_seam_greedy: energy map is empty");
    if (e.e.size() != size_t(W) * H) throw std::invalid_argument("find_seam_greedy: energy map size mismatch");

    Seam s(H);

    // Start: cheapest pixel of the top row. Strict `<` keeps the leftmost one on a tie.
    int x = 0;
    for (int i = 1; i < W; ++i)
        if (e.at(i, 0) < e.at(x, 0)) x = i;
    s[0] = x;

    // Each next row: pick the cheapest of the (up to) three pixels reachable from x. Candidates are
    // tried left to right and replace the current best only when strictly cheaper, so the leftmost
    // of equal candidates wins.
    for (int y = 1; y < H; ++y) {
        int bx = x > 0 ? x - 1 : x;  // leftmost reachable column
        const int last = x < W - 1 ? x + 1 : x;  // rightmost reachable column
        for (int c = bx + 1; c <= last; ++c)
            if (e.at(c, y) < e.at(bx, y)) bx = c;
        x = bx;
        s[y] = x;
    }
    return s;
}

}  // namespace seam
