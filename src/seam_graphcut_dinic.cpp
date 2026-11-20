/**
 * @brief Dinic adapter for the common C3 minimum-cut seam network.
 * @file seam_graphcut_dinic.cpp
 */
#include "seam.h"

namespace seam {

// Same min-cut network and seam extraction as the Edmonds-Karp variant
// (find_seam_graphcut in seam_graphcut.cpp); only the max-flow solver differs.
// Dinic is faster on these layered networks, but by max-flow/min-cut both
// return a seam of the same optimal cost.
/**
 * @brief Select Dinic for the shared graph-cut network and seam extraction.
 * @param e Input row-major energy map.
 * @return Optimal vertical seam for rounded integer capacities.
 * @see find_seam_graphcut
 * @note Validation and exceptions are inherited from the common finder.
 */
Seam find_seam_graphcut_dinic(const EnergyMap& e) {
    return find_seam_graphcut(e, dinic);
}

}  // namespace seam
