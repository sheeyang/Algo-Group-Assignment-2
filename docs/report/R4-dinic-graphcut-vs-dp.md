# R4: Dinic, EK vs Dinic, and graph cut vs DP

**Owner:** Dayton  **Code partner:** C4 ([`docs/tasks/C4-dinic.md`](../tasks/C4-dinic.md))
**Works with:** R3 (formulation and Edmonds-Karp), whose graph description you reference and do not repeat
**Assignment items:** 4 (Dinic), 5b (graph cut, Dinic part), 5c (graph cut vs DP)

## What to write

### 1. Dinic's algorithm

- How it works: level graph from a BFS, blocking flow by DFS along level-increasing edges, current-arc pointers, repeat until `t` is unreachable. Include pseudocode and a small worked example (2 or 3 phases on a tiny graph).
- Why it is correct (why the shortest augmenting path length strictly increases after each phase).
- Why we chose it: the brief names Ford-Fulkerson and Edmonds-Karp, and Dinic is our faster alternative on the same network. Mention other options considered (for example Boykov-Kolmogorov, push-relabel) and why they were not used.
- Implementation notes from C4: iterative DFS to avoid stack overflow, compact graph storage.

### 2. Complexity (5b, Dinic part)

- Bound `O(V^2 E)` in general, with the reasoning: at most `V` phases, each phase `O(VE)`. Say what V and E are here (see R3).
- Why it behaves much better in practice on grid-like graphs, and the observed number of phases from C4 (measured, not guessed).
- Space.
- Mark clearly what is worst case and what is observed.

### 3. Edmonds-Karp vs Dinic

- Timing table and chart from C4's `results/maxflow_*.csv` across image sizes; the size at which EK stops being practical; speedup factor.
- Link the observed difference to the two bounds and to the phase counts.
- Agreement: both give the same flow value on random graphs, and the same seam cost as DP.

### 4. Graph cut vs dynamic programming (5c)

- **Seams and visual quality:** the same image with DP and graph-cut seams. When costs are equal the outputs should be equally good; say what you see, and that seams may differ in position when several cheapest seams tie.
- **Running time:** DP vs EK vs Dinic on several image sizes, as a table and a chart; use the same timing method as R5's evaluation section.
- **Memory:** graph storage versus the DP table.
- Conclude: should anyone use graph cut for plain seam carving? What does it offer that DP does not (arbitrary constraints and energies), and what does it cost?

## Needs from the code partner (C4)

- `results/maxflow_*.csv`, Dinic phase counts, test results, DP vs Dinic seam images.

## Figures and tables

- Dinic worked example; EK vs Dinic vs DP timing chart; DP vs graph-cut seam overlay.

## Draft

(Write here. Suggested headings: `## Dinic's algorithm`, `## Dinic complexity`, `## Edmonds-Karp versus Dinic`, `## Graph cut versus dynamic programming`)
