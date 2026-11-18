#include "seam.h"

namespace seam {

// Same min-cut network and seam extraction as the Edmonds-Karp variant
// (find_seam_graphcut in seam_graphcut.cpp); only the max-flow solver differs.
// Dinic is faster on these layered networks, but by max-flow/min-cut both
// return a seam of the same optimal cost.
Seam find_seam_graphcut_dinic(const EnergyMap& e) {
    return find_seam_graphcut(e, dinic);
}

}  // namespace seam
