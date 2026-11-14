// Plain-assert tests for the C6 improvements. Adapt to the project's test framework if needed.
#include <cstdio>
#include <cstdlib>
// CHECK() would vanish under -DNDEBUG (Release); these checks must always run.
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #c, __FILE__, __LINE__); std::abort(); } } while (0)
#include <cstdio>
#include <random>

#include "seam_dp_opt.h"

using namespace seam;

static EnergyMap random_map(int W, int H, std::mt19937& rng, int levels) {
    EnergyMap m;
    m.width = W; m.height = H; m.e.resize(size_t(W) * H);
    std::uniform_int_distribution<int> d(0, levels);
    for (auto& v : m.e) v = d(rng);
    return m;
}

static Image random_image(int W, int H, std::mt19937& rng) {
    Image im(W, H);
    for (auto& v : im.rgb) v = uint8_t(rng() & 255);
    return im;
}

int main() {
    std::mt19937 rng(12345);
    int checks = 0;
    // DP variants: same seam (hence same cost) as the baseline, incl. tie-heavy maps (levels=1) and tiny maps.
    for (int iter = 0; iter < 400; ++iter) {
        int W = 1 + rng() % 40, H = 1 + rng() % 40;
        int levels = (iter % 3 == 0) ? 1 : (iter % 3 == 1 ? 5 : 1000);
        EnergyMap e = random_map(W, H, rng, levels);
        Seam base = find_seam_dp(e);
        CHECK(find_seam_dp_lowmem(e) == base);
        for (int T : {1, 2, 3, 4, 8}) CHECK(find_seam_dp_threaded(e, T) == base);
        CHECK(seam_cost(e, find_seam_dp_lowmem(e)) == seam_cost(e, base));
        checks += 7;
    }
    // Energy: threaded == baseline; incremental after each removal == full recompute (bit-identical).
    for (int iter = 0; iter < 60; ++iter) {
        int W = 2 + rng() % 30, H = 1 + rng() % 30;
        Image img = random_image(W, H, rng);
        for (int T : {1, 2, 4}) CHECK(sobel_energy_threaded(img, T).e == sobel_energy(img).e);
        EnergyMap e = sobel_energy(img);
        Image cur = img;
        while (cur.width > 1) {
            Seam s = find_seam_dp(e);
            cur = remove_vertical_seam(cur, s);
            e = sobel_energy_after_removal(e, cur, s);
            CHECK(e.e == sobel_energy(cur).e);
            ++checks;
        }
    }
    // Incremental also on arbitrary (non-optimal) valid seams, to exercise borders.
    for (int iter = 0; iter < 200; ++iter) {
        int W = 2 + rng() % 12, H = 1 + rng() % 12;
        Image cur = random_image(W, H, rng);
        EnergyMap e = sobel_energy(cur);
        Seam s(H);
        s[0] = rng() % W;
        for (int y = 1; y < H; ++y) s[y] = std::min(W - 1, std::max(0, s[y - 1] + int(rng() % 3) - 1));
        cur = remove_vertical_seam(cur, s);
        CHECK(sobel_energy_after_removal(e, cur, s).e == sobel_energy(cur).e);
        ++checks;
    }
    // Whole pipeline: every option combination gives the identical image.
    for (int iter = 0; iter < 20; ++iter) {
        Image img = random_image(10 + rng() % 30, 5 + rng() % 30, rng);
        int k = img.width / 2;
        Image ref = shrink_width(img, k, {});
        for (bool lm : {false, true})
            for (bool inc : {false, true})
                for (int T : {1, 2, 4}) {
                    OptOptions o; o.lowmem = lm; o.incremental = inc; o.threads = T;
                    CHECK(shrink_width(img, k, o).rgb == ref.rgb);
                    ++checks;
                }
    }
    std::printf("test_improvements: all %d checks passed\n", checks);
    return 0;
}
