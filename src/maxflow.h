/**
 * @file maxflow.h
 * @brief Image-independent residual network and shared max-flow interfaces.
 * @details C3 supplies FlowGraph, Edmonds-Karp and residual reachability.
 * C4 plugs Dinic into the same graph and seam-network contract.
 */
#pragma once
#include <cstdint>
#include <vector>

namespace seam {

/**
 * @brief Directed graph whose edge capacities represent remaining flow capacity.
 * @invariant Nodes are numbered from zero to adj.size()-1. Every original edge
 * is immediately followed by its reverse residual edge; index i ^ 1 selects
 * the partner in either direction. Reverse capacity initially equals zero.
 * @warning Solvers mutate capacities. Copy an unsolved graph before comparing
 * solvers, and preserve the pairing invariant when accessing public storage.
 * @par Complexity
 * Storage is O(V+E), including the paired reverse records.
 */
struct FlowGraph {
    /** @brief One directed residual edge; its source is implicit in adjacency. */
    struct Edge {
        int to;         ///< Destination node index.
        long long cap;  ///< Remaining capacity, updated by the flow solver.
    };

    /**
     * @brief Construct a graph containing isolated nodes.
     * @param n Number of nodes, including any source and sink.
     * @throws std::invalid_argument If n is negative.
     * @par Complexity
     * O(n) time and storage.
     */
    explicit FlowGraph(int n);

    /**
     * @brief Add an original edge and its zero-capacity reverse residual edge.
     * @param u Source node index.
     * @param v Destination node index. Self loops and parallel edges are allowed.
     * @param cap Nonnegative original capacity.
     * @return Index of the original edge in edges; its partner is index ^ 1.
     * @throws std::out_of_range If either node does not exist.
     * @throws std::invalid_argument If cap is negative.
     * @throws std::overflow_error If edge indices cannot fit in int.
     * @par Complexity
     * Amortised O(1) time and additional storage per call.
     */
    int add_edge(int u, int v, long long cap);

    std::vector<Edge> edges;  ///< Original/reverse residual records in pairs.
    std::vector<std::vector<int>> adj;  ///< Outgoing edge indices for each node.
};

/**
 * @brief Compute additional maximum flow using BFS augmenting paths.
 * @param[in,out] g Residual network; must retain FlowGraph's pairing invariant.
 * @param s Source node index.
 * @param t Sink node index, distinct from s.
 * @return Additional flow sent during this call. For a fresh network this is
 * the maximum flow; a second call on the solved network returns zero.
 * @post On normal return, no residual source-to-sink path remains.
 * @throws std::out_of_range If a terminal node does not exist.
 * @throws std::invalid_argument If s equals t.
 * @throws std::overflow_error If the flow total cannot fit in long long;
 * augmentations completed before the exception remain applied.
 * @par Complexity
 * O(V E^2) worst-case time and O(V) auxiliary storage, excluding the graph.
 * @see reachable_from
 */
long long edmonds_karp(FlowGraph& g, int s, int t);
/**
 * @brief Compute additional flow with level graphs and iterative blocking-flow paths.
 * @param g Residual graph; mutable references are updated by the solver.
 * @param s Source node index.
 * @param t Sink node index.
 * @return Additional flow sent during this call, zero for an already solved network.
 * @throws std::out_of_range For invalid terminals.
 * @throws std::invalid_argument If source equals sink.
 * @throws std::overflow_error If returned flow cannot fit in long long; earlier pushes remain
 * applied.
 * @note The graph is mutated. Optional counters reset after argument validation.
 * @par Complexity
 * O(V^2 E) worst-case time and O(V) auxiliary storage; DFS uses an explicit stack.
 */
long long dinic(FlowGraph& g, int s, int t);  // implemented by C4

/**
 * @brief Optional per-call solver measurements shared with C4.
 * @details Counters reset after terminal validation. A Dinic phase is one
 * successful level-graph BFS followed by blocking flow; its final failed BFS
 * is not a phase. EK leaves phases at zero. Augmentations count path pushes.
 */
struct MaxFlowStats {
    std::uint64_t phases = 0;  ///< Successful Dinic blocking-flow phases.
    std::uint64_t augmentations = 0;  ///< Individual source-to-sink flow pushes.
};
/**
 * @brief Run Edmonds-Karp with optional augmentation counting.
 * @param[in,out] g Residual graph, mutated as in the three-argument overload.
 * @param s Source node index.
 * @param t Sink node index, distinct from s.
 * @param[out] stats Optional counters; nullptr disables measurement. Resets
 * after successful argument validation; partial counts remain on overflow.
 * @return Additional flow sent during this call.
 * @throws std::out_of_range If a terminal node does not exist.
 * @throws std::invalid_argument If s equals t.
 * @throws std::overflow_error If the flow total exceeds long long.
 * @see edmonds_karp(FlowGraph&, int, int)
 */
long long edmonds_karp(FlowGraph& g, int s, int t, MaxFlowStats* stats);
/**
 * @brief Compute additional flow with level graphs and iterative blocking-flow paths.
 * @param g Residual graph; mutable references are updated by the solver.
 * @param s Source node index.
 * @param t Sink node index.
 * @param stats Optional reset per-call solver counters.
 * @return Additional flow sent during this call, zero for an already solved network.
 * @throws std::out_of_range For invalid terminals.
 * @throws std::invalid_argument If source equals sink.
 * @throws std::overflow_error If returned flow cannot fit in long long; earlier pushes remain
 * applied.
 * @note The graph is mutated. Optional counters reset after argument validation.
 * @par Complexity
 * O(V^2 E) worst-case time and O(V) auxiliary storage; DFS uses an explicit stack.
 */
long long dinic(FlowGraph& g, int s, int t, MaxFlowStats* stats);

/**
 * @brief Traverse the residual graph through strictly positive capacities.
 * @param[in] g Residual network; it is not modified.
 * @param s Starting node index.
 * @return One flag per node: true if reachable from s, including s itself.
 * @throws std::out_of_range If s does not exist.
 * @note This set is the source side of a minimum cut only after max-flow has
 * exhausted all augmenting paths to the chosen sink.
 * @par Complexity
 * O(V+E) time and O(V) auxiliary storage.
 */
std::vector<bool> reachable_from(const FlowGraph& g, int s);

}  // namespace seam
