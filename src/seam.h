#pragma once
#include <vector>

#include "energy.h"

namespace seam {

// A vertical seam: seam[y] is the column removed in row y.
// Invariant: |seam[y] - seam[y-1]| <= 1 and 0 <= seam[y] < width.
using Seam = std::vector<int>;

// Sum of energy along the seam (GLOSSARY.md: Seam cost).
double seam_cost(const EnergyMap& e, const Seam& s);

// Seam finders. Ties break toward the leftmost column. Horizontal seams are found
// by transposing the image first (see resize.cpp).
Seam find_seam_dp(const EnergyMap& e);              // optimal, O(W*H)
Seam find_seam_greedy(const EnergyMap& e);          // local row-by-row choice
Seam find_seam_graphcut_ek(const EnergyMap& e);     // min s-t cut, Edmonds-Karp
Seam find_seam_graphcut_dinic(const EnergyMap& e);  // min s-t cut, Dinic

// Returns a copy of img with the vertical seam removed (one column narrower).
Image remove_vertical_seam(const Image& img, const Seam& s);

}  // namespace seam
