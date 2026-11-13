#pragma once
#include <vector>

namespace seam {

// cap is the remaining (residual) capacity, mutated by the flow solver.
// edges[i ^ 1] is the reverse of edges[i], initially with zero capacity.
struct FlowGraph {
    struct Edge {
        int to;
        long long cap;
    };

    explicit FlowGraph(int n);
    int add_edge(int u, int v, long long cap);

    std::vector<Edge> edges;
    std::vector<std::vector<int>> adj;  // outgoing edge indices for each node
};

// Solvers consume residual capacities; copy the graph to compare solvers.
long long edmonds_karp(FlowGraph& g, int s, int t);
long long dinic(FlowGraph& g, int s, int t);  // implemented by C4
std::vector<bool> reachable_from(const FlowGraph& g, int s);

}  // namespace seam
