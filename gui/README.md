# C5 code walkthrough

Build/run instructions are in the root README's **Desktop app** section.
Start from the repository root so relative paths such as `testdata/landscape-tower.png` work.

## Read these files in order

1. **CMakeLists.txt** downloads three fixed library releases and builds `seamgui` only
   when `SEAM_BUILD_GUI=ON`. GLFW creates the native window and OpenGL context;
   ImGui draws controls; ImPlot draws benchmark charts.
2. **main.cpp** owns the window, controls, OpenGL textures and application state.
   `App::load` calls `load_image`; `controls` starts a resize; `poll` receives a completed
   result; `preview`, `playback` and `benchmark` draw the three tabs. OpenGL work stays
   on the main thread because its context belongs to that thread.
3. **jobs.h / jobs.cpp** run the expensive work through `std::async`. The worker owns
   a copy of the input image and returns one `JobResult`. Progress and cancellation
   use atomics, so the UI never reads a result vector while the worker is writing it.
4. **test_jobs.cpp** checks mixed-direction resizing against the existing engine,
   recorded seam cost and replay, cancellation/reuse, error recovery, benchmarks and
   shutdown while work is active. These tests do not need an OpenGL window.

## Follow one resize

- The user chooses a width, height and algorithm and clicks **Resize image**.
- `Jobs::start_resize` copies the original into a background task and calls
  `seam::resize`. All energy computation, seam finding and seam removal remain in `seamlib`.
- The shared step callback checks cancellation, measures seam cost with `sobel_energy`
  and `seam_cost`, and calls C6's `SeamTracker::record`.
- The worker returns the resized image, seam history, overview, elapsed time and cost.
- On a later frame, `App::poll` takes the ready result and uploads textures. Until then,
  the window keeps drawing and the last completed result stays visible.

The callback does not expose the engine's energy map, so cost measurement computes a
second map. GUI timing includes this overhead and tracking; the benchmark panel uses
the unmodified shared `run_benchmark` for comparable algorithm timings.

## Follow seam playback

The GUI stores the original, a list of removed seams, and one replay checkpoint.
`replay` reconstructs the image before the selected removal and `draw_seam` paints
that seam. Moving forward reuses the checkpoint; seeking backward starts again from
the original. It does not keep a full RGB image for every step. The overview comes
from `SeamTracker::overview`, which maps removed pixels back to the original image.

## Benchmarks and cancellation

Each selected algorithm is passed to C2's `run_benchmark` across all four scales.
This preserves its median-of-repeats measurement and time-budget rules, including
skipping larger sizes of an algorithm after a smaller size exceeds the budget.
Both the plot and CSV read the resulting `BenchRow` vector.

Resize cancellation is checked between seams. Benchmark cancellation is checked
between algorithms, preserving complete benchmark rows. Neither can interrupt a
single running max-flow call. A cancelled resize leaves the last completed result
intact; a stopped benchmark keeps completed measurements. Closing requests cancellation
and joins the worker before destroying its state. DP, greedy, Edmonds-Karp and
Dinic are available for both resizing and benchmarks. Use small inputs for graph
cut demonstrations because a single max-flow call can take a long time.

## One-minute demo

- **0-10 s:** Launch with `testdata/landscape-tower.png`; identify original and result panels.
- **10-25 s:** Select DP, choose 75% width with the height unchanged, and resize.
  Show the output dimensions, elapsed time and total seam cost; export the image.
- **25-40 s:** Open **Removed seams**, play a few steps, then show the all-seams overview.
- **40-55 s:** Open **Benchmarks**, keep DP and greedy selected, run 10 seams with 3 repeats,
  and explain that each point represents the median time at one input size. Export CSV.
- **55-60 s:** Explain that the GUI calls the same engine as the command-line tool and
  the background task lets the controls remain responsive during resizing.

For a reproducible cancellation demonstration, start a larger DP resize and press
**Cancel** while the progress bar is moving. For graph cut, use a small image first.
