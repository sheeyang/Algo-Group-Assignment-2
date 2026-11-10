#include "resize.h"

#include <stdexcept>

namespace seam {

static Seam find(const EnergyMap& e, Algorithm a) {
    switch (a) {
        case Algorithm::DP: return find_seam_dp(e);
        case Algorithm::Greedy: return find_seam_greedy(e);
        case Algorithm::GraphCutEK: return find_seam_graphcut_ek(e);
        case Algorithm::GraphCutDinic: return find_seam_graphcut_dinic(e);
    }
    throw std::logic_error("unknown algorithm");
}

Image resize(const Image& img, int target_w, int target_h, Algorithm algo,
             const StepCallback& on_step) {
    if (target_w < 1 || target_h < 1 || target_w > img.width || target_h > img.height)
        throw std::invalid_argument("target size must be within 1..current size");
    Image cur = img;
    bool vertical = true;
    while (cur.width > target_w || cur.height > target_h) {
        if (vertical && cur.width == target_w) vertical = false;
        if (!vertical && cur.height == target_h) vertical = true;
        if (vertical) {
            Seam s = find(sobel_energy(cur), algo);
            if (on_step) on_step(cur, s, true);
            cur = remove_vertical_seam(cur, s);
        } else {
            Image t = transpose(cur);
            Seam s = find(sobel_energy(t), algo);
            if (on_step) on_step(cur, s, false);
            cur = transpose(remove_vertical_seam(t, s));
        }
        vertical = !vertical;
    }
    return cur;
}

}  // namespace seam
