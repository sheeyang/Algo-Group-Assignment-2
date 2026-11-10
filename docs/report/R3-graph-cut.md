# R3: Graph cut formulation + Edmonds-Karp

**Owner:** Zhi Kai  **Code partner:** C3 ([`docs/tasks/C3-graph-cut.md`](../tasks/C3-graph-cut.md))
**Works with:** R4 (Dinic and the graph cut vs DP comparison), who builds on your formulation section
**Assignment items:** 4 (formulation and Edmonds-Karp), 5b (graph cut, EK part)

## What to write

### 1. The min-cut formulation (item 4)

- Describe the graph: nodes, source and sink, the finite edges with their capacities, and the infinite backward and diagonal edges. Draw a small example (e.g. 4 rows x 4 columns) with the minimum cut highlighted.
- Explain **why** every finite cut crosses each row exactly once and why the cut is a connected seam (what the infinite edges forbid). This is the core of the design marks.
- Explain how the seam is read from the cut: BFS/DFS from the source in the residual graph, then the rightmost source-side pixel per row.
- State the max-flow min-cut theorem and how it makes the cut capacity equal the seam cost, so the seam is optimal.
- Describe the **last-column issue and your fix** (see C3 task) and why it was needed. Say how you handled real-valued energies (scaling or tolerance) and ties.
- Horizontal seams: transposed image.
- Mention alternatives: forward-energy capacities, and that DP is a special structure of the same problem.

### 2. Edmonds-Karp and its complexity (5b, EK part)

- Edmonds-Karp: how it works (BFS shortest augmenting path), bound `O(V E^2)`, and what V and E are here (`V ~ mn`, `E ~ 5mn`, give the edge count). Give the reasoning behind the bound, not only the result.
- Space for the graph.
- Compare with DP's `O(mn)` time for one seam: say honestly why graph cut is slower, and what it offers in exchange (generality: the same machinery handles other constraints and energies).
- Mark clearly what is worst case and what is observed.
- Dinic is covered by R4. Agree with R4 who explains the shared parts (the network and the residual graph) so they are written once and referenced.

### 3. Correctness

- C3's test that EK graph-cut seam cost equals DP seam cost on random maps. Explain that the seams may differ in position when several cheapest seams tie.
- The max-flow tests on known graphs.

## Needs from the code partner (C3)

- Final network definition including the last-column fix; capacity choice; EK test results; seam images.

## Figures and tables

- Flow network example with the cut; DP vs graph-cut seam overlay.

## Draft

(Write here. Suggested headings: `## Graph cut formulation`, `## Edmonds-Karp and complexity`, `## Graph cut correctness`)
