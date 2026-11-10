# C4: Dinic max-flow + EK vs Dinic evidence

**Owner:** Dayton  **Report partner:** R4 ([`docs/report/R4-dinic-graphcut-vs-dp.md`](../report/R4-dinic-graphcut-vs-dp.md))
**Builds on:** C3 (`FlowGraph`, `find_seam_graphcut`)
**Assignment items:** 4 (Dinic as the faster solver), plus the evidence for 5b and 5c (graph cut)

## Goal

Implement **Dinic's algorithm** on the same `FlowGraph` that C3 builds, plug it into the seam network, prove it gives the same answers as Edmonds-Karp and DP, and measure how much faster it is. The brief names Ford-Fulkerson and Edmonds-Karp; Dinic is our optimisation and the main material for the "EK vs Dinic" comparison.

## Start without waiting

C3 publishes `src/maxflow.h` and `find_seam_graphcut` within 1 to 2 days (contract in [`C3-graph-cut.md`](C3-graph-cut.md)). Until then, write Dinic against that contract, on your own small test graphs, in new files, so you only need to merge when C3's header lands. If you need a change to the contract, agree it with Zhi Kai first.

## What to do

1. **`dinic(FlowGraph& g, int s, int t)`** in `src/maxflow_dinic.cpp`: repeatedly build the **level graph** by BFS from `s`, then push a **blocking flow** with DFS along edges that go from level `d` to `d+1`, using a per-node **current-arc pointer** so dead edges are not rescanned. Return the max-flow value.
2. **No deep recursion.** A path in the pixel graph can be as long as the image is tall or wide, so a recursive DFS can overflow the stack on a large image. Write the DFS iteratively (explicit stack) or prove a recursion depth bound and test a big image.
3. **`find_seam_graphcut_dinic`** in `src/seam_graphcut_dinic.cpp` (stub exists): `return find_seam_graphcut(e, dinic);` once C3's function is in.
4. **Tests** in `tests/test_dinic.cpp`:
   - small textbook graphs with known max-flow (CLRS-style networks, a graph with parallel edges, a disconnected sink with flow 0)
   - **random graphs: `dinic` returns the same flow value as `edmonds_karp`** (at least 200 graphs; copy the graph, because both mutate it)
   - **seam cost via Dinic equals seam cost via DP** on at least 200 random small energy maps (same test shape as C3's EK test; use integer-valued energies)
   - one larger image-sized case to show it finishes and does not overflow the stack
5. **Benchmark EK vs Dinic vs DP** and write `results/maxflow_*.csv`: time per seam for several image sizes (for example 32x32 up to what EK can still finish in seconds, and larger sizes for Dinic and DP only). Use C2's `run_benchmark` once it lands. Record the size at which EK becomes impractical. Count the number of phases (level graphs) Dinic needs, which is a good number to explain the speedup.
6. **Stretch (only after the above, discuss with the team first):** reuse the previous flow when the next seam is searched, or a Boykov-Kolmogorov solver for comparison. Keep any such code in new files.

## Deliverables

- `src/maxflow_dinic.cpp`, `src/seam_graphcut_dinic.cpp`, `tests/test_dinic.cpp`
- `results/maxflow_*.csv` (algorithm, width, height, time, phases or augmentations) and a note on where EK stops being practical
- For R4: Dinic phase counts, timing table or chart data, and seam images of DP vs Dinic graph cut (reuse or extend C3's)

## Acceptance checks

- `seamcli in.png out.png <w> <h> dinic` produces a resized image of the right size
- Dinic equals Edmonds-Karp on the flow value in every random graph, and equals DP on seam cost in every random map
- A real-sized image (for example 1000x700) completes without a stack overflow
- `ctest` green; no warnings

## Dependencies

- **C3** for `FlowGraph`, `find_seam_graphcut`, `edmonds_karp` (the reference to compare with)
- C2 for `run_benchmark` (optional, you can time it yourself first)
