/**
 * @brief Known-flow, random-reference and iterative-depth tests for Dinic.
 * @file test_dinic.cpp
 */
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

#include "doctest.h"
#include "maxflow.h"
#include "seam.h"

using namespace seam;

// ---------------------------------------------------------------------------
// Dinic on textbook flow networks
// ---------------------------------------------------------------------------

/** @test Dinic solves the textbook six-node network. */
TEST_CASE("Dinic solves the textbook six-node network") {
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
    CHECK(dinic(g, 0, 5) == 23);
    // The residual graph must describe the same min cut EK would leave.
    const auto seen = reachable_from(g, 0);
    CHECK_FALSE(seen[5]);
    long long cut = 0;
    for (size_t i = 0; i < original.edges.size(); i += 2) {
        const int from = original.edges[i ^ 1].to;
        if (seen[from] && !seen[original.edges[i].to]) cut += original.edges[i].cap;
    }
    CHECK(cut == 23);
    CHECK(dinic(g, 0, 5) == 0);  // already saturated: no augmenting path
}

/** @test Dinic can cancel an earlier flow choice. */
TEST_CASE("Dinic can cancel an earlier flow choice") {
    FlowGraph g(6);
    g.add_edge(0, 1, 1);
    g.add_edge(0, 2, 1);
    const int ac = g.add_edge(1, 3, 1);
    g.add_edge(1, 4, 1);
    g.add_edge(2, 3, 1);
    g.add_edge(3, 5, 1);
    g.add_edge(4, 5, 1);
    CHECK(dinic(g, 0, 5) == 2);
    CHECK(g.edges[ac].cap == 1);
}

/** @test Dinic handles parallel edges, self loops and zero capacities. */
TEST_CASE("Dinic handles parallel edges, self loops and zero capacities") {
    FlowGraph g(3);
    g.add_edge(0, 0, 100);
    g.add_edge(0, 1, 2);
    g.add_edge(0, 1, 3);
    g.add_edge(0, 2, 0);
    g.add_edge(1, 2, 4);
    CHECK(dinic(g, 0, 2) == 4);
}

/** @test Dinic returns zero for an unreachable sink. */
TEST_CASE("Dinic returns zero for an unreachable sink") {
    FlowGraph g(3);
    g.add_edge(0, 1, 9);
    CHECK(dinic(g, 0, 2) == 0);
    CHECK(reachable_from(g, 0) == std::vector<bool>{true, true, false});
}

/** @test Dinic measures level phases separately from path augmentations. */
TEST_CASE("Dinic measures level phases separately from path augmentations") {
    FlowGraph g(4);
    g.add_edge(0, 3, 1);  // first phase: the direct path
    g.add_edge(0, 1, 2);
    g.add_edge(1, 3, 2);  // second phase: a longer path
    g.add_edge(0, 2, 3);
    g.add_edge(2, 3, 3);  // same second phase, another augmentation
    FlowGraph reference = g;
    MaxFlowStats stats;
    CHECK(dinic(g, 0, 3, &stats) == 6);
    CHECK(stats.phases == 2);
    CHECK(stats.augmentations == 3);
    CHECK(dinic(reference, 0, 3) == 6);
    for (size_t i = 0; i < g.edges.size(); ++i)
        CHECK(g.edges[i].cap == reference.edges[i].cap);
    CHECK(dinic(g, 0, 3, &stats) == 0);
    CHECK(stats.phases == 0);
    CHECK(stats.augmentations == 0);
}

/** @test Dinic traverses a 100000-node path without recursive stack growth. */
TEST_CASE("Dinic traverses a 100000-node path without recursive stack growth") {
    const int nodes = 100000;
    FlowGraph g(nodes);
    for (int i = 0; i + 1 < nodes; ++i) g.add_edge(i, i + 1, 1);
    MaxFlowStats stats;
    CHECK(dinic(g, 0, nodes - 1, &stats) == 1);
    CHECK(stats.phases == 1);
    CHECK(stats.augmentations == 1);
}

/** @test Dinic rejects invalid arguments and flow overflow. */
TEST_CASE("Dinic rejects invalid arguments and flow overflow") {
    FlowGraph g(2);
    CHECK_THROWS_AS(dinic(g, -1, 1), std::out_of_range);
    CHECK_THROWS_AS(dinic(g, 0, 2), std::out_of_range);
    CHECK_THROWS_AS(dinic(g, 0, 0), std::invalid_argument);
    g.add_edge(0, 1, std::numeric_limits<long long>::max());
    g.add_edge(0, 1, 1);
    CHECK_THROWS_AS(dinic(g, 0, 1), std::overflow_error);
}

// ---------------------------------------------------------------------------
// Dinic == Edmonds-Karp on random graphs (both mutate, so copy first)
// ---------------------------------------------------------------------------

/** @test Dinic matches Edmonds-Karp on 300 random graphs. */
TEST_CASE("Dinic matches Edmonds-Karp on 300 random graphs") {
    std::mt19937 rng(20261113);
    for (int trial = 0; trial < 300; ++trial) {
        const int n = 2 + int(rng() % 7);
        FlowGraph g(n);
        const int edges = int(rng() % (n * n));
        for (int k = 0; k < edges; ++k) {
            const int u = int(rng() % n), v = int(rng() % n);
            g.add_edge(u, v, int(rng() % 20));
        }
        const int s = int(rng() % n);
        int t = int(rng() % n);
        if (t == s) t = (t + 1) % n;
        FlowGraph a = g, b = g;  // copy: both solvers mutate residual capacities
        INFO("trial=" << trial << " n=" << n << " s=" << s << " t=" << t);
        CHECK(dinic(a, s, t) == edmonds_karp(b, s, t));
    }
}

// ---------------------------------------------------------------------------
// Dinic graph-cut seam cost == DP seam cost
// ---------------------------------------------------------------------------

/**
 * @brief Assert Dinic seam validity and minimum-cost agreement with DP.
 * @param e Input row-major energy map.
 */
static void check_dinic_against_dp(const EnergyMap& e) {
    const Seam s = find_seam_graphcut_dinic(e);
    REQUIRE(s.size() == size_t(e.height));
    for (int y = 0; y < e.height; ++y) {
        REQUIRE(s[y] >= 0);
        REQUIRE(s[y] < e.width);
        if (y > 0) CHECK(std::abs(s[y] - s[y - 1]) <= 1);  // connected seam
    }
    CHECK(seam_cost(e, s) == doctest::Approx(seam_cost(e, find_seam_dp(e))));
}

/** @test Dinic graph cut finds the hand-checked seam. */
TEST_CASE("Dinic graph cut finds the hand-checked seam") {
    EnergyMap e{3, 3, {5, 1, 0, 1, 9, 9, 5, 1, 9}};
    check_dinic_against_dp(e);
}

/** @test Dinic graph cut handles tiny maps, zeros and ties. */
TEST_CASE("Dinic graph cut handles tiny maps, zeros and ties") {
    check_dinic_against_dp(EnergyMap{1, 1, {42}});
    check_dinic_against_dp(EnergyMap{1, 4, {7, 3, 9, 1}});
    check_dinic_against_dp(EnergyMap{4, 1, {9, 8, 7, 0}});
    check_dinic_against_dp(EnergyMap{3, 3, std::vector<double>(9, 0)});
    check_dinic_against_dp(EnergyMap{4, 4, std::vector<double>(16, 5)});
}

/** @test Dinic graph cut agrees with DP on 300 random integer energy maps. */
TEST_CASE("Dinic graph cut agrees with DP on 300 random integer energy maps") {
    std::mt19937 rng(777);
    for (int trial = 0; trial < 300; ++trial) {
        const int w = 1 + int(rng() % 7), h = 1 + int(rng() % 7);
        EnergyMap e{w, h, std::vector<double>(size_t(w) * h)};
        for (double& v : e.e) v = double(rng() % (trial % 2 ? 30 : 3));
        INFO("trial=" << trial << " width=" << w << " height=" << h);
        check_dinic_against_dp(e);
    }
}

// ---------------------------------------------------------------------------
// Large case: proves the iterative DFS does not overflow the stack
// ---------------------------------------------------------------------------

/** @test Dinic graph cut finishes on an image-sized map without stack overflow. */
TEST_CASE("Dinic graph cut finishes on an image-sized map without stack overflow") {
    const int w = 200, h = 200;
    std::mt19937 rng(2024);
    EnergyMap e{w, h, std::vector<double>(size_t(w) * h)};
    for (double& v : e.e) v = double(rng() % 50);
    const Seam s = find_seam_graphcut_dinic(e);
    REQUIRE(s.size() == size_t(h));
    for (int y = 0; y < h; ++y) {
        REQUIRE(s[y] >= 0);
        REQUIRE(s[y] < w);
        if (y > 0) CHECK(std::abs(s[y] - s[y - 1]) <= 1);
    }
    CHECK(seam_cost(e, s) == doctest::Approx(seam_cost(e, find_seam_dp(e))));
}
