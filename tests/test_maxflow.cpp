#include <limits>
#include <stdexcept>

#include "doctest.h"
#include "maxflow.h"

using namespace seam;

TEST_CASE("FlowGraph stores paired residual edges") {
    FlowGraph g(2);
    const int i = g.add_edge(0, 1, 7);
    CHECK(g.edges[i].to == 1);
    CHECK(g.edges[i].cap == 7);
    CHECK(g.edges[i ^ 1].to == 0);
    CHECK(g.edges[i ^ 1].cap == 0);
    CHECK(g.adj[0] == std::vector<int>{i});
    CHECK(g.adj[1] == std::vector<int>{i ^ 1});
}

TEST_CASE("Edmonds-Karp solves the textbook six-node network") {
    FlowGraph g(6);
    g.add_edge(0, 1, 16);
    g.add_edge(0, 2, 13);
    g.add_edge(1, 2, 10);
    g.add_edge(2, 1, 4);
    g.add_edge(1, 3, 12);
    g.add_edge(3, 2, 9);
    g.add_edge(2, 4, 14);
    g.add_edge(4, 3, 7);
    g.add_edge(3, 5, 20);
    g.add_edge(4, 5, 4);
    const FlowGraph original = g;
    CHECK(edmonds_karp(g, 0, 5) == 23);
    const auto seen = reachable_from(g, 0);
    CHECK_FALSE(seen[5]);
    long long cut = 0;
    for (size_t i = 0; i < original.edges.size(); i += 2) {
        const int from = original.edges[i ^ 1].to;
        if (seen[from] && !seen[original.edges[i].to]) cut += original.edges[i].cap;
    }
    CHECK(cut == 23);
    CHECK(edmonds_karp(g, 0, 5) == 0);  // no additional augmenting path
}

TEST_CASE("Edmonds-Karp can cancel an earlier flow choice") {
    // First BFS chooses s-a-c-t. A second unit requires s-b-c-a-d-t,
    // using the reverse residual edge c-a to undo a-c.
    FlowGraph g(6);
    g.add_edge(0, 1, 1);
    g.add_edge(0, 2, 1);
    const int ac = g.add_edge(1, 3, 1);
    g.add_edge(1, 4, 1);
    g.add_edge(2, 3, 1);
    g.add_edge(3, 5, 1);
    g.add_edge(4, 5, 1);
    CHECK(edmonds_karp(g, 0, 5) == 2);
    CHECK(g.edges[ac].cap == 1);
}

TEST_CASE("Edmonds-Karp handles parallel edges, self loops and zero capacities") {
    FlowGraph g(3);
    g.add_edge(0, 0, 100);
    g.add_edge(0, 1, 2);
    g.add_edge(0, 1, 3);
    g.add_edge(0, 2, 0);
    g.add_edge(1, 2, 4);
    CHECK(edmonds_karp(g, 0, 2) == 4);
}

TEST_CASE("Edmonds-Karp returns zero for an unreachable sink") {
    FlowGraph g(3);
    g.add_edge(0, 1, 9);
    CHECK(edmonds_karp(g, 0, 2) == 0);
    CHECK(reachable_from(g, 0) == std::vector<bool>{true, true, false});
}

TEST_CASE("Flow graph rejects invalid arguments and flow overflow") {
    CHECK_THROWS_AS(FlowGraph(-1), std::invalid_argument);
    FlowGraph g(2);
    CHECK_THROWS_AS(g.add_edge(-1, 1, 1), std::out_of_range);
    CHECK_THROWS_AS(g.add_edge(0, 2, 1), std::out_of_range);
    CHECK_THROWS_AS(g.add_edge(0, 1, -1), std::invalid_argument);
    CHECK_THROWS_AS(edmonds_karp(g, 0, 0), std::invalid_argument);
    CHECK_THROWS_AS(reachable_from(g, 2), std::out_of_range);
    g.add_edge(0, 1, std::numeric_limits<long long>::max());
    g.add_edge(0, 1, 1);
    CHECK_THROWS_AS(edmonds_karp(g, 0, 1), std::overflow_error);
}
