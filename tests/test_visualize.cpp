#include <cstdio>
#include <cstdlib>
// CHECK() would vanish under -DNDEBUG (Release); these checks must always run.
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #c, __FILE__, __LINE__); std::abort(); } } while (0)
#include <cstdio>
#include <random>

#include "resize.h"
#include "visualize.h"

using namespace seam;

int main() {
    std::mt19937 rng(99);
    int checks = 0;
    for (int iter = 0; iter < 30; ++iter) {
        const int W = 12 + rng() % 40, H = 12 + rng() % 40;
        // Every pixel carries its own coordinates (r = x, g = y), so a seam pixel in ANY shrunk image
        // tells us which original pixel it is. Small random b makes the energy non-trivial.
        Image img(W, H);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) { uint8_t* p = img.px(x, y); p[0] = uint8_t(x); p[1] = uint8_t(y); p[2] = uint8_t(rng() & 255); }

        SeamTracker tr(W, H);
        std::vector<Image> before;
        const int tw = W - int(rng() % 8), th = H - int(rng() % 8);
        Image res = resize(img, tw, th, Algorithm::DP, [&](const Image& b, const Seam& s, bool v) {
            before.push_back(b);
            // Ground truth from the pixel coordinates stored in the pixels themselves.
            const int n = int(s.size());
            for (int i = 0; i < n; ++i) {
                const uint8_t* p = v ? b.px(s[i], i) : b.px(i, s[i]);
                CHECK(tr.removed_at(p[0], p[1]) == 0);  // no two seams share a pixel
            }
            tr.record(s, v);
            for (int i = 0; i < n; ++i) {
                const uint8_t* p = v ? b.px(s[i], i) : b.px(i, s[i]);
                CHECK(tr.removed_at(p[0], p[1]) == tr.steps());  // mapped to the right original pixel
                ++checks;
            }
        });
        // Number of removed original pixels == total seam length; kept pixels == final image size.
        long removed = 0, kept = 0;
        for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) (tr.removed_at(x, y) ? removed : kept)++;
        long total = 0;
        for (auto& r : tr.history()) total += long(r.seam.size());
        CHECK(removed == total && kept == long(res.width) * res.height);
        // Every pixel of the final image is a kept original pixel.
        for (int y = 0; y < res.height; ++y) for (int x = 0; x < res.width; ++x) {
            const uint8_t* p = res.px(x, y);
            CHECK(tr.removed_at(p[0], p[1]) == 0);
        }
        // Overview: removed pixels painted, others untouched.
        Image ov = tr.overview(img);
        for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
            const uint8_t* a = ov.px(x, y); const uint8_t* o = img.px(x, y);
            if (tr.removed_at(x, y)) { const bool v = tr.history()[tr.removed_at(x, y) - 1].vertical; CHECK(v ? (a[0] == 255 && a[1] == 0 && a[2] == 0) : (a[0] == 0 && a[1] == 200 && a[2] == 255)); }
            else CHECK(a[0] == o[0] && a[1] == o[1] && a[2] == o[2]);
        }
        // On-demand frames equal the frames from the callback.
        for (int k = 0; k < tr.steps(); ++k) {
            CHECK(replay(img, tr.history(), 0, k).rgb == before[k].rgb);
            CHECK(render_step(img, tr.history(), k).rgb ==
                   draw_seam(before[k], tr.history()[k].seam, tr.history()[k].vertical).rgb);
            ++checks;
        }
        CHECK(replay(img, tr.history(), 0, tr.steps()).rgb == res.rgb);
    }
    std::printf("test_visualize: all %d checks passed\n", checks);
    return 0;
}
