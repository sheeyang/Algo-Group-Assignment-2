// Whole-pipeline comparison of variant combinations: remove k vertical seams, ms per seam.
// Usage: bench_combined <out_dir>   -> improvements_combined.csv
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "seam_dp_opt.h"

using namespace seam;
using Clock = std::chrono::steady_clock;

static Image synthetic_image(int W, int H, unsigned seed) {  // identical to bench_improvements.cpp
    std::mt19937 rng(seed);
    std::normal_distribution<double> noise(0, 6);
    Image im(W, H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            double v = 128 + 70 * std::sin(x * 0.045) * std::cos(y * 0.031) + 30 * std::sin((x + y) * 0.013);
            if ((x / 97 + y / 61) % 5 == 0) v += 45;
            for (int c = 0; c < 3; ++c) im.px(x, y)[c] = uint8_t(std::clamp(v + 12 * c + noise(rng), 0.0, 255.0));
        }
    return im;
}

int main(int argc, char** argv) {
    const std::string out = argc > 1 ? argv[1] : "results";
    std::ofstream csv(out + "/improvements_combined.csv");
    csv << "size,width,height,seams,config,ms_per_seam,speedup_vs_baseline,identical_result\n";
    struct Size { int w, h, k, reps; const char* name; };
    struct Cfg { const char* name; OptOptions o; };
    OptOptions inc; inc.incremental = true;
    OptOptions incl = inc; incl.lowmem = true;
    OptOptions th; th.threads = 2; th.thread_dp = false;
    const Cfg cfgs[] = {{"baseline", {}}, {"lowmem", [] { OptOptions o; o.lowmem = true; return o; }()},
                        {"incremental", inc}, {"incremental+lowmem", incl}, {"energy_threads2", th}};
    for (const Size& S : {Size{256, 192, 40, 15, "small"}, Size{1024, 768, 20, 9, "medium"}, Size{2048, 1536, 6, 7, "large"}}) {
        Image img = synthetic_image(S.w, S.h, 7);
        Image ref = shrink_width(img, S.k, {});
        double base = 0;
        for (const Cfg& c : cfgs) {
            std::vector<double> t;
            bool same = true;
            for (int r = 0; r < S.reps + 1; ++r) {  // first rep is a warm-up
                auto t0 = Clock::now();
                Image res = shrink_width(img, S.k, c.o);
                double ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count() / S.k;
                same = same && res.rgb == ref.rgb;
                if (r > 0) t.push_back(ms);
            }
            std::sort(t.begin(), t.end());
            double med = t[t.size() / 2];
            if (base == 0) base = med;
            csv << S.name << ',' << S.w << ',' << S.h << ',' << S.k << ',' << c.name << ',' << med << ',' << base / med << ',' << same << '\n';
        }
    }
}
