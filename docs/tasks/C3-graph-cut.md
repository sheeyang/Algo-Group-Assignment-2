# C3: Graph cut network + Edmonds-Karp

**Owner:** Zhi Kai  **Report partner:** R3 ([`docs/report/R3-graph-cut.md`](../report/R3-graph-cut.md))
**Teammate you unblock:** C4 (Dinic), who builds on your `FlowGraph` and seam network
**Assignment items:** 4 (formulation and Edmonds-Karp), plus the evidence for 5b

## Goal

Reformulate seam finding as a minimum s-t cut, build the flow network, solve it with **Edmonds-Karp**, and show it finds a seam of the same cost as DP. You also own the shared pieces (graph class, network, seam extraction) that C4 plugs Dinic into, so **publish them first**.

## Publish first (within 1 to 2 days, so C4 is not blocked)

Push a branch or PR with `src/maxflow.h` and the declaration of `find_seam_graphcut`, even if the bodies are stubs. Proposed contract (agree changes with C4):

```cpp
// src/maxflow.h
struct FlowGraph {
    explicit FlowGraph(int n);
    int add_edge(int u, int v, long long cap);  // also stores the paired reverse edge (cap 0); edge i and i^1 are a pair
    // compact arrays (to, cap, adjacency), NOT one heap allocation per edge
};
long long edmonds_karp(FlowGraph& g, int s, int t);      // C3 implements, returns the max-flow value
long long dinic(FlowGraph& g, int s, int t);             // C4 implements
std::vector<bool> reachable_from(const FlowGraph& g, int s);  // residual graph (cap > 0), C3 implements

// src/seam.h (add next to the existing finders)
using MaxFlowFn = long long (*)(FlowGraph&, int, int);
Seam find_seam_graphcut(const EnergyMap& e, MaxFlowFn maxflow);  // C3 implements; EK and Dinic both call it
```

`find_seam_graphcut_ek(e)` is then `find_seam_graphcut(e, edmonds_karp)`. C4 writes `find_seam_graphcut_dinic(e)` as `find_seam_graphcut(e, dinic)` in its own file.

## What to do

1. **`FlowGraph`** in `src/maxflow.h` / `src/maxflow.cpp`, independent of images so it can be tested on textbook graphs, plus **`edmonds_karp(s, t)`** (BFS shortest augmenting path) and **`reachable_from(s)`** (nodes reachable from `s` in the residual graph, needed for step 4).
2. **Flow network** from the brief (item 4) inside `find_seam_graphcut` in `src/seam_graphcut.cpp`, one node per pixel plus source and sink:
   - source to every pixel of the first column, every pixel of the last column to sink, both infinite
   - edge `(i,j) -> (i,j+1)` with capacity equal to the cost of removing pixel `(i,j)` (its energy)
   - infinite backward edge `(i,j+1) -> (i,j)`, and infinite diagonal edges `(i,j) -> (i-1,j-1)` and `(i,j) -> (i+1,j-1)`, so every finite cut crosses each row exactly once and the seam is connected
3. **Fill in `find_seam_graphcut_ek`** (stub in `seam_graphcut.cpp`). Horizontal seams are handled by the existing transpose in `resize.cpp`, so you only build the vertical network.
4. **Extract the seam:** run max-flow, call `reachable_from(source)` to get the source side S; in each row the **rightmost pixel in S** is the seam pixel.
5. **Watch these pitfalls** (decide, test and write down your answer for R3):
   - **Last column.** With the construction exactly as written, the cut edge in a row sits between `j` and `j+1`, so the seam can never use the last column, while DP can. Costs then disagree with DP on some inputs. Find a fix (for example a virtual column or an adjusted source/sink edge), explain it, and verify it.
   - **Capacities.** Energies are doubles. Either scale and round to integers (the contract uses `long long`) or use a tolerance. State which and why. Use integer-valued energies in the DP-equality tests so equality is exact.
   - **Ties.** Min cuts are not unique, so the seam may differ from DP's while the **cost** is equal. Compare costs, not positions ("Seam agreement" in `GLOSSARY.md`).
   - **Speed and memory.** Edmonds-Karp on a full image is very slow. Keep the graph compact (arrays) and limit EK benchmarks to small images.

## Deliverables

- `src/maxflow.*` (`FlowGraph`, `edmonds_karp`, `reachable_from`), `src/seam_graphcut.cpp`, `tests/test_maxflow.cpp` (EK part), `tests/test_graphcut.cpp` (EK part)
- Tests: Edmonds-Karp on small known graphs agrees with the known flow value; **EK graph-cut seam cost equals DP seam cost** on at least 200 random small maps; the seam is connected and in bounds
- A note for R3 that states the final network definition, the last-column fix, the capacity choice and the tie behaviour
- Seam images for DP vs graph cut on 2 to 3 test images (C4 and R4 reuse them)

## Acceptance checks

- `seamcli in.png out.png <w> <h> ek` produces a resized image of the right size
- The equality test passes on every random map, not most of them
- `maxflow.h` and `find_seam_graphcut` are on a branch C4 can use **before** your EK is finished
- `ctest` green; no warnings

## Dependencies

Only `seam.h` / `energy.h` (exist now). No need to wait for C1 or C2. C4 depends on you.
