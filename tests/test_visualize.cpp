// C6 visualisation (7a): seams mapped back to original coordinates, step rendering.
#include <random>

#include "doctest.h"
#include "resize.h"
#include "visualize.h"

using namespace seam;

TEST_CASE("seams map back to original coordinates; no two seams share a pixel") {
    std::mt19937 rng(99);
    for (int iter = 0; iter < 30; ++iter) {
        const int W = 12 + int(rng() % 40), H = 12 + int(rng() % 40);
        // Every pixel carries its own coordinates (r = x, g = y), so a seam pixel of ANY shrunk image
        // tells us which original pixel it is. A random b keeps the energy non-trivial.
        Image img(W, H);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) {
                uint8_t* p = img.px(x, y);
                p[0] = uint8_t(x);
                p[1] = uint8_t(y);
                p[2] = uint8_t(rng() & 255);
            }

        SeamTracker tr(W, H);
        std::vector<Image> before;
        const int tw = W - int(rng() % 8), th = H - int(rng() % 8);
        const Image res = resize(img, tw, th, Algorithm::DP, [&](const Image& b, const Seam& s, bool v) {
            before.push_back(b);
            const int n = int(s.size());
            for (int i = 0; i < n; ++i) {
                const uint8_t* p = v ? b.px(s[i], i) : b.px(i, s[i]);
                CHECK(tr.removed_at(p[0], p[1]) == 0);  // not removed by an earlier seam
            }
            tr.record(s, v);
            for (int i = 0; i < n; ++i) {
                const uint8_t* p = v ? b.px(s[i], i) : b.px(i, s[i]);
                CHECK(tr.removed_at(p[0], p[1]) == tr.steps());  // mapped to the right original pixel
            }
        });

        // Removed originals == total seam length; kept originals == size of the result.
        long removed = 0, kept = 0, total = 0;
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) (tr.removed_at(x, y) ? removed : kept)++;
        for (const auto& r : tr.history()) total += long(r.seam.size());
        CHECK(removed == total);
        CHECK(kept == long(res.width) * res.height);
        // Every pixel of the result is an original pixel that was never removed.
        for (int y = 0; y < res.height; ++y)
            for (int x = 0; x < res.width; ++x) CHECK(tr.removed_at(res.px(x, y)[0], res.px(x, y)[1]) == 0);

        // Overview: removed pixels get the seam colour, all others are untouched.
        const Image ov = tr.overview(img);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) {
                const uint8_t* a = ov.px(x, y);
                const uint8_t* o = img.px(x, y);
                if (const int k = tr.removed_at(x, y)) {
                    if (tr.history()[size_t(k) - 1].vertical) CHECK((a[0] == 255 && a[1] == 0 && a[2] == 0));
                    else CHECK((a[0] == 0 && a[1] == 200 && a[2] == 255));
                } else {
                    CHECK((a[0] == o[0] && a[1] == o[1] && a[2] == o[2]));
                }
            }

        // On-demand frames equal the frames seen in the callback.
        for (int k = 0; k < tr.steps(); ++k) {
            CHECK(replay(img, tr.history(), 0, k).rgb == before[size_t(k)].rgb);
            CHECK(render_step(img, tr.history(), k).rgb ==
                  draw_seam(before[size_t(k)], tr.history()[size_t(k)].seam, tr.history()[size_t(k)].vertical).rgb);
        }
        CHECK(replay(img, tr.history(), 0, tr.steps()).rgb == res.rgb);
    }
}
