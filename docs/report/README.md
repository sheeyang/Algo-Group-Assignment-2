# Report

One Markdown file per report task, so five people can write at the same time without merge conflicts.
Coding tasks live in [`docs/tasks/`](../tasks/README.md). Person *n* suggested pairing: C*n* + R*n*, and you write up the code you built.

| File | Owner | Covers |
|------|-------|--------|
| [R1-q2a-dp.md](R1-q2a-dp.md) | | Q2a (i) and (ii), DP algorithm, correctness, complexity |
| [R2-q2b-greedy.md](R2-q2b-greedy.md) | | Q2b, greedy algorithm, greedy vs DP |
| [R3-graph-cut.md](R3-graph-cut.md) | | Graph cut formulation, EK vs Dinic, graph cut vs DP |
| [R4-intro-design-gui.md](R4-intro-design-gui.md) | | Introduction, architecture, evaluation method, GUI (bonus b), references |
| [R5-improvements-bonus-front.md](R5-improvements-bonus-front.md) | | Front matter, improvements (5d), bonus a and c, AI declaration, final assembly |

## Final report structure (follows the submission instructions, item 9a)

The report is **one document** submitted to the "Assignment 2 Report Only" dropbox. R5 assembles it in this order:

| # | Section | Written in |
|---|---------|------------|
| 0 | Front matter: team ID, members with SIT ID, links to code and presentation video | R5 |
| 1 | Introduction and overview of seam carving | R4 |
| 2 | Answers to the Question (item 2): Q2a (i), Q2a (ii) | R1 |
|   | Q2b | R2 |
| 3 | Algorithms, design and implementation: system architecture | R4 |
|   | Dynamic programming | R1 |
|   | Greedy | R2 |
|   | Graph cut | R3 |
| 4 | Results and evaluation: test images and method | R4 |
|   | Results per algorithm | R1, R2, R3 |
| 5 | Analysis and discussion: time and space complexity (5b) | R1 (DP), R2 (greedy), R3 (graph cut) |
|   | Comparison with greedy and graph cut (5c) | R2 (greedy vs DP), R3 (graph cut vs DP), R5 (three-way summary) |
|   | Improvements (5d) | R5 |
| 6 | Bonus: step-by-step visualisation and CI/CD | R5; interactive GUI: R4 |
| 7 | Conclusion | R5 |
| 8 | AI usage declaration (how AI was used, and the main prompts) | R5, from `docs/ai-usage.md` |
| 9 | References | R4 (everyone sends R4 their sources) |

The rubric rewards: correct design with **justification and alternatives considered**, **proof of testing**, **in-depth analysis with no errors**, and documentation "beyond the requirement (e.g. references with comparative study)". The assignment says to **highlight the algorithm and analysis**: give those the most space.

## Writing rules

- Use the terms in [`GLOSSARY.md`](../../GLOSSARY.md) (seam, seam cost, energy map, ...). Do not invent synonyms.
- Every claim about speed or quality needs a number or an image from a CSV or figure in this repo, not from memory. Say which file it came from.
- Figures go in `figures/`, named `<section>-<what>.png` (for example `greedy-vs-dp-castle.png`).
- Put math in plain text or LaTeX-style inline (`O(mn)`); keep proofs step by step, because the quiz may show report extracts.
- Cite sources as `[n]` and send the full reference to R4. The assignment's own links are the starting list: the Avidan and Shamir paper, the Wikipedia article, the ACM papers and the avikdas.com article.
- Write in your own words and be able to explain every sentence: the project quiz is closed-book and based on the submitted report and code.
- If an AI tool helped with your section, log it in [`docs/ai-usage.md`](../ai-usage.md) as you go.

## Hand-off

Each file has a **Draft** section at the bottom: write there. When your draft is done, tell R5. R5 merges drafts into the final document and exports it to PDF (or Word) for XSite.
