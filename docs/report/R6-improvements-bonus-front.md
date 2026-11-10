# R6: Front matter, improvements, bonus, AI declaration, assembly

**Owner:** Zhe An  **Code partner:** C6 ([`docs/tasks/C6-visualisation-ci-improvements.md`](../tasks/C6-visualisation-ci-improvements.md))
**Assignment items:** 5d, bonus 7a and 7c, 5c (three-way summary), 9a (i, iv, v), final assembly

## What to write

### 1. Front matter (9a i and iv)

- Team ID, and every member's name with **SIT ID**.
- Links: GitHub repository (make sure it is accessible to the marker) and the presentation video (YouTube or similar). Add the video link once it is recorded.
- Title, date, and a one-paragraph abstract of what was done.

### 2. Improvements (5d)

The brief asks: "do we need to maintain the entire DP Table?, does multi-threading help?"
- **DP table:** what is actually needed (two rows for the cost, a direction value per cell for backtracking), the memory saved, and the time effect, with C6's measurements.
- **Incremental energy update:** idea, complexity gain per seam, results.
- **Multi-threading:** what was parallelised (energy rows, DP cells within a row), speedup per image size and thread count. Explain **why** small images may not speed up (thread overhead, memory-bound work) and the limit from the sequential dependency between DP rows.
- Report negative results honestly. Every claim needs a number from `results/improvements_*.csv`.

### 3. Bonus write-up (7a and 7c)

- **Step-by-step visualisation:** how the frames and the all-seams overview are produced (including mapping seams back to original coordinates), with sample images. (The interactive GUI part is in R5.)
- **CI/CD:** what the workflow does (build and test on two operating systems, smoke test, optional GUI build, artifacts, badge), with a screenshot of a green run.

### 4. Three-way comparison and conclusion (5c summary)

- A summary table of DP, greedy, graph cut (EK and Dinic): optimal or not, time and space, measured speed, visual quality, and when to use each. Pull the evidence from R1, R2, R3 and R4; do not repeat their sections.
- Conclusions and lessons learned.

### 5. AI usage declaration (9a v)

- Build it from [`docs/ai-usage.md`](../ai-usage.md): which AI tools were used, at which stage (design, implementation, report writing), what was kept or changed, and **the main prompts used**. Remind teammates to log as they go; chase missing entries a few days before the deadline.

### 6. Final assembly and submission

- Merge R1 to R4 into the order in [`README.md`](README.md); unify terms with `GLOSSARY.md`, numbering, figure captions and references; check every figure referenced exists.
- Run the checklist in the next section, export to PDF (or Word), and submit to **XSite Assignment 2 Report Only**. Put the slides and test inputs/outputs in **Assignment 2 Other Materials**.

## Submission checklist (deadline Fri 20 Nov, 23:59)

- [ ] Team ID and all members with SIT IDs
- [ ] Answers to the Question (item 2): 2a (i), 2a (ii), 2b
- [ ] Algorithm, design, implementation, results/evaluation, comparison and discussion
- [ ] Time and space complexity (5b); comparison with greedy and graph cut (5c); improvements (5d)
- [ ] GitHub link (opens for the marker) and video link (opens, under the time limit: 2 minutes per person, 12 minutes for the team)
- [ ] AI usage declaration with the main prompts
- [ ] Slides and test inputs/outputs in the "Other Materials" dropbox
- [ ] Everyone can explain the report and the code for the quiz on Tue 24 Nov

## Needs from the code partner (C6)

- `results/improvements_*.csv`; step-by-step sample images; CI run screenshot.

## Draft

(Write here. Suggested headings: `## Front matter`, `## Improvements`, `## Bonus`, `## Comparison and conclusion`, `## AI usage declaration`)
