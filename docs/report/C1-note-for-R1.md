# C1 note for R1

From C1 (Core + DP) to R1 (Q2a and the DP algorithm). Everything below is checked against the code and tests
on branch `c1-core-dp`: `src/seam_dp.cpp`, `tests/test_dp.cpp`, `tests/test_energy.cpp`, `tests/test_resize.cpp`, `tests/test_image.cpp`.

## 1. The recurrence as implemented

`M[y][x]` is the cheapest cost of a seam from the top row ending at pixel `(x, y)`.

```
M[0][x] = e[0][x]
M[y][x] = e[y][x] + min(M[y-1][x-1], M[y-1][x], M[y-1][x+1])     y >= 1
```

A parent outside the image (`x-1 < 0` or `x+1 >= W`) is not a candidate, so the first and last columns take the
min of two values and a width-1 image takes the single parent. The optimal seam ends at the minimum of the last
row `M[H-1][*]` and is recovered by walking up, at each step moving to the parent with the smallest `M`.
The table is kept whole (`W*H` doubles) because backtracking needs every row.

Optimal substructure: the prefix of a cheapest seam ending at `(x, y)` is a cheapest seam ending at its parent,
since replacing it with a cheaper prefix would give a cheaper seam to `(x, y)`.

## 2. Tie-break rule

Among tied optimal seams the DP returns the one that is leftmost **judged from the bottom row upward**:
smallest column in the last row, then in the row above, and so on. Implemented as:

- last row: strict `<` while scanning left to right, so the leftmost minimum wins;
- backtracking: candidates tried left, straight, right; `<=` lets left win a tie against straight, strict `<` means
  right wins only if strictly cheaper than both.

The table fill needs no rule because it stores only costs. The rule is in `GLOSSARY.md` (Optimal seam).
For a horizontal seam it reads "topmost, from the rightmost column leftward", because the image is transposed first.
The greedy, graph-cut and Dinic owners should use the same rule if they want seams (not only costs) to match.

## 3. Edge cases found and what was done

| Found | Result |
|-------|--------|
| `find_seam_dp` on an empty map (width or height 0) read out of bounds | now throws `std::invalid_argument`; also throws if `e.size() != W*H` |
| `remove_vertical_seam` did not validate the seam (wrong length, column outside the image) or an image with no columns | now throws `std::invalid_argument` |
| "Leftmost" in the glossary was ambiguous (two tied seams can be leftmost in different rows) | defined as above; brute force checks it |
| Energies are `double`, so mathematically equal seams could differ by rounding | exact tie behaviour is only guaranteed on exactly representable energies; the brute-force tests use integer-valued maps. State this as a limitation |
| Width 1 | the seam is the only column; removing it leaves a width-0 image (never reached by `resize`, which requires target >= 1) |
| `load_image` forces 3 channels | an alpha channel in the input PNG is dropped |
| `resize` cannot enlarge | any target above the current size throws `std::invalid_argument` |
| Horizontal seams | found on `transpose(I)`; Sobel energy of the transpose is exactly the transposed energy map (the kernels are transposes and `|Gx| + |Gy|` is symmetric), so this is a faithful horizontal seam |

## 4. Tests: what was checked

- **Brute force** (`tests/test_dp.cpp`): for every width 1..6 and height 1..6, 50 random integer-valued maps
  (half with values 0..3 to force many ties, half 0..20), fixed RNG seed. All seams are enumerated and the test checks
  that the DP seam is valid (one column per row, in bounds, adjacent rows differ by at most 1), that its cost equals
  the minimum over all seams, and that it is exactly the bottom-up-leftmost minimum-cost seam. 1800 maps, all pass.
- **Hand-checked seams**: a trap map, width 1, height 1, 1x1, and 7 tie cases (all-equal, last-row tie, left/straight/right
  ties during backtracking).
- **Sobel by hand** (`tests/test_energy.cpp`): flat image gives 0; vertical and horizontal edges give 1020 only next to
  the edge; border clamping gives 400 on a 2x1 image; a white pixel in a black 3x3 gives 510 on the eight neighbours and 0
  at the centre; luma weights 0.299/0.587/0.114; transpose symmetry.
- **Resize** (`tests/test_resize.cpp`): same-size no-op, width-only, height-only, combined, to 1x1; the callback count equals the number
  of removed seams; alternation order V H V H V; a horizontal seam equals the vertical seam on the transpose; width-only
  resize equals removing DP seams by hand; invalid targets throw.
- **Do the tests bite?** Five deliberate bugs were injected into the DP one at a time (left `<=` to `<`, right `<` to `<=`,
  last-row `<` to `<=`, drop the upper-left parent, drop the upper-right parent). Each made at least one test fail.
- **Result**: 39 test cases, 7604 assertions, all pass; no warnings under `-Wall -Wextra -Wpedantic` for the library and the tests (MinGW g++).

## 5. Facts for the complexity section

- `sobel_energy`: one pass over `W*H` pixels, 12 grayscale reads each: `O(WH)` time, `O(WH)` space for the map.
- `find_seam_dp`: table fill `O(WH)` time and `O(WH)` space (rows-only would be `O(W)` for the cost alone, but backtracking needs all rows);
  backtracking `O(H)`.
- `remove_vertical_seam`: `O(WH)` copy.
- `resize` recomputes the energy map for every seam, so removing `k` seams costs `O(k * W * H)` in total (vertical and horizontal alike).
  A horizontal step adds two transposes, still `O(WH)`.

## 6. Still needed from other tasks

- Figures for the worked example (energy map, seam drawn on an image) need C6's `visualize.*`. A synthetic test image
  (gradient plus a red disc and a green rectangle) kept both objects intact when resized with DP; it is not committed.
- Real-image numbers (timings, quality) come from C2's benchmark harness and `testdata/`.
