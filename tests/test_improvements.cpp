// C6 improvements (5d): every variant must return exactly what the baseline returns.
#include <random>

#include "doctest.h"
#include "seam_dp_opt.h"

using namespace seam;

namespace {

EnergyMap random_map(int W, int H, std::mt19937& rng, int levels) {
    EnergyMap m;
    m.width = W;
    m.height = H;
    m.e.resize(size_t(W) * H);
    std::uniform_int_distribution<int> d(0, levels);
    for (auto& v : m.e) v = d(rng);
    return m;
}

Image random_image(int W, int H, std::mt19937& rng) {
    Image im(W, H);
    for (auto& v : im.rgb) v = uint8_t(rng() & 255);
    return im;
}

}  // namespace

TEST_CASE("low-memory and threaded DP return the baseline seam") {
    std::mt19937 rng(12345);
    for (int iter = 0; iter < 400; ++iter) {
        const int W = 1 + int(rng() % 40), H = 1 + int(rng() % 40);
        // levels = 1 makes almost every cell a tie, which exercises the tie-breaking rule.
        const int levels = (iter % 3 == 0) ? 1 : (iter % 3 == 1 ? 5 : 1000);
        const EnergyMap e = random_map(W, H, rng, levels);
        const Seam base = find_seam_dp(e);
        CHECK(find_seam_dp_lowmem(e) == base);
        CHECK(seam_cost(e, find_seam_dp_lowmem(e)) == seam_cost(e, base));
        for (int T : {1, 2, 3, 4, 8}) CHECK(find_seam_dp_threaded(e, T) == base);
    }
}

TEST_CASE("threaded energy equals sobel_energy") {
    std::mt19937 rng(7);
    for (int iter = 0; iter < 60; ++iter) {
        const Image img = random_image(2 + int(rng() % 30), 1 + int(rng() % 30), rng);
        for (int T : {1, 2, 4}) CHECK(sobel_energy_threaded(img, T).e == sobel_energy(img).e);
    }
}

TEST_CASE("incremental energy equals a full recompute after every removal") {
    std::mt19937 rng(8);
    for (int iter = 0; iter < 60; ++iter) {
        Image cur = random_image(2 + int(rng() % 30), 1 + int(rng() % 30), rng);
        EnergyMap e = sobel_energy(cur);
        while (cur.width > 1) {
            const Seam s = find_seam_dp(e);
            cur = remove_vertical_seam(cur, s);
            e = sobel_energy_after_removal(e, cur, s);
            CHECK(e.e == sobel_energy(cur).e);  // bit-identical, not just close
        }
    }
}

TEST_CASE("incremental energy is correct for arbitrary valid seams (borders)") {
    std::mt19937 rng(9);
    for (int iter = 0; iter < 200; ++iter) {
        const int W = 2 + int(rng() % 12), H = 1 + int(rng() % 12);
        Image cur = random_image(W, H, rng);
        const EnergyMap e = sobel_energy(cur);
        Seam s(H);
        s[0] = int(rng() % unsigned(W));
        for (int y = 1; y < H; ++y) {
            const int step = int(rng() % 3) - 1;
            s[y] = s[y - 1] + step < 0 ? 0 : (s[y - 1] + step > W - 1 ? W - 1 : s[y - 1] + step);
        }
        cur = remove_vertical_seam(cur, s);
        CHECK(sobel_energy_after_removal(e, cur, s).e == sobel_energy(cur).e);
    }
}

TEST_CASE("every option combination of shrink_width gives the identical image") {
    std::mt19937 rng(10);
    for (int iter = 0; iter < 20; ++iter) {
        const Image img = random_image(10 + int(rng() % 30), 5 + int(rng() % 30), rng);
        const int k = img.width / 2;
        const Image ref = shrink_width(img, k, OptOptions{});
        for (bool lm : {false, true})
            for (bool inc : {false, true})
                for (int T : {1, 2, 4}) {
                    OptOptions o;
                    o.lowmem = lm;
                    o.incremental = inc;
                    o.threads = T;
                    CHECK(shrink_width(img, k, o).rgb == ref.rgb);
                }
    }
}
