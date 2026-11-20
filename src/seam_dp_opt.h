/**
 * @brief Low-memory, threaded and incremental-energy experiment interfaces.
 * @file seam_dp_opt.h
 */
#pragma once
// C6 improvements (assignment 5d). Variants live next to the baseline; find_seam_dp is untouched.
#include <cstdint>

#include "seam.h"

namespace seam {

// (1) Do we need the whole DP table? No: two cost rows (O(W) doubles) + one int8 direction per cell.
// Returns exactly the same seam as find_seam_dp (same tie-breaking).
/**
 * @brief Find the baseline DP seam using rolling costs and byte directions.
 * @param e Input row-major energy map.
 * @return Same seam and tie rule as full-table DP.
 * @throws std::invalid_argument For an empty or inconsistent map.
 * @par Complexity
 * O(W H) time; O(W) double costs plus O(W H) byte directions.
 */
Seam find_seam_dp_lowmem(const EnergyMap& e);

// (3) Same result, cells of one DP row split across `threads` threads (barrier between rows).
// threads <= 1 falls back to find_seam_dp_lowmem.
/**
 * @brief Compute disjoint cell ranges in each DP row with a barrier between rows.
 * @param e Input row-major energy map.
 * @param threads Requested worker count; serial fallback applies when the effective count is one.
 * @return Same seam as serial DP; small or single-thread cases use low-memory DP.
 * @throws std::invalid_argument For an empty or inconsistent map.
 * @note Workers are capped by width and 256. Row dependencies prevent parallelising across rows.
 */
Seam find_seam_dp_threaded(const EnergyMap& e, int threads);

// (3) Same result as sobel_energy, rows split across `threads` threads.
/**
 * @brief Compute the baseline Sobel expression on disjoint row ranges.
 * @param img RGB image with storage matching its dimensions.
 * @param threads Requested worker count; serial fallback applies when the effective count is one.
 * @return Same energy values as baseline Sobel for valid input.
 * @pre img has positive dimensions and consistent RGB storage.
 * @note Worker count is clamped to 1..height; workers are joined before return.
 */
EnergyMap sobel_energy_threaded(const Image& img, int threads);

// (2) Incremental energy. `old_e` is the energy map of the image BEFORE removing vertical seam
// `removed`; `new_img` is the image AFTER the removal. Returns exactly sobel_energy(new_img),
// recomputing only the pixels whose 3x3 neighbourhood touched the seam (<= 6 per row).
/**
 * @brief Shift cached energy and recompute pixels whose Sobel neighbourhood changed.
 * @param old_e Cached energy map before the vertical seam removal.
 * @param new_img Image after the vertical seam has been removed.
 * @param removed Vertical seam expressed in the old image coordinates.
 * @return Energy map for the new image.
 * @pre removed seam coordinates are valid for the old image; both maps have consistent storage.
 * @throws std::invalid_argument If old/new dimensions or seam length do not match.
 * @note At most six candidate columns per row are recomputed. Shifting/copying still costs O(W H).
 */
EnergyMap sobel_energy_after_removal(const EnergyMap& old_e, const Image& new_img, const Seam& removed);

// Time spent per stage, accumulated over all seams (seconds).
/** @brief Per-stage seconds accumulated over one shrink_width invocation. */
struct StageTimes {
    double energy = 0, dp = 0, remove = 0;
};

/**
 * @brief Independent options for controlled DP/energy experiments.
 * @details Incremental energy is used after the first removal. Threaded DP
 * takes precedence over lowmem when enabled with more than one thread.
 */
struct OptOptions {
    bool lowmem = false;       ///< find_seam_dp_lowmem instead of find_seam_dp
    bool incremental = false;  ///< incremental energy update instead of full recompute
    int threads = 1;           ///< >1: threaded energy (full recompute only) and, if thread_dp, threaded DP
    bool own_energy = false;   ///< use sobel_energy_threaded (same maths, own code) even with threads == 1
    bool thread_dp = true;     ///< false: thread only the energy rows, keep the DP serial
};

// Removes `k` vertical seams one after another with the chosen variants (all-false = baseline).
// Result is identical for every option combination.
/**
 * @brief Remove k vertical seams with selected DP and energy variants.
 * @param img RGB image with storage matching its dimensions.
 * @param k Number of vertical seams to remove; must leave at least one column.
 * @param opt Optimisation flags selecting DP and energy variants.
 * @param times Optional destination for accumulated stage times in seconds.
 * @return Image narrowed by k columns; input is unchanged.
 * @throws std::invalid_argument If k is negative or would remove every column.
 * @note Stage times are replaced, not appended, when requested; values are seconds.
 */
Image shrink_width(const Image& img, int k, const OptOptions& opt, StageTimes* times = nullptr);

}  // namespace seam
