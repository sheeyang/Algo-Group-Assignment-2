#pragma once
#include <vector>

#include "energy.h"
#include "maxflow.h"

namespace seam {

// A vertical seam: seam[y] is the column removed in row y.
// Invariant: |seam[y] - seam[y-1]| <= 1 and 0 <= seam[y] < width.
using Seam = std::vector<int>;

// Sum of energy along the seam (GLOSSARY.md: Seam cost).
double seam_cost(const EnergyMap& e, const Seam& s);

// DP and greedy ties break toward the leftmost column. Graph cut may choose a
// different seam with the same minimum cost. Horizontal seams are found
// by transposing the image first (see resize.cpp).
Seam find_seam_dp(const EnergyMap& e);              // optimal, O(W*H)
Seam find_seam_greedy(const EnergyMap& e);          // local row-by-row choice
Seam find_seam_graphcut_ek(const EnergyMap& e);     // min s-t cut, Edmonds-Karp
Seam find_seam_graphcut_dinic(const EnergyMap& e);  // min s-t cut, Dinic

using MaxFlowFn = long long (*)(FlowGraph&, int, int);
// Shared network and seam extraction for EK and Dinic. Energies must be finite
// and nonnegative; capacities are round(energy * 1000), so arbitrary doubles
// are optimised to that precision. Integer-valued energies are represented exactly
// within the supported capacity range.
Seam find_seam_graphcut(const EnergyMap& e, MaxFlowFn maxflow);

// Returns a copy of img with the vertical seam removed (one column narrower).
Image remove_vertical_seam(const Image& img, const Seam& s);

}  // namespace seam
