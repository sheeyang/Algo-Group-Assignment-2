#include <stdexcept>

#include "seam.h"

namespace seam {

// TODO(C4 owner): once C3 publishes find_seam_graphcut(e, maxflow_fn), this becomes a one-line
// call with dinic(). The Dinic max-flow itself goes in maxflow_dinic.cpp.
// See docs/tasks/C4-dinic.md.
Seam find_seam_graphcut_dinic(const EnergyMap&) { throw std::logic_error("graph cut (Dinic) not implemented"); }

}  // namespace seam
