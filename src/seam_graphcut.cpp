#include <stdexcept>

#include "seam.h"

namespace seam {

// TODO(graph-cut owner): build the flow network from the brief (item 4), run
// Edmonds-Karp / Dinic, BFS the residual graph from the source, and take the
// rightmost source-side pixel per row.
Seam find_seam_graphcut_ek(const EnergyMap&) { throw std::logic_error("graph cut (EK) not implemented"); }
Seam find_seam_graphcut_dinic(const EnergyMap&) { throw std::logic_error("graph cut (Dinic) not implemented"); }

}  // namespace seam
