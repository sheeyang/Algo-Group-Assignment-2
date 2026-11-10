# R2: Q2b, greedy, and greedy vs DP

**Owner:** ____________  **Code partner:** C2 ([`docs/tasks/C2-greedy-cli-bench.md`](../tasks/C2-greedy-cli-bench.md))
**Assignment items:** 2b, 3, 5b (greedy part), 5c (greedy vs DP)

## What to write

### 1. Q2b: can divide and conquer speed up the Sobel convolution?

The question: "Discuss whether applying the Divide and Conquer method could improve the efficiency of this computation."
- State the cost of the direct Sobel filter on an m x n image (a fixed-size kernel per pixel) and what it means for the best possible improvement.
- Discuss what divide and conquer can and cannot give: splitting the image into blocks or strips, the overlap (halo) of border pixels each block needs, the recurrence for the split, and why the total work stays the same order.
- Discuss where it **does** help: running blocks in parallel, cache locality. Link to what C5 measures for multi-threading.
- Mention related ideas and whether they apply: separable kernels (Sobel is a smoothing kernel times a difference kernel), and FFT-based convolution (when it pays off and why it does not for a 3x3 kernel).
- Give a clear verdict with the reasoning, not just a list.

### 2. Greedy algorithm: design and implementation (item 3)

- The rule: take the cheapest pixel of the top row, then in each row the cheapest of the three reachable pixels, no look-ahead. Pseudocode, tie rule, border handling.
- A small counter-example where greedy is trapped and DP is cheaper (use C2's test case; draw both seams).
- Why it can fail: a locally cheap step can lead into expensive pixels later.

### 3. Greedy complexity (5b, greedy part)

- Time for one seam (and compare with DP's table fill), space needed, and the total for removing `k` seams. Say whether the energy map recomputation still dominates.

### 4. Greedy vs DP (item 3 and 5c)

The brief asks: does greedy give comparable visual quality? Compare **seams, visual quality and running time**:
- **Seams:** show the same image with the first few seams from each algorithm; report seam cost greedy versus DP (from C2's CSV).
- **Visual quality:** side-by-side outputs for at least 3 test images and several shrink amounts (e.g. 10%, 30%, 50%). Describe visible artefacts (distorted lines, cut objects) honestly, including images where they look the same.
- **Running time:** table from C2's benchmark for several image sizes; give the ratio and explain it from the complexity analysis.
- Conclude: when is greedy good enough, when not.

## Needs from the code partner (C2)

- `results/bench_*.csv` for greedy and DP; output images of both algorithms on the test images; the trapping example.

## Figures and tables

- Greedy vs DP seams (first seam and 20 seams); side-by-side outputs; timing table or chart; the trapping example.

## Draft

(Write here. Suggested headings: `## Question 2b`, `## Greedy algorithm`, `## Greedy complexity`, `## Greedy versus dynamic programming`)
