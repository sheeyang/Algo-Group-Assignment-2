# Seam Carving (Algorithms Assignment 2)

Content-aware image shrinking in C++17 with three seam finders: dynamic programming, greedy, and graph cut (Edmonds-Karp and Dinic).
Vocabulary lives in [GLOSSARY.md](GLOSSARY.md); design decisions in [docs/adr/](docs/adr/).

## Layout

| Path | Purpose |
|------|---------|
| `src/` | `seamlib`: engine (energy, seam finders, resize). No GUI dependency |
| `cli/` | `seamcli`: headless tool for batch runs and benchmarks |
| `gui/` | `seamgui`: Dear ImGui + ImPlot frontend (bonus) |
| `docs/tasks/` | one file per coding task: what to do, deliverables, acceptance checks |
| `tests/` | doctest unit tests, linked against `seamlib` only |
| `third_party/` | vendored single headers (stb, doctest) |
| `docs/` | assignment PDFs, ADRs, report drafts (`docs/report/`), AI usage log |

## Build and test

```
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
build/cli/seamcli in.png out.png 400 300 dp
```

## Team split (5 people)

Everyone must be able to explain every part: the quiz is team-based and can lower the whole team's multiplier.
Each person owns **one coding task and one report task**, so all 5 members touch both the code and the report.

Full task descriptions (what to do and what to deliver): [`docs/tasks/`](docs/tasks/README.md). Report drafts, one file per report task: [`docs/report/`](docs/report/README.md).

### Coding tasks

| # | Owner | Coding task |
|---|-------|-------------|
| C1 | | **Core + DP**: image I/O, Sobel energy, DP seam finder, resize loop, DP unit tests (hand-checked seams) |
| C2 | | **Greedy + CLI/benchmarks**: greedy seam finder, `seamcli` options, benchmark runner that writes CSV (time and seam cost per algorithm and image size) |
| C3 | | **Graph cut**: flow network from the brief, Edmonds-Karp and Dinic, tests that graph-cut seam cost equals DP seam cost |
| C4 | | **GUI**: Dear ImGui + ImPlot app, width/height sliders (bonus b), algorithm picker, benchmark plot with CSV export |
| C5 | | **Visualisation + CI + improvements**: step frames with every seam drawn (bonus a), GitHub Actions (bonus c), DP improvements for 5d (rolling rows instead of the full table, multi-threading experiment) |

### Report tasks

| # | Owner | Report task |
|---|-------|-------------|
| R1 | | **Q2a (i) and (ii)** (exponential seam count, DP recurrence) + DP algorithm, correctness and time/space complexity (5b) |
| R2 | | **Q2b** (divide and conquer for Sobel) + greedy algorithm section + greedy vs DP comparison of seams, visual quality and running time (3, 5c) |
| R3 | | **Graph cut** section (min-cut formulation, max-flow min-cut argument, Edmonds-Karp vs Dinic complexity) + graph cut vs DP comparison (4, 5c) |
| R4 | | **Introduction, system design and architecture, test images and evaluation method, GUI / user-interaction write-up (bonus b)**, references |
| R5 | | **Front matter** (team ID, members with SIT IDs, GitHub and video links), **improvements and bonus** write-up (5d, bonus a and c), **AI usage declaration**, final assembly and export of the report |

Suggested pairing: person *n* does C*n* and R*n*, but any swap works as long as each person has exactly one of each.

## Presentation

Each person has 2 minutes (12 minutes for the team): present your own coding task and report section, and demo it running.

## Report checklist

Team ID and members with SIT IDs; Q2 answers; algorithm, design, implementation, results and comparison; GitHub link; video link; AI usage declaration (log in `docs/ai-usage.md`).
