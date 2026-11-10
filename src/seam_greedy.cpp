#include <stdexcept>

#include "seam.h"

namespace seam {

// TODO(greedy owner): pick the cheapest top-row pixel, then at each next row the
// cheapest of the three reachable pixels (leftmost on ties).
Seam find_seam_greedy(const EnergyMap&) { throw std::logic_error("greedy not implemented"); }

}  // namespace seam
