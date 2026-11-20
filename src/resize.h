/**
 * @brief Algorithm selection, progress callback and content-aware resize API.
 * @file resize.h
 */
#pragma once
#include <functional>

#include "seam.h"

namespace seam {

/** @brief Select full-table DP, local greedy, EK min-cut or Dinic min-cut. */
enum class Algorithm { DP, Greedy, GraphCutEK, GraphCutDinic };

/**
 * @brief Optional synchronous observer called before each seam is removed.
 * @details before is the current unmodified image. For a vertical seam,
 * removed[y] is a column; for a horizontal seam, removed[x] is a row in before,
 * not a column in a transposed image. References are borrowed for this call.
 * Throwing aborts the resize; the input supplied to resize remains unchanged.
 */
using StepCallback = std::function<void(const Image& before, const Seam& removed, bool vertical)>;

// Shrinks img to target_w x target_h by alternating one vertical and one horizontal
// seam removal until both targets are met (GLOSSARY.md: Resize).
/**
 * @brief Remove seams until both requested dimensions are reached.
 * @param img RGB image with storage matching its dimensions.
 * @param target_w Requested final width, between one and the original width.
 * @param target_h Requested final height, between one and the original height.
 * @param algo Seam-finding algorithm.
 * @param on_step Optional callback before each removal; receives the current image, selected seam
 * and orientation. Exceptions propagate.
 * @return Resized RGB copy; input is unchanged.
 * @throws std::invalid_argument If either target lies outside 1..original dimension.
 * @note Mixed targets alternate vertical and horizontal removal. Energy is recomputed each time.
 * Callback exceptions and solver errors propagate.
 * @see StepCallback
 */
Image resize(const Image& img, int target_w, int target_h, Algorithm algo,
             const StepCallback& on_step = nullptr);

}  // namespace seam
