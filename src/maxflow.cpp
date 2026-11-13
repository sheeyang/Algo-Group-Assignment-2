#include "maxflow.h"

#include <algorithm>
#include <limits>
#include <queue>
#include <stdexcept>

namespace seam {

FlowGraph::FlowGraph(int n) {
    if (n < 0) throw std::invalid_argument("FlowGraph: negative node count");
    adj.resize(n);
}

static void check_node(const FlowGraph& g, int u) {
    if (u < 0 || size_t(u) >= g.adj.size())
        throw std::out_of_range("FlowGraph: node outside graph");
}

int FlowGraph::add_edge(int u, int v, long long cap) {
    check_node(*this, u);
    check_node(*this, v);
    if (cap < 0) throw std::invalid_argument("FlowGraph: negative capacity");
    if (edges.size() > size_t(std::numeric_limits<int>::max() - 2))
        throw std::overflow_error("FlowGraph: too many edges");
    const int i = static_cast<int>(edges.size());
    edges.push_back({v, cap});
    edges.push_back({u, 0});
    adj[u].push_back(i);
    adj[v].push_back(i ^ 1);
    return i;
}

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
            if (edge.cap > 0 && !seen[edge.to]) {
                seen[edge.to] = true;
                q.push(edge.to);
            }
        }
    }
    return seen;
}

long long edmonds_karp(FlowGraph& g, int s, int t) {
    check_node(g, s);
    check_node(g, t);
    if (s == t) throw std::invalid_argument("edmonds_karp: source equals sink");
    long long flow = 0;
    std::vector<int> parent(g.adj.size());
    while (true) {
        std::fill(parent.begin(), parent.end(), -1);
        std::queue<int> q;
        parent[s] = -2;
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
        if (parent[t] == -1) return flow;

        long long amount = std::numeric_limits<long long>::max();
        for (int v = t; v != s; v = g.edges[parent[v] ^ 1].to)
            amount = std::min(amount, g.edges[parent[v]].cap);
        if (amount > std::numeric_limits<long long>::max() - flow)
            throw std::overflow_error("edmonds_karp: flow exceeds long long");
        for (int v = t; v != s; v = g.edges[parent[v] ^ 1].to) {
            const int i = parent[v];
            g.edges[i].cap -= amount;
            g.edges[i ^ 1].cap += amount;
        }
        flow += amount;
    }
}

}  // namespace seam
