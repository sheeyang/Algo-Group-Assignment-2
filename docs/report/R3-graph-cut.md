# R3: Graph cut

**Owner:** ____________  **Code partner:** C3 ([`docs/tasks/C3-graph-cut.md`](../tasks/C3-graph-cut.md))
**Assignment items:** 4, 5b (graph cut part), 5c (graph cut vs DP)

## What to write

### 1. The min-cut formulation (item 4)

- Describe the graph: nodes, source and sink, the finite edges with their capacities, and the infinite backward and diagonal edges. Draw a small example (e.g. 4 rows x 4 columns) with the minimum cut highlighted.
- Explain **why** every finite cut crosses each row exactly once and why the cut is a connected seam (what the infinite edges forbid). This is the core of the design marks.
- Explain how the seam is read from the cut: BFS/DFS from the source in the residual graph, then the rightmost source-side pixel per row.
- State the max-flow min-cut theorem and how it makes the cut capacity equal the seam cost, so the seam is optimal.
- Describe the **last-column issue and your fix** (see C3 task) and why it was needed. Say how you handled real-valued energies (scaling or tolerance) and ties.
- Horizontal seams: transposed image.
- Mention alternatives: forward-energy capacities, and that DP is a special structure of the same problem.

### 2. Max-flow algorithms and complexity (5b, graph cut part)

- Edmonds-Karp: how it works (BFS shortest augmenting path), bound `O(V E^2)`, and what V and E are here (`V ~ mn`, `E ~ 5mn`, give the edge count).
- Dinic: level graph and blocking flows, bound `O(V^2 E)`, and why it is much faster in practice on grid-like graphs.
- Space for the graph. Compare with DP's `O(mn)` time for one seam: say honestly why graph cut is slower, and what it offers in exchange (generality: the same machinery handles other constraints and energies).
- Give the bounds with the reasoning; mark clearly what is worst case and what is observed.

### 3. Results and graph cut vs DP (5c)

- **Correctness:** C3's test that graph-cut seam cost equals DP seam cost on random maps, for both EK and Dinic. Explain that the seams may differ in position when several cheapest seams tie.
- **Seams and visual quality:** the same image with DP and graph-cut seams; outputs should look identical when the costs are equal.
- **Running time:** EK vs Dinic vs DP on several image sizes (from C3 or C2's CSV), as a table and a chart; say at which size EK stops being practical.
- Conclude: should anyone use graph cut for plain seam carving?

## Needs from the code partner (C3)

- Final network definition including the last-column fix; max-flow test results; timing CSV; seam images.

## Figures and tables

- Flow network example with the cut; DP vs graph-cut seam overlay; EK/Dinic/DP timing chart.

## Draft

(Write here. Suggested headings: `## Graph cut formulation`, `## Max-flow algorithms and complexity`, `## Graph cut versus dynamic programming`)
