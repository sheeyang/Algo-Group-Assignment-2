# C3 castle photo

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
