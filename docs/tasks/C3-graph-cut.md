# C3: Graph cut (min-cut / max-flow)

**Owner:** ____________  **Report partner:** R3 ([`docs/report/R3-graph-cut.md`](../report/R3-graph-cut.md))
**Assignment items:** 4, plus the evidence for 5b and 5c

## Goal

Reformulate seam finding as a minimum s-t cut, solve it with max-flow using **both Edmonds-Karp and Dinic**, and show it finds a seam of the same cost as DP.

## What to do

1. **Max-flow library** in `src/maxflow.h` / `src/maxflow.cpp`: a small directed-graph class with `add_edge(u, v, cap)`, plus `edmonds_karp(s, t)` and `dinic(s, t)`. Keep it independent of images so it can be tested on textbook graphs. Provide the set of nodes reachable from `s` in the residual graph (needed for step 4).
2. **Flow network** from the brief (item 4) in `src/seam_graphcut.cpp`, one node per pixel plus source and sink:
   - source to every pixel of the first column, every pixel of the last column to sink, both infinite
   - edge `(i,j) -> (i,j+1)` with capacity equal to the cost of removing pixel `(i,j)` (its energy)
   - infinite backward edge `(i,j+1) -> (i,j)`, and infinite diagonal edges `(i,j) -> (i-1,j-1)` and `(i,j) -> (i+1,j-1)`, so every finite cut crosses each row exactly once and the seam is connected
3. **Fill in `find_seam_graphcut_ek` and `find_seam_graphcut_dinic`** (stubs in `seam_graphcut.cpp`). Horizontal seams are handled by the existing transpose in `resize.cpp`, so you only build the vertical network.
4. **Extract the seam:** run max-flow, BFS/DFS from the source in the residual graph to get the source side S; in each row the **rightmost pixel in S** is the seam pixel.
5. **Watch these pitfalls** (decide, test and write down your answer for R3):
   - **Last column.** With the construction exactly as written, the cut edge in a row sits between `j` and `j+1`, so the seam can never use the last column, while DP can. Costs then disagree with DP on some inputs. Find a fix (for example a virtual column or an adjusted source/sink edge), explain it, and verify it.
   - **Capacities.** Energies are doubles. Either scale and round to integers (e.g. `long long`) or use a tolerance. State which and why. Use integer-valued energies in the DP-equality tests so equality is exact.
   - **Ties.** Min cuts are not unique, so the seam may differ from DP's while the **cost** is equal. Compare costs, not positions ("Seam agreement" in `GLOSSARY.md`).
   - **Speed and memory.** Edmonds-Karp on a full image is very slow. Use a compact adjacency (arrays, not one heap allocation per edge), and limit EK benchmarks to small images.

## Deliverables

- `src/maxflow.*`, `src/seam_graphcut.cpp`, `tests/test_maxflow.cpp`, `tests/test_graphcut.cpp`
- Tests: max-flow on small known graphs (EK and Dinic agree with the known value); **graph-cut seam cost equals DP seam cost** on at least 200 random small maps, for both EK and Dinic; the seam is connected and in bounds
- Timing data for R3: EK vs Dinic vs DP across image sizes (use C2's `run_benchmark` once it lands, or time it yourself)
- Seam images for DP vs graph cut on 2 to 3 test images

## Acceptance checks

- `seamcli in.png out.png <w> <h> ek` and `... dinic` produce a resized image of the right size
- The equality test passes on every random map, not most of them
- `ctest` green; no warnings

## Dependencies

Only `seam.h` / `energy.h` (exist now). No need to wait for C1 or C2.
