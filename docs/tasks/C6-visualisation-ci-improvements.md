# C6: Visualisation + CI + improvements

**Owner:** Zhe An  **Report partner:** R6 ([`docs/report/R6-improvements-bonus-front.md`](../report/R6-improvements-bonus-front.md))
**Assignment items:** bonus 7a (step-by-step visualisation) and 7c (CI/CD), and 5d (improvements)

## Goal

Make every seam visible, keep the repo continuously tested, and answer the assignment's improvement questions with measurements ("do we need to maintain the entire DP table?", "does multi-threading help?").

## What to do

### A. Step-by-step visualisation (bonus a)

1. `src/visualize.h` / `src/visualize.cpp`, no GUI code:
   - `draw_seam(const Image&, const Seam&, bool vertical)` returns a copy with the seam drawn in a bright colour
   - an overview image with **all removed seams drawn on the original image**. Seams are found on shrinking images, so each seam's coordinates must be mapped back to original coordinates (keep an index map that is shrunk together with the image)
2. `seamcli` option `--steps <dir>`: write one frame per removal (`step_0001.png`, ...) using the existing `StepCallback` in `resize.h`, plus the all-seams overview. Coordinate with C2, who owns `cli/`: add your option in a new file if possible.
3. Provide C5 a function to render step *k* on demand, so the GUI does not store every frame in memory. Propose the interface on day 1.
4. Tests: mapping back to original coordinates is correct (every drawn seam pixel lies on the original image and no two seams share a pixel).

### B. CI/CD (bonus c)

`.github/workflows/ci.yml` already builds and tests on Ubuntu and Windows. Extend it:
- a **smoke test**: run `seamcli` on a file from `testdata/` and check the output size
- an optional job that builds the GUI (`-DSEAM_BUILD_GUI=ON`; on Ubuntu install `xorg-dev libgl1-mesa-dev` first), allowed to be separate so a GUI failure is easy to spot
- upload the `seamcli` binary (and test log on failure) as artifacts
- a CI status badge at the top of `README.md`
- stretch: on a version tag, attach binaries to a GitHub Release

### C. Improvements for question 5d

Implement each variant **next to** the baseline (do not replace `find_seam_dp`), keep results identical to the baseline, and measure with C2's benchmark:
1. **Do we need the whole DP table?** Cost-only needs two rows (O(W) memory). Backtracking needs directions: store one small direction value per cell instead of a full `double` table, or compare memory of the options. Report measured memory and time.
2. **Incremental energy update:** after removing a seam, only energies near the seam change, so recompute those instead of the whole map.
3. **Multi-threading:** parallelise energy computation by rows, and try parallelising the cells of one DP row (each depends only on the previous row). Measure the speedup for several image sizes and thread counts; small images may get **slower**, and that is a valid finding.
4. Tests: every variant returns a seam with the **same cost** as the baseline on random maps.

## Deliverables

- `src/visualize.*`, `--steps` in `seamcli`, the optimised variants (new files), tests for all of them
- Extended CI workflow with a passing run on `main`, badge in `README.md`
- `results/improvements_*.csv` (time, memory, threads) and a sample step-by-step output (a few frames and the all-seams overview) in `docs/report/figures/` for R6

## Acceptance checks

- The all-seams overview of a test image shows each seam once, at its original position
- CI is green on both operating systems and fails when a test is broken (try it once on a throwaway branch)
- Every improvement has a before/after number, including the cases where it did not help

## Dependencies

- C1 for the baseline and for any change to `resize.h`
- C2 for `run_benchmark` and `cli/`
- C5 for the step rendering interface
