# C5: Dear ImGui + ImPlot GUI

**Owner:** Jarrel  **Report partner:** R5 ([`docs/report/R5-intro-design-gui.md`](../report/R5-intro-design-gui.md))
**Assignment items:** bonus 7b (user interaction), the demo for 8c, and the results charts

## Goal

A native desktop app where the user opens an image, changes its width and height interactively, picks an algorithm, and sees the result, timing and benchmark charts. It is the demo for the presentation. Decision record: [`docs/adr/0002-native-imgui-frontend.md`](../adr/0002-native-imgui-frontend.md).

## What to do

1. **Build setup** in `gui/CMakeLists.txt`: pull Dear ImGui, ImPlot and GLFW with CMake `FetchContent`, each **pinned to a release tag** (not a branch), OpenGL3 backend. Build only when `-DSEAM_BUILD_GUI=ON` so CI and teammates without GL stay unaffected. Target `seamgui` links `seamlib` only.
2. **Main window** (`gui/main.cpp`, split into more files as it grows):
   - load an image (path box and/or drag-and-drop via GLFW) and show original and result side by side as OpenGL textures
   - width and height sliders (1 to current size) and an algorithm selector (DP, greedy, graph cut EK/Dinic)
   - run the resize on a **worker thread** with progress and a cancel option, so the UI never freezes; show elapsed time and total seam cost
   - handle errors visibly (bad file, algorithm not implemented yet) instead of crashing
3. **Benchmark panel:** call C2's `run_benchmark`, plot time versus image size per algorithm with **ImPlot**, and an **Export CSV** button that writes the same rows (`write_csv`).
4. **Step-through panel:** show the seams removed one at a time using C6's rendering API (agree the interface with C6, see `docs/tasks/C6-visualisation-ci-improvements.md`). Until C6 lands, stub it.
5. Keep all image and seam logic in `seamlib`; the GUI only calls it and draws.

## Deliverables

- `gui/` with the `seamgui` target, buildable on Windows (MinGW) and Linux with the instructions in `README.md`
- A short "How to build and use the GUI" section added to `README.md`
- Screenshots in `docs/report/figures/` (main window, resize in progress, benchmark plot) for R5
- A 1-minute demo script for the presentation

## Acceptance checks

- Dragging a slider and pressing resize changes the output size to exactly the slider values
- The UI stays responsive during a long resize
- Exported CSV opens in a spreadsheet and matches the plot
- `cmake -S . -B build -DSEAM_BUILD_GUI=ON` works on a clean clone (first configure needs internet)

## Dependencies

- C2 for `run_benchmark` / `write_csv` (agree the header early; stub until ready)
- C6 for step rendering
- Greedy and graph cut may not exist yet: the GUI must cope with "not implemented" gracefully
