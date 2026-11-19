/**
 * @file maxflow.cpp
 * @brief C3 residual-graph operations and Edmonds-Karp implementation.
 * @details BFS selects shortest augmenting paths by edge count, not energy.
 * Bottleneck pushes preserve capacity/flow constraints. When BFS cannot reach
 * the sink, residual reachability exposes a minimum cut for seam extraction.
 */
#include "maxflow.h"

#include <algorithm>
#include <limits>
#include <queue>
#include <stdexcept>

namespace seam {

/** @brief Initialise adjacency storage after validating the node count. */
FlowGraph::FlowGraph(int n) {
    if (n < 0) throw std::invalid_argument("FlowGraph: negative node count");
    adj.resize(n);
}

/**
 * @brief Validate a node index before accessing adjacency storage.
 * @param g Graph that supplies the valid node range.
 * @param u Node index to validate.
 * @throws std::out_of_range If u is negative or outside the graph.
 */
static void check_node(const FlowGraph& g, int u) {
    if (u < 0 || size_t(u) >= g.adj.size())
        throw std::out_of_range("FlowGraph: node outside graph");
}

/** @brief Append an original/reverse pair and register both adjacency indices. */
int FlowGraph::add_edge(int u, int v, long long cap) {
    check_node(*this, u);
    check_node(*this, v);
    if (cap < 0) throw std::invalid_argument("FlowGraph: negative capacity");
    if (edges.size() > size_t(std::numeric_limits<int>::max() - 2))
        throw std::overflow_error("FlowGraph: too many edges");
    const int i = static_cast<int>(edges.size());
    // The reverse edge records how much of this edge's flow can be undone.
    // Keep each pair together so both solvers can use i ^ 1 to update it.
    edges.push_back({v, cap});
    edges.push_back({u, 0});
    adj[u].push_back(i);
    adj[v].push_back(i ^ 1);
    return i;
}

/** @brief Mark all nodes accessible through positive residual capacities. */
std::vector<bool> reachable_from(const FlowGraph& g, int s) {
    check_node(g, s);
    std::vector<bool> seen(g.adj.size(), false);
    std::queue<int> q;
    seen[s] = true;
    q.push(s);
    while (!q.empty()) {
        const int u = q.front();
        q.pop();
        for (int i : g.adj[u]) {
            const auto& edge = g.edges[i];
            // Saturated edges cannot carry more flow and cannot be traversed.
            if (edge.cap > 0 && !seen[edge.to]) {
                seen[edge.to] = true;
                q.push(edge.to);
            }
        }
    }
    return seen;
}

/** @brief Run the measured overload with statistics disabled. */
long long edmonds_karp(FlowGraph& g, int s, int t) {
    return edmonds_karp(g, s, t, nullptr);
}

/** @brief Augment shortest residual paths until no additional flow is possible. */
long long edmonds_karp(FlowGraph& g, int s, int t, MaxFlowStats* stats) {
    check_node(g, s);
    check_node(g, t);
    if (s == t) throw std::invalid_argument("edmonds_karp: source equals sink");
    if (stats) *stats = {};
    long long flow = 0;
    // parent[v] stores the incoming EDGE index, not the previous node. This
    // distinguishes parallel edges and lets us update the exact edge used.
    std::vector<int> parent(g.adj.size());
    while (true) {
        std::fill(parent.begin(), parent.end(), -1);
        std::queue<int> q;
        parent[s] = -2;  // mark the source visited without giving it a parent
        q.push(s);
        // BFS chooses an augmenting path with the fewest edges.
        while (!q.empty() && parent[t] == -1) {
            const int u = q.front();
            q.pop();
            for (int i : g.adj[u]) {
                const auto& edge = g.edges[i];
                if (edge.cap > 0 && parent[edge.to] == -1) {
                    parent[edge.to] = i;
                    q.push(edge.to);
                }
            }
        }
        // No augmenting path remains: the current flow is maximal. The nodes
        // still reachable from s now define a minimum s-t cut.
        if (parent[t] == -1) return flow;

        // Walk backwards from t. The reverse edge's destination is the
        // previous node; the smallest capacity is this path's bottleneck.
        long long amount = std::numeric_limits<long long>::max();
        for (int v = t; v != s; v = g.edges[parent[v] ^ 1].to)
            amount = std::min(amount, g.edges[parent[v]].cap);
        if (amount > std::numeric_limits<long long>::max() - flow)
            throw std::overflow_error("edmonds_karp: flow exceeds long long");
        // Send the bottleneck amount along the whole path. Increasing reverse
        // capacity allows a later path to cancel or reroute this choice.
        for (int v = t; v != s; v = g.edges[parent[v] ^ 1].to) {
            const int i = parent[v];
            g.edges[i].cap -= amount;
            g.edges[i ^ 1].cap += amount;
        }
        flow += amount;
        if (stats) ++stats->augmentations;
    }
}

}  // namespace seam
