# Coding tasks

Six coding tasks, one per person. Each file says what to do, what to deliver, and how to know it is done.
Report tasks are separate: see [`docs/report/`](../report/README.md). Person *n* suggested pairing: C*n* + R*n*.

| Task | Owner | Title | Main files | Report partner |
|------|-------|-------|------------|----------------|
| [C1](C1-core-dp.md) | Shee Yang | Core + DP | `src/energy.*`, `src/seam_dp.cpp`, `src/resize.*`, `tests/test_dp.cpp` | R1 |
| [C2](C2-greedy-cli-bench.md) | Wei Jun | Greedy + CLI + benchmarks | `src/seam_greedy.cpp`, `src/bench.*`, `cli/`, `testdata/` | R2 |
| [C3](C3-graph-cut.md) | Zhi Kai | Graph cut network + Edmonds-Karp | `src/seam_graphcut.cpp`, `src/maxflow.*` | R3 |
| [C4](C4-dinic.md) | Dayton | Dinic + EK vs Dinic | `src/maxflow_dinic.cpp`, `src/seam_graphcut_dinic.cpp` | R4 |
| [C5](C5-gui.md) | Jarrel | ImGui + ImPlot GUI | `gui/` | R5 |
| [C6](C6-visualisation-ci-improvements.md) | Zhe An | Visualisation + CI + improvements | `src/visualize.*`, `.github/`, DP variants | R6 |

## Everyone can start today

The scaffold already compiles. C2, C3 and C4 implement against `Seam find_seam_*(const EnergyMap&)` in
`src/seam.h` and test on small hand-built `EnergyMap`s, so nobody waits for C1. C5 and C6 depend on
the contracts below, so agree them in the first two days. **C4 builds on C3's `FlowGraph`**: C3 publishes
`src/maxflow.h` first (see C3), and C4 writes Dinic against that contract meanwhile.

## Shared contracts (change only after telling the others)

| Contract | Where | Owner |
|----------|-------|-------|
| `Image`, `load_image`, `save_image`, `transpose` | `src/image.h` | C1 |
| `EnergyMap`, `sobel_energy` | `src/energy.h` | C1 |
| `Seam`, `seam_cost`, the four `find_seam_*` functions, `remove_vertical_seam` | `src/seam.h` | C1 |
| `Algorithm`, `resize(...)`, `StepCallback` | `src/resize.h` | C1 |
| `FlowGraph`, `edmonds_karp`, `reachable_from`, `find_seam_graphcut(e, MaxFlowFn)` (see C3); C4 adds `dinic` | `src/maxflow.h`, `src/seam.h` | C3 (C4 for `dinic`) |
| `BenchRow`, `run_benchmark(...)` (see C2) | `src/bench.h` | C2 |
| Seam overlay and step rendering (see C6) | `src/visualize.h` | C6 |

Vocabulary is fixed in [`GLOSSARY.md`](../../GLOSSARY.md): use those terms in code, comments, commits and the report.

## Working rules

- One branch per task (`c1-core-dp`, `c2-greedy-cli`, ..., `c6-dinic`), merged into `main` by pull request; another teammate reviews. CI must be green.
- Put new code in **new files** where you can. `src/` and `tests/` use a glob, so adding `src/foo.cpp` or `tests/test_foo.cpp` needs no CMake edit.
- Do not reformat or rename files you do not own.
- Commit small and often. Do not commit `build/`.

## Definition of done (every task)

- Builds with **no warnings** (`-Wall -Wextra -Wpedantic`) on Windows (MinGW) and Linux.
- Unit tests in `tests/test_<topic>.cpp`, and `ctest` passes. Tests that prove correctness count for the rubric ("proof of detail testing to ensure correctness of all cases").
- Code is commented where the algorithm is not obvious (rubric: "good programming practice, well explained and commented").
- Your outputs for the report partner (numbers, CSV, figures) are saved where the task file says, and you have told them.
- If you used an AI tool, add a row to [`docs/ai-usage.md`](../ai-usage.md) with the main prompt.
- You can explain every line you wrote, and the code of the teammates around you: the project quiz is team-based and works from the submitted code.
- You can demo it in about 1 minute for the presentation (2 minutes per person).
