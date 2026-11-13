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

// A solver must finish max-flow and leave the residual graph in its argument;
// seam extraction uses those updated capacities, rather than the return value.
using MaxFlowFn = long long (*)(FlowGraph&, int, int);

// Find a minimum-cost vertical seam using the supplied EK or Dinic solver.
// Returns one column per row, with adjacent columns differing by at most one.
// Does not modify e. Energies must be finite and nonnegative; capacities are
// round(energy * 1000), so arbitrary doubles are optimised to that precision.
// Integer-valued energies are represented exactly within the supported range.
// Tied minima may produce different positions from DP. Throws invalid_argument
// for empty/inconsistent maps, invalid energies, or a null solver; throws
// overflow_error when node indices or capacities exceed the supported range.
// Network construction and extraction take O(W*H) time and storage, in addition
// to the supplied solver's cost. Horizontal seams use a transposed input map.
Seam find_seam_graphcut(const EnergyMap& e, MaxFlowFn maxflow);

// Returns a copy of img with the vertical seam removed (one column narrower).
Image remove_vertical_seam(const Image& img, const Seam& s);

}  // namespace seam
