# C3 test images

`castle.jpeg` is an unchanged copy of the photo supplied by Zhi Kai as
`Assignment 2.jpeg` on 13 November 2026. It shows a stone tower, sky, grass,
and a small person. The original photographer and licence were not supplied.
This photo is separate from the five generated images listed in the parent
README.

`tests/test_graphcut_photo.cpp` loads the JPEG and samples the whole scene to
64x43 pixels in memory to keep Edmonds-Karp fast. It removes two vertical and
two horizontal seams, checking each seam's cost against DP, connectivity,
bounds, and the final 62x41 dimensions. The original JPEG is preserved.

Run only this test after building:

```powershell
.\build-c3\tests\seam_tests.exe --test-case="*castle photo*"
```

To inspect one vertical seam removal at the original resolution, run:

```powershell
.\build-c3\cli\seamcli.exe testdata/c3/castle.jpeg build-c3/castle-ek.png 273 186 ek
.\build-c3\cli\seamcli.exe testdata/c3/castle.jpeg build-c3/castle-dp.png 273 186 dp
```

The original is 274x186. These outputs remove only one column, so the visible
change will be small. Full-resolution EK can take more than a minute for one
seam, so use the automated test above for quick correctness checks. Generated
outputs belong in the ignored build directory, not in this fixture folder.

## Small portrait and building inputs

These 64x43 PNGs are small copies of the existing repository images, prepared
for practical DP-versus-EK seam comparisons. They use `scale_image()` from
`src/bench.cpp`: box averaging at scale 0.08, preserving the aspect ratio
apart from rounding the height. This preparation is normal image scaling;
no seams have been removed from these inputs. Source images are unchanged.

| Small input | Repository source |
|-------------|-------------------|
| `portrait-small.png` | `testdata/portrait-red-jacket.png` |
| `building-small.png` | `testdata/edges-glass-building.png` |

The sources are team-generated images; their provenance and prompts are in
[`../README.md`](../README.md). The first-seam EK and DP costs were checked
to agree within 1e-6 on both small inputs.

Example: remove ten vertical seams from the portrait and save red seam frames:

```powershell
.\build\release\cli\seamcli.exe testdata/c3/portrait-small.png build/release/portrait-ek.png --algo ek --width 54 --steps build/release/portrait-ek-steps
.\build\release\cli\seamcli.exe testdata/c3/portrait-small.png build/release/portrait-dp.png --algo dp --width 54 --steps build/release/portrait-dp-steps
```

Repeat with `building-small.png` and corresponding output filenames. Each
steps folder contains `step_0001.png` and an `overview.png` of removed pixels.

Completed report figures and their regeneration instructions are in
[`docs/report/figures/c3/README.md`](../../docs/report/figures/c3/README.md).
