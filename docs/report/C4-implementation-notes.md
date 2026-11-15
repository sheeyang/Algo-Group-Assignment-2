# C4 hand-off for R4

All timings below were generated on 16 November 2026 with a Windows MinGW
GCC 13.2.0 Release build. Reproduction commands, counting definitions and figure
descriptions are in [the C4 evidence README](figures/c4/README.md).

## Solver and correctness

Dinic uses BFS level graphs, per-node current-arc pointers and an explicit DFS
path stack. It calls the same graph-cut construction and extraction as
Edmonds-Karp. The virtual column, integer capacity scale of 1000 and non-unique
minimum-cut behaviour are shared with C3; no second network is maintained.
Both existing three-argument solver functions remain available. Optional
`MaxFlowStats` overloads expose measured counts without changing `MaxFlowFn`.

The engine suite has 100 passing tests, including 300 random Dinic/EK graph
comparisons and 300 random Dinic/DP seam-cost comparisons. Added counter checks
distinguish two Dinic phases from three individual augmentations, verify residual
capacities match an unmeasured run, and check counters reset on a solved graph.
A 100000-node chain checks that DFS path depth does not consume the call stack.
The existing 200 x 200 random-map test remains in the normal test suite.

`results/maxflow_large.csv` records the separate real-size acceptance run:
a 1000 x 700 resampling of the landscape fixture completed one Dinic seam in
109748.9151 ms (about 109.75 seconds), with 572 phases and 156400 augmentations.
The seam was connected, inside the image and had the same cost as DP:
7967.598. This is a single-run completion check; it is not a median benchmark
or a complete multi-seam resize. Large graph-cut demos can therefore take a
long time even though the GUI remains responsive.

## EK versus Dinic measurements

The following first-seam values come from `results/maxflow_benchmark.csv`.
Energy calculation and image I/O are excluded. Graph-cut `time_ms` includes
network construction, max-flow and extraction; `solver_ms` isolates max-flow.
Times and counts in each row belong to the same selected run.

| Size | DP (ms) | Dinic (ms) | EK (ms) | Dinic phases | EK augmentations |
|------|---------|------------|---------|--------------|------------------|
| 32 x 21 | 0.0015 | 2.2372 | 5.0254 | 17 | 231 |
| 64 x 43 | 0.0045 | 8.8413 | 93.3704 | 26 | 721 |
| 128 x 85 | 0.0678 | 124.0191 | 1897.8851 | 89 | 3356 |
| 200 x 133 | 0.0777 | 601.7415 | 13592.5853* | 169 | 7618 |
| 400 x 267 | 0.1507 | 5598.4281* | skipped | 247 | - |
| 800 x 533 | 1.3503 | skipped | skipped | - | - |

Normally the program selects the median of three repeats. An asterisk means the
first call exceeded the 5000 ms budget, so only one run was performed and that
algorithm's larger sizes were skipped. The budget cannot interrupt a call that
has already started. For this fixture and machine, EK first exceeded five seconds
at 200 x 133 (13.59 seconds for one seam); Dinic first exceeded it at 400 x 267
(5.60 seconds). These are observed thresholds, not guarantees for other images
or hardware. Every measured seam agrees with the DP reference within 1e-6.

The CSV now comes from committed benchmark code and actual solver counters.
It supersedes the older max-flow CSV; use this run consistently rather than
mixing numbers from different compilers, fixtures or timing definitions.

## Figures and shared resize benchmarks

`figures/c4/dp-vs-dinic-portrait.png` and
`figures/c4/dp-vs-dinic-building.png` show the original, first seam from each
algorithm, and the results after ten removals. Native panels and all-seams
overviews are saved alongside them. The fixtures are the same 64 x 43 inputs
used by C3, and both final results are 54 x 43.

`results/c4-seam-checks.csv` contains twenty per-step comparisons, each against
DP on Dinic's exact current image. All agree within 1e-6. First-seam costs and
position/final-pixel equality are recorded in `results/c4-first-seams.csv`.
The portrait has equal first-seam costs with different positions and different
final pixels. The building has matching first-seam positions and final pixels.
These observations separate optimal cost from tie-dependent pixel equality.

`results/c4-benchmark-portrait.csv` and `results/c4-benchmark-building.csv` use
C2's shared runner: three repeats of ten removals per algorithm, selecting the
median total-time run. The independently evolving images can have different
later energies and cumulative costs, even when their final pixels match. Use
the per-step comparisons to claim optimality, rather than requiring cumulative
costs from separate resize sequences to be equal.
