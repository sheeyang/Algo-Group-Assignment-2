# Report figures

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
