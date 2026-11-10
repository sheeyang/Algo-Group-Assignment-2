# R1: Q2a and the DP algorithm

**Owner:** Shee Yang  **Code partner:** C1 ([`docs/tasks/C1-core-dp.md`](../tasks/C1-core-dp.md))
**Assignment items:** 2a (i), 2a (ii), 1, 5b (DP part), 5a

## What to write

### 1. Q2a (i): the number of seams grows at least exponentially in m

Given an m x n image with n > 1, where a seam has one pixel per row and adjacent rows differ by at most one column.
- Prove the number of seams is at least exponential in m. A complete answer states the claim (give the explicit lower bound), proves it by induction on the rows or by counting choices per row, and says where `n > 1` is needed.
- Hint: count how many choices a seam has in the next row once it is at a given pixel, including at the left and right borders.
- Explain why this rules out brute force and motivates dynamic programming.

### 2. Q2a (ii): relating the seam problem to subproblems

- Define the subproblem precisely (the cheapest seam from the top row ending at pixel `(i, j)`), then write the recurrence **with the boundary cases** (first and last column, first row).
- Argue optimal substructure: why the best seam ending at `(i, j)` contains a best seam ending at one of its three parents.
- Give the answer extraction (minimum over the last row) and how the seam is recovered (backtracking).
- Give the running time and space of the table (feed section 5b below).

### 3. DP algorithm, design and implementation (item 1)

- The three steps from the brief: energy map, minimum seam by DP, remove and repeat. Include pseudocode and a small worked example (a 3x3 or 4x4 grid with the table filled in).
- Vertical and horizontal seams (the horizontal case runs the same code on the transposed image).
- The Sobel energy: the kernels, grayscale conversion, border clamping.
- The tie-breaking rule (leftmost), and why a fixed rule matters for comparing algorithms.
- **Alternatives considered** (rubric: "Consider alternative approaches"): e.g. forward energy, other energy functions, which ones you did not use and why.

### 4. Correctness and testing

Summarise C1's tests: hand-checked seams, the brute-force comparison on random maps, edge cases (width 1, ties, same-size resize). State exactly what was checked and the result.

### 5. Complexity (5b, DP part)

- Energy map: time and space.
- One seam by DP: time `O(mn)`, space for the full table versus rows only; seam removal cost.
- Whole resize to remove `k` seams: total time including recomputing the energy map every time. Say how it grows with `k`, `m` and `n`.
- Show your working, not only the final answer. These are the analysis marks.

## Needs from the code partner (C1)

- Recurrence and tie rule as implemented; list of edge cases found; brute-force test result.
- A few images of an energy map and a removed seam for the worked example (`figures/`).

## Figures and tables

- Worked DP table example. Energy map of a test image. Seam drawn on a test image.

## Draft

(Write here. Suggested headings: `## Question 2a(i)`, `## Question 2a(ii)`, `## Dynamic programming`, `## DP correctness and testing`, `## DP complexity`)
