# Seam Carving (Algorithms Assignment 2)

Content-aware image shrinking in C++17 with three seam finders: dynamic programming, greedy, and graph cut (Edmonds-Karp and Dinic).
Vocabulary lives in [GLOSSARY.md](GLOSSARY.md); design decisions in [docs/adr/](docs/adr/).

## Layout

| Path | Purpose |
|------|---------|
| `src/` | `seamlib`: engine (energy, seam finders, resize). No GUI dependency |
| `cli/` | `seamcli`: headless tool for batch runs and benchmarks |
| `gui/` | `seamgui`: Dear ImGui + ImPlot frontend (bonus) |
| `tests/` | doctest unit tests, linked against `seamlib` only |
| `third_party/` | vendored single headers (stb, doctest) |
| `docs/` | assignment PDFs, ADRs, report draft, AI usage log |

## Build and test

```
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
build/cli/seamcli in.png out.png 400 300 dp
```

## Team split (5 people)

Everyone must be able to explain every part: the quiz is team-based and can lower the whole team's multiplier.

| # | Owner | Scope |
|---|-------|-------|
| 1 | | **DP + core**: energy, DP seam finder, resize loop, DP tests, Q2a written answers |
| 2 | | **Greedy + CLI**: greedy seam finder, `seamcli` options, benchmark/CSV output, Q2b (divide and conquer) |
| 3 | | **Graph cut**: flow network, Edmonds-Karp, Dinic, DP-vs-graph-cut cost cross-check tests |
| 4 | | **GUI + CI**: ImGui/ImPlot app, size sliders, step-by-step seam frames, CSV export, GitHub Actions |
| 5 | | **Report + video**: report structure, complexity analysis (5b-d), comparison write-up, AI usage declaration, presentation recording |

## Report checklist

Team ID and members with SIT IDs; Q2 answers; algorithm, design, implementation, results and comparison; GitHub link; video link; AI usage declaration.
