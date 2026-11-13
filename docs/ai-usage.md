# AI Usage Log

The report needs an AI Usage Declaration (how AI tools were used, plus the main prompts).
Append an entry whenever AI is used, so we don't reconstruct it from memory.

| Date | Who | Tool | Phase (design / implementation / report) | Main prompt | What we kept / changed |
|------|-----|------|-------------------------------------------|-------------|------------------------|
| 2026-11-10 | | Claude Code | design + setup | `/grill-with-docs` "read the assignment requirements, and help me setup the project" | Chose C++17, ImGui/ImPlot, repo layout, glossary, ADRs; scaffold generated |
| 2026-11-10 | Shee Yang | Claude Code | design + implementation (C1) | `/grill-with-docs` "help me do C1" | Agreed 8 decisions (bad-input throws, tie rule judged bottom-up, double energies with integer-valued test maps, horizontal seam tested via `StepCallback`). AI wrote the validation and comments in `seam_dp.cpp`, `tests/test_dp.cpp`, `test_energy.cpp`, `test_image.cpp`, `test_resize.cpp`, and two glossary clarifications. Checked by mutating the DP and confirming the tests fail |
| 2026-11-10 | Shee Yang | Claude Code, ChatGPT | implementation (test data) | Claude Code drafted five image prompts (landscape, portrait, strong edges, person, greedy trap) and `testdata/README.md`; the images were then generated in ChatGPT with those prompts (see `testdata/README.md` for each) | Kept all five images; README records prompt, source and purpose per image |
| 2026-11-13 | Wei Jun | Claude Code | implementation (C2 greedy) | "explain what c2 is doing and the underlying algorithms" then "branch out and implement greedy" | AI wrote `find_seam_greedy` (leftmost tie rule, border handling, input validation) and `tests/test_greedy.cpp` (trap map where greedy costs 17 vs DP 6, borders, ties, 1000 random maps with greedy >= DP, greedy resize end to end). Checked by flipping the tie comparison and confirming 4 tests fail |
