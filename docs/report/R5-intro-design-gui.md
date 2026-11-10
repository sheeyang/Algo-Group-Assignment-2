# R5: Introduction, design, evaluation method, GUI, references

**Owner:** Jarrel  **Code partner:** C5 ([`docs/tasks/C5-gui.md`](../tasks/C5-gui.md))
**Assignment items:** report structure (9a iii), bonus 7b, references

## What to write

### 1. Introduction

- What seam carving is and why it beats scaling and cropping (the assignment's three-image example), with the Avidan and Shamir idea in a few sentences.
- The problem statement for this project, and an overview of the report.

### 2. System design and architecture

- The three targets and why: `seamlib` (engine), `seamcli` (headless), `seamgui` (ImGui/ImPlot). Draw a component diagram and the data flow of one resize (image -> energy map -> seam -> remove -> repeat).
- Key data types (`Image`, `EnergyMap`, `Seam`) and the common seam-finder interface that lets DP, greedy and graph cut be swapped.
- Design decisions and their reasons, taken from [`docs/adr/`](../adr/): C++17 with no OpenCV (and what was considered), the native ImGui frontend, the alternating vertical/horizontal rule, shrinking only.
- Build, test and CI setup in one short paragraph (details in R6's bonus section).

### 3. Test images and evaluation method

- The test images (source, size, why chosen) and the synthetic hand-checkable cases.
- How we measure: time with a steady clock, median of repeats, machine and compiler used (CPU, RAM, OS, g++ version, optimisation flags), what "seam cost" and "visual quality" mean (criteria used when judging by eye).
- This section is the shared method for R1, R2, R3, R6 and R4. Agree it with them early; get the machine details from whoever runs the benchmarks.

### 4. Interactive GUI (bonus b)

- What the user can do (load, resize with sliders, choose algorithm, run, see timing, benchmark plot, CSV export, step-through) with screenshots.
- How it is built: ImGui and ImPlot on GLFW + OpenGL3, the worker thread that keeps the UI responsive, how the GUI reuses `seamlib` without duplicating logic.
- Limitations and what could be improved.

### 5. References

- Collect references from R1, R2, R3, R6 and R4. Start with the assignment's links (Avidan and Shamir, the Wikipedia article, the two ACM papers, the avikdas.com article), then add textbook sources for DP, greedy, max-flow, Edmonds-Karp, Dinic and Sobel. One consistent citation style.

## Needs from the code partner (C5)

- GUI screenshots in `figures/`; the build instructions; the demo script.

## Figures and tables

- Architecture diagram; data-flow diagram; test image table; GUI screenshots.

## Draft

(Write here. Suggested headings: `## Introduction`, `## System design`, `## Evaluation method`, `## Interactive GUI`, `## References`)
