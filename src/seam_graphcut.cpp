#include <cmath>
#include <limits>
#include <stdexcept>

#include "seam.h"

namespace seam {

// Convert seam selection to a minimum cut, solve it, then read the seam from
// the residual graph. The solver is injected so C4 can reuse this with Dinic.
Seam find_seam_graphcut(const EnergyMap& e, MaxFlowFn maxflow) {
    const int w = e.width, h = e.height;
    if (w < 1 || h < 1) throw std::invalid_argument("graph cut: empty energy map");
    if (e.e.size() != size_t(w) * h)
        throw std::invalid_argument("graph cut: energy map size mismatch");
    if (!maxflow) throw std::invalid_argument("graph cut: missing flow solver");
    // Each row has w real nodes plus a virtual node to allow the last pixel
    // to be selected. Leave two node indices for source and sink.
    const long long nodes = (static_cast<long long>(w) + 1) * h;
    if (nodes > std::numeric_limits<int>::max() - 2)
        throw std::overflow_error("graph cut: too many nodes");

    // Integer capacities avoid epsilon decisions in residual BFS. A scale of
    // 1000 retains thousandths (the precision of the grayscale coefficients),
    // but can change the preferred seam for arbitrary finer-precision inputs.
    constexpr long double scale = 1000.0L;
    constexpr long long capacity_limit = std::numeric_limits<long long>::max() / 4;
    std::vector<long long> capacities;
    capacities.reserve(e.e.size());
    long long sum = 0;
    for (double energy : e.e) {
        if (!std::isfinite(energy) || energy < 0)
            throw std::invalid_argument("graph cut: energy must be finite and nonnegative");
        const long double rounded = std::round(static_cast<long double>(energy) * scale);
        if (rounded >= static_cast<long double>(capacity_limit))
            throw std::overflow_error("graph cut: energy exceeds capacity range");
        const long long cap = static_cast<long long>(rounded);
        if (cap > capacity_limit - 1 - sum)
            throw std::overflow_error("graph cut: total capacity exceeds supported range");
        capacities.push_back(cap);
        sum += cap;
    }
    // A finite seam costs at most sum, so no minimum cut crosses an INF edge.
    const long long inf = sum + 1;
    const int source = static_cast<int>(nodes), sink = source + 1;
    FlowGraph g(sink + 1);
    // Flatten row-major coordinates, including the extra column x == w.
    const auto id = [w](int x, int y) { return y * (w + 1) + x; };
    for (int y = 0; y < h; ++y) {
        // Force the first real node onto the source side and the virtual node
        // onto the sink side. Every row must therefore contain a cut edge.
        g.add_edge(source, id(0, y), inf);
        g.add_edge(id(w, y), sink, inf);
        // Cutting x -> x+1 selects pixel x and pays its scaled energy. The
        // final edge into the virtual column allows selecting pixel w-1.
        for (int x = 0; x < w; ++x)
            g.add_edge(id(x, y), id(x + 1, y), capacities[size_t(y) * w + x]);
        for (int x = 1; x <= w; ++x) {
            // An INF backward edge forbids a source-side node with a sink-side
            // node to its left. Thus each row's source side is a single prefix.
            // These constraints are separate from the zero-capacity reverse
            // residual edges automatically created by add_edge().
            g.add_edge(id(x, y), id(x - 1, y), inf);
            // If (x,y) is on the source side, its neighbours one column left
            // must be too. Together, the two directions between adjacent rows
            // force their prefix endpoints to differ by at most one column.
            if (y > 0) g.add_edge(id(x, y), id(x - 1, y - 1), inf);
            if (y + 1 < h) g.add_edge(id(x, y), id(x - 1, y + 1), inf);
        }
    }

    // Max-flow/min-cut makes the minimum cut's capacity equal to the optimal
    // scaled seam cost. Reachability identifies its source side; no INF edge
    // crosses it, so each row has exactly one finite cut edge.
    maxflow(g, source, sink);
    const auto seen = reachable_from(g, source);
    Seam result(h, 0);
    // Scan only real pixels. The last reachable node is the left endpoint of
    // that row's cut edge, which is the pixel to remove.
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            if (seen[id(x, y)]) result[y] = x;
    return result;
}

// Select Edmonds-Karp while keeping network construction and extraction shared.
Seam find_seam_graphcut_ek(const EnergyMap& e) {
    return find_seam_graphcut(e, edmonds_karp);
}

}  // namespace seam
