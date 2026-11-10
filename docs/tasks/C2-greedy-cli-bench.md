# C2: Greedy + CLI + benchmarks

**Owner:** Wei Jun  **Report partner:** R2 ([`docs/report/R2-q2b-greedy.md`](../report/R2-q2b-greedy.md))
**Assignment items:** 3, plus the evidence for 5c, and the shared benchmark used by the GUI

## Goal

Implement the greedy seam finder, turn `seamcli` into a proper tool, and build the benchmark runner that produces the numbers (time, seam cost) for the whole report.

## What to do

1. **Greedy seam finder** in `src/seam_greedy.cpp` (`find_seam_greedy`):
   - start at the cheapest pixel of the top row, then in each next row take the cheapest of the (up to) three reachable pixels, with **no look-ahead**; ties go to the leftmost column (`GLOSSARY.md`: Greedy seam finder)
   - the result must be a valid seam (connected, in bounds), for any width including 1
2. **Tests** in `tests/test_greedy.cpp`:
   - a hand-built `EnergyMap` where greedy gets trapped and returns a strictly more expensive seam than DP (this is the example for the report)
   - on random maps: seam is valid, and `seam_cost(greedy) >= seam_cost(dp)` always
3. **Benchmark runner** in `src/bench.h` / `src/bench.cpp` (no GUI code, the GUI and CLI both use it). Proposed contract, agree changes with C5:
   ```cpp
   struct BenchRow { std::string algorithm; int width, height; int seams; double ms; double total_cost; };
   std::vector<BenchRow> run_benchmark(const Image& img, const std::vector<Algorithm>& algos,
                                       const std::vector<double>& scales, int seams, int repeats);
   void write_csv(const std::vector<BenchRow>& rows, std::ostream& out);
   ```
   - time with `std::chrono::steady_clock`, run `repeats` times and report the median
   - `total_cost` is the sum of the seam costs of every removed seam (fair across algorithms)
   - algorithms not yet implemented (graph cut) must be skipped cleanly, not crash the run
4. **`seamcli` options** (keep the current positional form working): `--algo dp|greedy|ek|dinic`, `--width`, `--height` (or a `--percent`), `--bench <image> --out results.csv`, and a clear usage message. Return non-zero on bad input.
5. **Test images** in `testdata/` (4 to 6): for example a landscape, a portrait, strong edges, a person, plus 1 or 2 tiny synthetic images. Use only images you may share (your own, public domain or CC0) and list the source and licence in `testdata/README.md`. Keep each file small enough for GitHub (under about 1 MB).

## Deliverables

- `src/seam_greedy.cpp`, `src/bench.*`, updated `cli/`, `tests/test_greedy.cpp`, `testdata/` with `README.md`
- `results/bench_*.csv` produced by your runner on the test images (commit them; R2, R3 and R6 build tables from these)
- For R2: seam and output images for greedy vs DP on at least 3 test images, and the observed timing ratio

## Acceptance checks

- `seamcli --bench testdata/x.png --out results/x.csv` writes a CSV with a header and one row per algorithm and scale
- Greedy never beats DP on cost in any test; at least one test shows it losing strictly
- `ctest` green; no warnings

## Interfaces others rely on

`BenchRow` and `run_benchmark` are used by the C5 GUI (plot + CSV export). Publish the header early, even with a stub body.
