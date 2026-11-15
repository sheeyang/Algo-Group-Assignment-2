# C4 / R4 evidence

These figures and CSVs are generated from seamlib, using the same portrait and
building fixtures as C3. Input provenance and preparation are documented in
`testdata/c3/README.md`. Inputs are 64 x 43; results are 54 x 43 after ten vertical
seam removals. The labelled `dp-vs-dinic-*.png` comparisons show the input, each
algorithm's first seam in red, and each algorithm's result. They enlarge the
native panels by 4x using nearest-neighbour display scaling.

The measured timing table, observed EK practicality threshold and large-image
result are summarised in [the C4 hand-off note](../../C4-implementation-notes.md).

Individual `*-dp-first.png`, `*-dinic-first.png`, `*-dp-result.png` and
`*-dinic-result.png` files contain the native-resolution panels.
`*-dp-overview.png` and `*-dinic-overview.png` show all ten removed seams mapped
onto the original image with C6's `SeamTracker`.

## Reproduce

From the repository root on Windows, with CMake and a C++17 compiler installed:

```powershell
cmake -S . -B build-c4 -DCMAKE_BUILD_TYPE=Release
powershell -ExecutionPolicy Bypass -File tools/generate-c4-evidence.ps1 -BuildDir build-c4 -Large
```

The script builds the tools, generates and labels the figures, then regenerates
the measurements. `-Large` adds the slower 1000 x 700 acceptance run. Omit it for
the normal benchmark sizes. On Linux, the C++ generators produce the native PNG
panels and all CSVs without PowerShell or graphics libraries:

```sh
cmake -S . -B build-c4 -DCMAKE_BUILD_TYPE=Release
cmake --build build-c4 --target c4_evidence bench_maxflow --parallel
./build-c4/bench/c4_evidence
./build-c4/bench/bench_maxflow results --large
```

The Windows script additionally composes the labelled comparisons using
System.Drawing. Both programs require a Release build and must run from the
repository root. For Visual Studio, the executables are under `bench/Release/`.

## Numerical evidence and interpretation

- `results/c4-first-seams.csv`: first-seam costs, position agreement, and whether
  the independently resized final images are pixel-identical.
- `results/c4-seam-checks.csv`: a DP reference on Dinic's exact current image for
  each of the twenty removals. The generator checks seam length, bounds,
  connectivity and agreement within 1e-6. Independent resize sequences can take
  different tied seams, so their later cumulative costs need not agree.
- `results/c4-benchmark-*.csv`: C2's shared `run_benchmark`, with ten seams and
  three repeats. Each row comes from the median total-time run; `find_ms` belongs
  to that same run. Energy calculation and removal are included in `ms`.
- `results/maxflow_benchmark.csv`: a separate first-seam benchmark on scaled
  copies of `testdata/landscape-tower.png`, using C2's `scale_image` box filter.
  Sobel computation happens before timing. `time_ms` includes seam finding
  (network construction, solver and extraction for graph cut); `solver_ms`
  measures just max-flow. DP has no max-flow solver, so that column is zero.
  The median run supplies both timing and counts. `repeats` states how many runs
  completed; normally three, but repetition stops when a run exceeds 5000 ms.
  That algorithm's subsequent larger sizes are explicitly marked as skipped.
  A single in-flight max-flow call cannot be interrupted by the budget.
- `results/maxflow_large.csv`: one DP and one Dinic seam on a 1000 x 700 image,
  made from the landscape fixture by deterministic nearest-neighbour resampling.
  This is a completion/stack-safety check, not a median performance comparison.

Counts come directly from the measured solver overloads in `src/maxflow.*` and
`src/maxflow_dinic.cpp`. `phases` counts successful Dinic level-graph BFS/blocking
flow passes, excluding the final unsuccessful BFS. `augmentations` counts
individual source-to-sink path pushes. EK has augmentations but no Dinic phases;
DP has neither. Counts reset for every solver call. These regenerated results
supersede the earlier max-flow CSV whose measurement program was not committed.

Timings depend on the machine and compiler. Use measurements from the same run
when comparing algorithms. Every measured seam is validated against DP; exact
pixel/position agreement is recorded separately and is not required on ties.
