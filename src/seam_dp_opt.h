#pragma once
// C6 improvements (assignment 5d). Variants live next to the baseline; find_seam_dp is untouched.
#include <cstdint>

#include "seam.h"

namespace seam {

// (1) Do we need the whole DP table? No: two cost rows (O(W) doubles) + one int8 direction per cell.
// Returns exactly the same seam as find_seam_dp (same tie-breaking).
Seam find_seam_dp_lowmem(const EnergyMap& e);

// (3) Same result, cells of one DP row split across `threads` threads (barrier between rows).
// threads <= 1 falls back to find_seam_dp_lowmem.
Seam find_seam_dp_threaded(const EnergyMap& e, int threads);

// (3) Same result as sobel_energy, rows split across `threads` threads.
EnergyMap sobel_energy_threaded(const Image& img, int threads);

// (2) Incremental energy. `old_e` is the energy map of the image BEFORE removing vertical seam
// `removed`; `new_img` is the image AFTER the removal. Returns exactly sobel_energy(new_img),
// recomputing only the pixels whose 3x3 neighbourhood touched the seam (<= 6 per row).
EnergyMap sobel_energy_after_removal(const EnergyMap& old_e, const Image& new_img, const Seam& removed);

// Time spent per stage, accumulated over all seams (seconds).
struct StageTimes {
    double energy = 0, dp = 0, remove = 0;
};

struct OptOptions {
    bool lowmem = false;       // find_seam_dp_lowmem instead of find_seam_dp
    bool incremental = false;  // incremental energy update instead of full recompute
    int threads = 1;           // >1: threaded energy (full recompute only) and, if thread_dp, threaded DP
    bool own_energy = false;   // use sobel_energy_threaded (same maths, own code) even with threads == 1
    bool thread_dp = true;     // false: thread only the energy rows, keep the DP serial
};

// Removes `k` vertical seams one after another with the chosen variants (all-false = baseline).
// Result is identical for every option combination.
Image shrink_width(const Image& img, int k, const OptOptions& opt, StageTimes* times = nullptr);

}  // namespace seam
