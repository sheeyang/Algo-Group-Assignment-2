/**
 * @file seam.h
 * @brief Shared seam-finder contracts and seam-removal interface.
 * @details C3 adds the solver callback and common graph-cut finder, letting EK
 * and C4's Dinic reuse the same network definition and seam extraction.
 */
#pragma once
#include <vector>

#include "energy.h"
#include "maxflow.h"

namespace seam {

// A vertical seam: seam[y] is the column removed in row y.
// Invariant: |seam[y] - seam[y-1]| <= 1 and 0 <= seam[y] < width.
using Seam = std::vector<int>;

// Sum of energy along the seam (GLOSSARY.md: Seam cost).
/**
 * @brief Sum original energy values along a vertical seam.
 * @param e Input row-major energy map.
 * @param s Selected seam in current image coordinates.
 * @return Sum of e.at(s[y], y) over all rows.
 * @pre s contains one valid column per row; inputs are not validated here.
 * @par Complexity
 * O(H) time and O(1) auxiliary storage.
 */
double seam_cost(const EnergyMap& e, const Seam& s);

// DP and greedy ties break toward the leftmost column. Graph cut may choose a
// different seam with the same minimum cost. Horizontal seams are found
// by transposing the image first (see resize.cpp).
/**
 * @brief Find an optimal vertical seam using a full cumulative-cost table.
 * @param e Input row-major energy map.
 * @return Minimum-cost seam; ties prefer leftmost columns from the bottom upward.
 * @throws std::invalid_argument For an empty or inconsistent map.
 * @par Complexity
 * O(W H) time and storage.
 */
Seam find_seam_dp(const EnergyMap& e);              // optimal, O(W*H)
/**
 * @brief Choose the cheapest top pixel, then the cheapest reachable pixel in each row.
 * @param e Input row-major energy map.
 * @return Connected seam with leftmost local tie-breaking; global optimality is not guaranteed.
 * @throws std::invalid_argument For an empty or inconsistent map.
 * @par Complexity
 * O(W+H) time and O(H) output storage.
 */
Seam find_seam_greedy(const EnergyMap& e);          // local row-by-row choice
/**
 * @brief Find a vertical seam by minimum cut using Edmonds-Karp.
 * @param[in] e Nonempty, consistent map of finite nonnegative energies.
 * @return One removable column per row, optimal for rounded capacities.
 * @throws std::invalid_argument For an invalid energy map.
 * @throws std::overflow_error If capacities or graph indices exceed limits.
 * @note Uses the common graph-cut validation, rounding and tie behaviour.
 * @par Complexity
 * O((W H)^3) worst-case time, O(W H) storage; use small inputs for EK.
 * @see find_seam_graphcut
 */
Seam find_seam_graphcut_ek(const EnergyMap& e);     // min s-t cut, Edmonds-Karp
/**
 * @brief Select Dinic for the shared graph-cut network and seam extraction.
 * @param e Input row-major energy map.
 * @return Optimal vertical seam for rounded integer capacities.
 * @see find_seam_graphcut
 * @note Validation and exceptions are inherited from the common finder.
 */
Seam find_seam_graphcut_dinic(const EnergyMap& e);  // min s-t cut, Dinic

/**
 * @brief Callback accepting a graph, source and sink, returning additional flow.
 * @details The solver must complete max-flow and leave the updated residual
 * graph in its argument. Seam extraction uses that graph, not the return value.
 */
using MaxFlowFn = long long (*)(FlowGraph&, int, int);

/**
 * @brief Build a pixel flow network and extract an optimal vertical seam.
 * @param[in] e Nonempty energy map whose storage matches width times height;
 * all entries must be finite and nonnegative. The map is not modified.
 * @param maxflow Non-null solver satisfying the MaxFlowFn residual contract.
 * @return One column per row, in bounds and differing by at most one between
 * adjacent rows, minimising the sum of round(energy * 1000) capacities.
 * @throws std::invalid_argument For an invalid map or missing solver.
 * @throws std::overflow_error If node indices or capacities exceed limits.
 * @note Exceptions from the supplied solver propagate to the caller.
 * @warning Rounding may change very close rankings in arbitrary double maps.
 * Compare costs rather than requiring DP's seam positions when minima tie.
 * @par Network definition
 * Each row has W real nodes and a virtual node. Source-to-first and
 * virtual-to-sink edges have INF capacity. Rightward edges carry scaled
 * energy; backward/diagonal INF edges enforce connected row prefixes.
 * INF is one greater than the sum of finite capacities, so no minimum cut
 * crosses an INF edge. The rightmost source-reachable real node is selected.
 * @par Complexity
 * Construction/extraction use O(W H) time and storage, plus the solver cost.
 * There are H(W+1)+2 nodes and 4WH+2H-2W original directed edges.
 * @see find_seam_graphcut_ek
 */
Seam find_seam_graphcut(const EnergyMap& e, MaxFlowFn maxflow);

// Returns a copy of img with the vertical seam removed (one column narrower).
/**
 * @brief Copy every row while omitting its selected seam pixel.
 * @param img RGB image with storage matching its dimensions.
 * @param s Selected seam in current image coordinates.
 * @return New image one column narrower; input remains unchanged.
 * @throws std::invalid_argument For zero width, wrong seam length or out-of-range columns.
 * @note Connectivity is assumed, not validated; width one produces a zero-width result.
 * @par Complexity
 * O(W H) time and output storage.
 */
Image remove_vertical_seam(const Image& img, const Seam& s);

}  // namespace seam
