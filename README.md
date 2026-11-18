[![CI](https://github.com/sheeyang/Algo-Group-Assignment-2/actions/workflows/ci.yml/badge.svg)](https://github.com/sheeyang/Algo-Group-Assignment-2/actions/workflows/ci.yml)

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
| `bench/` | benchmark programs for the 5d improvements; they write `results/improvements_*.csv` |
| `third_party/` | vendored single headers (stb, doctest) |
| `docs/` | assignment PDFs, ADRs, report drafts (`docs/report/`), AI usage log |

## Run in Visual Studio on Windows

Install the **Desktop development with C++** workload in Visual Studio 2022 or later.
In Visual Studio Installer, make sure the workload includes **C++ CMake tools for Windows**.

1. Choose **File > Open > Folder** and select the cloned folder containing `CMakeLists.txt`.
2. On the toolbar, select **Windows Desktop (CLI + GUI)** and wait for CMake to finish.
   The first configuration needs internet access to download the GUI libraries.
3. In the **Startup Item** dropdown by the green Run button, select `seamgui.exe` or `seamcli.exe`.
   Press **Ctrl+F5** to build and run. For `seamgui.exe`, drag an image from `testdata/` onto the window.

To resize an image with `seamcli.exe`, select it as the startup item, then choose
**Debug > Debug and Launch Settings**. In the generated configuration, set `currentDir`
to the absolute path to your clone and add the arguments below for a sample resize.

```json
"currentDir": "C:/path/to/your/clone",
"args": ["testdata/landscape-tower.png", "out.png", "640", "430", "dp"]
```

Save the launch settings and run the corresponding startup item. The sample writes `out.png`
to your clone.

## Build and test from a terminal

### Windows PowerShell

Run these commands from the repository root with CMake, Ninja, and a C++ toolchain
available. If you use MSVC, open **Developer PowerShell for Visual Studio**.

Build both executables with the desktop preset. The first configuration downloads
the GUI libraries.

```powershell
cmake --preset windows-desktop
cmake --build out/build/windows-desktop --parallel
```

Run `seamcli` to resize the sample image to 640 × 430 and save it as `out.png`.

```powershell
.\out\build\windows-desktop\cli\seamcli.exe testdata/landscape-tower.png out.png 640 430 dp
```

Run `seamgui` to open the sample image in the desktop app.

```powershell
.\out\build\windows-desktop\gui\seamgui.exe testdata/landscape-tower.png
```

### Linux

```
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
build/cli/seamcli in.png out.png 400 300 dp
```

## Command-line tool (`seamcli`)

```
seamcli in.png out.png 600 400 greedy                 # positional: width, height, algorithm
seamcli in.png out.png --algo dp --width 600          # only the width; height unchanged
seamcli in.png out.png --algo greedy --percent 75     # both sides to 75%
seamcli --bench testdata/landscape-tower.png --out results/bench_landscape-tower.csv
seamcli --help                                        # every option and its default
```

Algorithms: `dp` (default), `greedy`, `ek` (graph cut, Edmonds-Karp), `dinic`. Seam carving only shrinks, so targets must be between 1 and the current size.
Exit codes: 0 success, 1 runtime error (for example an unreadable image), 2 bad command line.

DP-versus-Dinic figures and the reproducible benchmark for measured Dinic phases,
EK augmentations and first-seam timings are documented in
[`docs/report/figures/c4/README.md`](docs/report/figures/c4/README.md).

`--bench` removes `--seams` (20) vertical seams from scaled copies of the image (`--scales`, default 0.125, 0.25, 0.5, 1) with each algorithm in `--algos`, `--repeats` (3) times, and writes the median run as CSV:

| Column | Meaning |
|--------|---------|
| `algorithm` | `dp`, `greedy`, `ek` or `dinic` |
| `width`, `height` | image size at that scale, before removing seams |
| `seams` | vertical seams removed (at most width - 1) |
| `ms` | median wall time of the run: energy map + seam finding + removal |
| `find_ms` | the seam-finding part of that run |
| `total_cost` | sum of the seam costs of every removed seam |

Algorithms that are not implemented yet are skipped with a note on stderr. A run slower than `--budget-ms` (10000) is stopped, gets no row, and the algorithm's larger sizes are skipped; this keeps Edmonds-Karp from running for hours. The notes for the committed results are in `results/bench_*.log`.

## Desktop app (`seamgui`)

The optional Dear ImGui + ImPlot frontend loads an image, resizes it on a worker thread,
compares the original and result, replays removed seams, and plots/export benchmarks.
First configuration downloads GLFW 3.4, Dear ImGui v1.91.9b and ImPlot v0.17.
An OpenGL 3.3 driver is required. The default headless build is unchanged.

On Windows, use [Visual Studio](#run-in-visual-studio-on-windows) or the
[Windows PowerShell](#windows-powershell) desktop preset.

Linux (Ubuntu/Debian):

```sh
sudo apt install build-essential cmake xorg-dev libgl1-mesa-dev
cmake -S . -B build-gui -DSEAM_BUILD_GUI=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-gui --parallel
ctest --test-dir build-gui --output-on-failure
./build-gui/gui/seamgui testdata/landscape-tower.png
```

Use a different build directory if one already contains another compiler's configuration.
Load an image by path or drag-and-drop; choose dimensions and an algorithm, then press
**Resize image**. Every run starts from the original. **Export image** saves the last
completed result. Output folders are created as needed; an existing output file is replaced.
The **Removed seams** tab offers playback, a step slider and an overview of all removed seams.
The **Benchmarks** tab runs the shared benchmark function, plots time against input pixel
count and exports exactly those measurements as CSV.

Cancellation waits for the current seam (resize) or algorithm (benchmark) to finish;
an in-flight max-flow call cannot be interrupted. Closing the app requests cancellation
and waits safely. All four algorithms, including Dinic, are available in the GUI.
GUI resize time includes seam tracking and the extra
energy calculation used to report seam cost; use benchmark measurements for comparisons.
See [`gui/README.md`](gui/README.md) for the code walkthrough and a one-minute demo.

## Team split (6 people)

Everyone must be able to explain every part: the quiz is team-based and can lower the whole team's multiplier.
Each person owns **one coding task and one report task**, so all 6 members touch both the code and the report.

Full task descriptions (what to do and what to deliver): [`docs/tasks/`](docs/tasks/README.md). Report drafts, one file per report task: [`docs/report/`](docs/report/README.md).

### Coding tasks

| # | Owner | Coding task |
|---|-------|-------------|
| C1 | Shee Yang | **Core + DP**: image I/O, Sobel energy, DP seam finder, resize loop, DP unit tests (hand-checked seams) |
| C2 | Wei Jun | **Greedy + CLI/benchmarks**: greedy seam finder, `seamcli` options, benchmark runner that writes CSV (time and seam cost per algorithm and image size) |
| C3 | Zhi Kai | **Graph cut network + Edmonds-Karp**: `FlowGraph`, flow network from the brief, seam extraction, Edmonds-Karp, tests that graph-cut seam cost equals DP seam cost |
| C4 | Dayton | **Dinic**: Dinic max-flow on C3's `FlowGraph`, Dinic seam finder, tests against Edmonds-Karp and DP, EK vs Dinic benchmark |
| C5 | Jarrel | **GUI**: Dear ImGui + ImPlot app, width/height sliders (bonus b), algorithm picker, benchmark plot with CSV export |
| C6 | Zhe An | **Visualisation + CI + improvements**: step frames with every seam drawn (bonus a), GitHub Actions (bonus c), DP improvements for 5d (rolling rows instead of the full table, multi-threading experiment) |

### Report tasks

| # | Owner | Report task |
|---|-------|-------------|
| R1 | Shee Yang | **Q2a (i) and (ii)** (exponential seam count, DP recurrence) + DP algorithm, correctness and time/space complexity (5b) |
| R2 | Wei Jun | **Q2b** (divide and conquer for Sobel) + greedy algorithm section + greedy vs DP comparison of seams, visual quality and running time (3, 5c) |
| R3 | Zhi Kai | **Graph cut formulation + Edmonds-Karp**: min-cut formulation, max-flow min-cut argument, last-column fix, EK complexity and correctness (4, 5b) |
| R4 | Dayton | **Dinic and graph cut vs DP**: Dinic algorithm and complexity, EK vs Dinic comparison, graph cut vs DP seams, quality, running time and memory (4, 5b, 5c) |
| R5 | Jarrel | **Introduction, system design and architecture, test images and evaluation method, GUI / user-interaction write-up (bonus b)**, references |
| R6 | Zhe An | **Front matter** (team ID, members with SIT IDs, GitHub and video links), **improvements and bonus** write-up (5d, bonus a and c), **AI usage declaration**, final assembly and export of the report |

Suggested pairing: person *n* does C*n* and R*n*, but any swap works as long as each person has exactly one of each.

## Presentation

Each person has 2 minutes (6 people x 2 = exactly the 12-minute team limit, so no one can overrun): present your own coding task and report section, and demo it running.

## Report checklist

Team ID and members with SIT IDs; Q2 answers; algorithm, design, implementation, results and comparison; GitHub link; video link; AI usage declaration (log in `docs/ai-usage.md`).
