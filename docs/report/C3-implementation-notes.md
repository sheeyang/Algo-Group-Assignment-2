# C3 implementation notes

Read the implementation in this order:

1. `src/maxflow.h`: the graph shared with C4. `adj[u]` contains outgoing edge
   indices; `edges[i ^ 1]` is the reverse residual edge of `edges[i]`.
2. `src/maxflow.cpp`: BFS augmenting paths, bottleneck calculation, residual
   updates, and source reachability after max-flow.
3. `src/seam_graphcut.cpp`: energy capacities, graph construction, then seam
   extraction. `find_seam_graphcut(e, solver)` is shared with Dinic.
4. `tests/test_maxflow.cpp` and `tests/test_graphcut.cpp`: known flow values,
   reverse-edge cancellation, and comparison against DP on 500 random maps.

## Final network and correctness argument

For width W and height H, create nodes (x,y) for x=0..W and y=0..H-1,
plus source s and sink t. Column W is virtual; it has no pixel energy.

- Add s -> (0,y) and (W,y) -> t with capacity INF for every row.
- Add (x,y) -> (x+1,y) with capacity round(1000 * energy(x,y)), x=0..W-1.
- Add (x,y) -> (x-1,y) with capacity INF, x=1..W.
- Add (x,y) -> (x-1,y-1) and (x,y) -> (x-1,y+1) with capacity INF
  where the neighbouring row exists, x=1..W.

The virtual column fixes the brief's last-column problem: cutting the edge
from the last real pixel to the virtual node selects that last pixel.

INF is one greater than the sum of all finite capacities. A straight seam
gives a cut costing at most that sum, so a minimum cut cannot cross an INF edge.
The backward constraints make each row's source-side nodes a nonempty prefix
ending before its virtual node. Exactly one finite forward edge then crosses
the cut per row. The diagonal constraints force adjacent prefix endpoints to
differ by at most one column. Conversely, every valid seam defines such a cut.
Thus cut capacity equals the scaled seam cost; max-flow/min-cut gives an
optimal seam for the integer capacities.

After the solver finishes, traverse positive residual capacities from s.
The rightmost reachable real node in each row is the selected seam pixel.
Horizontal seams use the existing transpose in `resize.cpp`.

## Capacities and ties

Capacities use `long long` and energies are scaled by 1000 and rounded. This
matches the precision of the current grayscale coefficients (0.299, 0.587,
0.114) in the Sobel energy calculation, apart from floating-point arithmetic
error. Arbitrary double inputs are quantised: differences smaller than this
precision can alter the optimum compared with unrounded DP. Integer-valued
maps within the supported capacity range are represented exactly and used in
the random agreement tests. Nonfinite and negative energies are rejected;
capacity and total-flow overflow are checked.

Compare seam costs rather than seam positions when minima tie. Graph cut
does not promise DP's bottom-up tie rule.

## Complexity and C4 integration

There are V=H(W+1)+2 nodes and E=4WH+2H-2W original directed edges.
Each original edge also has a reverse residual edge. Storage is O(V+E).
Edmonds-Karp takes O(V E^2) worst-case time: O((WH)^3) on this network.
DP takes O(WH) per seam, so use small inputs for EK benchmarks.

C4 can implement `dinic(FlowGraph&, int, int)` in `maxflow_dinic.cpp` and
replace its seam wrapper with `return find_seam_graphcut(e, dinic);`.
Copy a graph before comparing flow solvers because both mutate capacities.
The existing CMake globs include new source and test files automatically.

Image overlays, timing measurements, and final report figures remain to be
collected. These notes describe the implementation, not measured performance.

## Local verification (13 November 2026)

Built with MSVC using the installed Visual Studio CMake and Ninja tools in
`build-c3/`. All 52 test cases and 14,152 assertions passed, including the
500 random integer-map comparisons and mixed vertical/horizontal resizing.
The CLI resized a generated 6x5 BMP to a PNG whose header was checked as 4x3.
MSVC reported C5285 in the existing vendored doctest header; the new solver
sources compiled without warnings.

Run the built tests from the repository root:

```powershell
.\build-c3\tests\seam_tests.exe
```

The temporary input and outputs used for that smoke test were removed during
cleanup. For a reproducible photo test and CLI example, see
[`testdata/c3/README.md`](../../testdata/c3/README.md).
