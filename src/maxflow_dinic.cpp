#include "maxflow.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

namespace seam {

namespace {

// Reuse the same node checks as the rest of maxflow so Dinic rejects the same
// bad arguments as edmonds_karp. Kept local because maxflow.cpp's copy is
// static; duplicating two lines is cheaper than widening the public surface.
void check_node(const FlowGraph& g, int u) {
    if (u < 0 || size_t(u) >= g.adj.size())
        throw std::out_of_range("FlowGraph: node outside graph");
}

// Build the level graph by BFS from s over edges with positive residual
// capacity. level[v] is the fewest such edges from s to v, or -1 if v is
// unreachable. Returns true when t became reachable, i.e. another phase is
// possible.
bool build_levels(const FlowGraph& g, int s, int t, std::vector<int>& level,
                  std::vector<int>& bfs_queue) {
    std::fill(level.begin(), level.end(), -1);
    level[s] = 0;
    int head = 0, tail = 0;
    bfs_queue[tail++] = s;
    while (head < tail) {
        const int u = bfs_queue[head++];
        for (int i : g.adj[u]) {
            const auto& edge = g.edges[i];
            if (edge.cap > 0 && level[edge.to] == -1) {
                level[edge.to] = level[u] + 1;
                bfs_queue[tail++] = edge.to;
            }
        }
    }
    return level[t] != -1;
}

}  // namespace

// Dinic's algorithm. Each phase builds one level graph (build_levels) and then
// pushes a blocking flow along edges that advance exactly one level, using a
// per-node current-arc pointer (iter) so saturated or dead edges are never
// rescanned within the phase. The number of phases is at most V - 1, which is
// why Dinic beats Edmonds-Karp on these layered seam networks.
//
// The DFS is written iteratively with an explicit stack: a vertical seam path
// can be as long as the image is tall, so a recursive DFS would risk a stack
// overflow on a real image (see docs/tasks/C4-dinic.md).
long long dinic(FlowGraph& g, int s, int t) {
    check_node(g, s);
    check_node(g, t);
    if (s == t) throw std::invalid_argument("dinic: source equals sink");

    const int n = static_cast<int>(g.adj.size());
    long long flow = 0;
    std::vector<int> level(n);
    std::vector<int> bfs_queue(n);
    std::vector<int> iter(n);   // current-arc pointer: next adj index to try
    std::vector<int> path;      // edge indices forming the current s->... path
    path.reserve(n);

    while (build_levels(g, s, t, level, bfs_queue)) {
        std::fill(iter.begin(), iter.end(), 0);
        path.clear();
        int u = s;
        // One pass of this loop either extends the path forward, pushes flow
        // when it reaches t, or retreats after exhausting a node's arcs. The
        // phase ends when we retreat past the source.
        while (true) {
            if (u == t) {
                // Found an augmenting path within the level graph. Its
                // bottleneck is the smallest residual capacity along it.
                long long amount = std::numeric_limits<long long>::max();
                for (int i : path) amount = std::min(amount, g.edges[i].cap);
                if (amount > std::numeric_limits<long long>::max() - flow)
                    throw std::overflow_error("dinic: flow exceeds long long");
                // Push the bottleneck, and when an edge saturates, roll the
                // path back to that edge's tail so its arc is retried/skipped.
                int cut_to = static_cast<int>(path.size());
                for (int k = 0; k < static_cast<int>(path.size()); ++k) {
                    const int i = path[k];
                    g.edges[i].cap -= amount;
                    g.edges[i ^ 1].cap += amount;
                    if (g.edges[i].cap == 0 && k < cut_to) cut_to = k;
                }
                flow += amount;
                // Resume the DFS from the tail of the first saturated edge.
                path.resize(cut_to);
                u = (cut_to == 0) ? s : g.edges[path.back()].to;
                continue;
            }
            // Advance the current-arc pointer to the next usable edge: one
            // with residual capacity that steps to the next level.
            bool advanced = false;
            while (iter[u] < static_cast<int>(g.adj[u].size())) {
                const int i = g.adj[u][iter[u]];
                const auto& edge = g.edges[i];
                if (edge.cap > 0 && level[edge.to] == level[u] + 1) {
                    path.push_back(i);
                    u = edge.to;
                    advanced = true;
                    break;
                }
                ++iter[u];
            }
            if (advanced) continue;
            // Dead end: no more useful arcs from u. Mark it so this phase does
            // not re-enter it, then retreat along the path.
            level[u] = -1;
            if (path.empty()) break;  // retreated past the source: phase done
            ++iter[g.edges[path.back() ^ 1].to];  // skip the arc we came in on
            path.pop_back();
            u = path.empty() ? s : g.edges[path.back()].to;
        }
    }
    return flow;
}

}  // namespace seam
