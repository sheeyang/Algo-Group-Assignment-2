# C1: Core + DP

**Owner:** ____________  **Report partner:** R1 ([`docs/report/R1-q2a-dp.md`](../report/R1-q2a-dp.md))
**Assignment items:** 1 (a, b, c), the correctness base for 3, 4 and 5

## Goal

Own the shared engine every other task builds on, and make the dynamic-programming seam finder provably correct for **both vertical and horizontal seams**.

## Starting point (already scaffolded)

`image.*`, `energy.*` (Sobel), `seam_dp.cpp` (`find_seam_dp`, `seam_cost`, `remove_vertical_seam`), `resize.*` (alternating vertical/horizontal loop), and three tests in `tests/test_dp.cpp`. It builds and passes. Your job is to review it, harden it, prove it, and extend it.

## What to do

1. **Review the scaffold line by line.** You are the owner of `image.h`, `energy.h`, `seam.h`, `resize.h`. Fix anything wrong.
2. **Edge cases**, each with a test:
   - width 1 (a vertical seam is the only column) and height 1; 1x1 image
   - ties: confirm the leftmost rule from `GLOSSARY.md` holds in the DP table fill **and** in backtracking
   - `resize` to the same size (no-op), to 1x1, only-width and only-height targets, and invalid targets (must throw)
   - `transpose` twice is the identity; a horizontal seam on image I equals a vertical seam on `transpose(I)`
3. **Prove the DP is optimal.** Write a brute-force test that enumerates every seam of small random `EnergyMap`s (e.g. up to 6x6) and checks `seam_cost(find_seam_dp(e)) ==` the minimum over all seams. Also check every returned seam is connected and inside the image.
4. **Test `sobel_energy`** on tiny images where the Gx and Gy values can be computed by hand: flat image gives zero energy, a vertical edge gives high energy only at the edge, border pixels use clamping.
5. **Test `resize`** end to end on a synthetic image: output dimensions are exactly the target, and with a `StepCallback` the number of calls equals the number of removed seams.
6. **Comments:** explain the recurrence `M[y][x] = e[y][x] + min(M[y-1][x-1], M[y-1][x], M[y-1][x+1])` and the backtracking in `seam_dp.cpp`.
7. **Stretch (only after the above):** forward energy as an optional switch (`GLOSSARY.md`: Forward energy). It changes the DP cost terms, so add it as a separate function (e.g. `find_seam_dp_forward`), not by editing the default path. Tell C3 if you do, because graph cut can use the same costs.

## Deliverables

- Reviewed and hardened `src/image.*`, `src/energy.*`, `src/seam_dp.cpp`, `src/resize.*`
- `tests/test_dp.cpp` (extend), `tests/test_energy.cpp`, `tests/test_resize.cpp`
- A one-page note for R1 (`docs/report/` partner): the recurrence, the tie-break rule, which edge cases you found, and the brute-force test result

## Acceptance checks

- `ctest` green on Windows and Linux; no compiler warnings
- Brute-force test covers at least 200 random maps and passes
- `seamcli in.png out.png <w> <h> dp` works on a real image for a width-only, height-only and combined target

## Interfaces others rely on

Do not change signatures in `seam.h`, `energy.h`, `image.h` or `resize.h` without telling the others. Adding is fine.
