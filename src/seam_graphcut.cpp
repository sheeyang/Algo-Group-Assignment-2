#include <stdexcept>

#include "seam.h"

namespace seam {

// TODO(C3 owner): build the flow network from the brief (item 4), run
// Edmonds-Karp, BFS the residual graph from the source, and take the
// rightmost source-side pixel per row.
// Suggested shape (see docs/tasks/C3-graph-cut.md): put the shared network building and seam
// extraction in find_seam_graphcut(e, maxflow_fn) so C4 can reuse it with Dinic.
// find_seam_graphcut_dinic lives in seam_graphcut_dinic.cpp (owned by C4).
Seam find_seam_graphcut_ek(const EnergyMap&) { throw std::logic_error("graph cut (EK) not implemented"); }

}  // namespace seam
