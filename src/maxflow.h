#pragma once
#include <vector>

namespace seam {

// Directed flow network shared by Edmonds-Karp and Dinic. Nodes are numbered
// 0..n-1. Each added edge occupies two consecutive entries in edges: the
// forward edge and its reverse residual edge, whose initial capacity is zero.
// Flipping the lowest bit (i ^ 1) finds the paired edge in either direction.
struct FlowGraph {
    struct Edge {
        int to;         // destination node
        long long cap;  // remaining residual capacity; changed by the solver
    };

    // Create n isolated nodes. Throws invalid_argument for a negative count.
    explicit FlowGraph(int n);

    // Add u -> v with nonnegative capacity and a paired v -> u residual edge.
    // Return the forward edge's index. Parallel edges and self loops are allowed.
    // Throws out_of_range for invalid nodes, invalid_argument for a negative
    // capacity, or overflow_error if edge indices cannot fit in int.
    int add_edge(int u, int v, long long cap);

    std::vector<Edge> edges;
    std::vector<std::vector<int>> adj;  // outgoing edge indices for each node
};

// Push as much additional flow as possible from s to t using BFS augmenting
// paths. On a fresh graph, the return value is the maximum flow; calling again
// on the solved graph returns zero. Mutates g's residual capacities, so copy
// the graph before comparing solvers. After completion, reachable_from(g, s)
// gives the source side of a minimum cut.
// Throws out_of_range for invalid nodes, invalid_argument when s == t, and
// overflow_error if the returned flow would exceed long long. An overflow
// can leave earlier augmentations applied. Worst-case time: O(V * E^2).
long long edmonds_karp(FlowGraph& g, int s, int t);
long long dinic(FlowGraph& g, int s, int t);  // implemented by C4

// Return seen[v] == true exactly when v is reachable from s along edges with
// positive residual capacity. Does not modify g. This describes a minimum cut
// only after the flow solver has exhausted all s-to-t augmenting paths.
// Throws out_of_range for an invalid source. Time: O(V + E).
std::vector<bool> reachable_from(const FlowGraph& g, int s);

}  // namespace seam
