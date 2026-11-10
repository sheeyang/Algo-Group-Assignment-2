#pragma once
#include <functional>

#include "seam.h"

namespace seam {

enum class Algorithm { DP, Greedy, GraphCutEK, GraphCutDinic };

// Called before each removal; `removed` is the seam in the coordinates of `before`
// (for a horizontal seam, removed[x] is the row removed in column x).
using StepCallback = std::function<void(const Image& before, const Seam& removed, bool vertical)>;

// Shrinks img to target_w x target_h by alternating one vertical and one horizontal
// seam removal until both targets are met (GLOSSARY.md: Resize).
Image resize(const Image& img, int target_w, int target_h, Algorithm algo,
             const StepCallback& on_step = nullptr);

}  // namespace seam
