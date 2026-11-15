# Report figures

## C4: DP versus Dinic

The labelled comparisons, native seam/result panels, per-step correctness
checks and reproducible max-flow benchmark instructions are in
[`c4/README.md`](c4/README.md).

## `greedy-vs-dp-<image>.jpg` (C2, for R2)

Each figure shows the original test image, then the DP and greedy results side by side at 90%, 70% and 50% of
the original width (height unchanged, so only vertical seams are removed). Panels are drawn at half size.
Images: `landscape-tower`, `edges-glass-building`, `portrait-red-jacket`, `trap-wall-door`.

Made on 2026-11-13 from branch `c2-greedy-cli` with, for each image, percentage and algorithm:

```
seamcli testdata/<image>.png out/fig/<image>-<algo>-<pct>.png --algo <dp|greedy> --width <800 * pct / 100>
```

The panels were then laid out and labelled with a short Pillow script (not kept in the repo; it only arranges
the seamcli outputs, it does no seam carving). Matching numbers: `results/bench_<image>.csv`.

## `gui-*` (C5, for R5)

Clean screenshots captured by Jarrel from the running Windows Release GUI on
2026-11-15 using `testdata/landscape-tower.png`. All four supplied PNGs are
preserved without conversion and replace the earlier computer-control captures:

- `gui-main.png`: DP resize from 800 x 533 to 600 x 533; clean screenshot supplied by Jarrel, preserved without conversion.
- `gui-progress.png`: a mixed-direction DP resize toward 400 x 266, captured at
  263 of 667 seams (39%), with progress and cancellation controls visible while
  the last completed 600 x 533 result remains on screen.
- `gui-seams.png`: all 200 vertical seams from the 600 x 533 result, mapped onto the original; clean screenshot supplied by Jarrel, preserved without conversion.
- `gui-benchmark.png`: clean screenshot supplied by Jarrel of the shared benchmark panel with DP and greedy, four scales,
  10 vertical seams and 3 repeats. The corresponding exported rows are in
  `results/gui-benchmark.csv`; these are demonstration measurements from this machine.

The GUI code and one-minute demo walkthrough are in `gui/README.md`.
