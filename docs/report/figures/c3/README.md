# C3 / R3 evidence

Generated on 16 November 2026 with the project's MSVC x64 Release build.
These are curated report artifacts; temporary executables stay under `build/`.

## Figures

- `dp-vs-ek-portrait.png`, `dp-vs-ek-building.png`: labelled five-panel
  comparisons: input, DP first seam, EK first seam, DP result, EK result.
  The input is 64x43 and the output is 54x43 after ten vertical removals.
  Panels are enlarged 4x using nearest-neighbour display scaling.
- `<image>-input.png`, `<image>-dp-first.png`, `<image>-ek-first.png`,
  `<image>-dp-result.png`, `<image>-ek-result.png`: native-resolution panels.
- `<image>-dp-overview.png`, `<image>-ek-overview.png`: all ten removed seams,
  mapped to input coordinates by C6's `SeamTracker`.
- `network-example.svg`: complete original directed network for a four-row,
  four-column example, including virtual nodes and all INF constraints.
  Reverse residual edges are omitted for readability. The unique seam is
  [1,1,2,1]; the original energy cost is 4 and the scaled cut capacity is 4000.
- `graphcut-network.png`: 2320x1470 raster copy of that SVG for embedding in
  the final report. The SVG remains the editable vector source; re-export
  the PNG if the network example is changed.

Input source and downscaling details are in
[`testdata/c3/README.md`](../../../../testdata/c3/README.md).

## Numerical evidence

Under `results/`:

- `c3-first-seams.csv`: first-seam costs, positional equality and final pixel
  equality on each input. Cost equality and position equality are different.
- `c3-seam-checks.csv`: ten EK steps per input, each compared to DP on EK's
  exact current image. All twenty comparisons agree within 1e-6. Every seam
  was also checked for valid length, bounds and connectivity.
- `c3-benchmark-portrait.csv`, `c3-benchmark-building.csv`: median of five
  fresh ten-seam runs per algorithm, using C2's `run_benchmark`. No image I/O
  or figure rendering is timed. `find_ms` is the seam-finding portion of the
  selected median total-time run, not an independent median.
- `c3-tests.log`, `c3-build.log`: final test and clean Release build output.

Timings are a local snapshot; rerunning changes measured times. Do not claim
that cumulative costs along independently evolving resize sequences must
agree. The per-step reference on the same current image is the correctness
check. R3 discusses the observed differences.

## Reproduce

From a Visual Studio Developer PowerShell at the repository root, build the
MSVC Release library and CLI:

```powershell
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DSEAM_BUILD_GUI=OFF
cmake --build build/release
ctest --test-dir build/release --output-on-failure
```

Then generate measurements and figures:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/generate-c3-evidence.ps1
```

This runs a checked-in C++ driver against the actual seam library, then uses
Windows System.Drawing to arrange and label the PNG panels. It does no AI
image generation or substitution for the seam finders. The PowerShell
execution-policy option applies to that process only. The script locates
MSVC using Visual Studio's `vswhere`, compiles its driver in the ignored build
directory and leaves the source photos unchanged.

The generator updates CSVs but does not rewrite the report's timing table;
if regenerating measurements, update that table from the new CSV values.

For interactive inspection through the existing CLI instead:

```powershell
.\build\release\cli\seamcli.exe testdata/c3/portrait-small.png build/release/portrait-ek.png --algo ek --width 54 --steps build/release/portrait-ek-steps
.\build\release\cli\seamcli.exe testdata/c3/portrait-small.png build/release/portrait-dp.png --algo dp --width 54 --steps build/release/portrait-dp-steps
```

Repeat with `building-small.png` and matching building output names. The
first overlay is `step_0001.png`; `overview.png` maps all removals onto the
original small input. Share these report figures and CSVs with R4 and R6.
