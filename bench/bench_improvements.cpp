/**
 * @brief DP experiment runner with warm-up, median timings and heap tracking.
 * @file bench_improvements.cpp
 */
// Benchmark for assignment 5d. Writes results/improvements_{memory,dp,incremental,threads}.csv.
// Usage: bench_improvements <out_dir>
#if defined(_WIN32)
#include <malloc.h>
/**
 * @brief Query the allocator usable size of a live allocation for heap measurements.
 * @param p Allocator-owned memory pointer.
 * @return Platform-reported allocated byte count, which may exceed requested bytes.
 * @pre p is a live allocation from the compatible C allocator.
 */
static size_t usable_size(void* p) { return _msize(p); }
#elif defined(__APPLE__)
#include <malloc/malloc.h>
/**
 * @brief Query the allocator usable size of a live allocation for heap measurements.
 * @param p Allocator-owned memory pointer.
 * @return Platform-reported allocated byte count, which may exceed requested bytes.
 * @pre p is a live allocation from the compatible C allocator.
 */
static size_t usable_size(void* p) { return malloc_size(p); }
#else
#include <malloc.h>
/**
 * @brief Query the allocator usable size of a live allocation for heap measurements.
 * @param p Allocator-owned memory pointer.
 * @return Platform-reported allocated byte count, which may exceed requested bytes.
 * @pre p is a live allocation from the compatible C allocator.
 */
static size_t usable_size(void* p) { return malloc_usable_size(p); }
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <new>
#include <random>
#include <string>
#include <thread>

#include "seam_dp_opt.h"

// ---- heap tracking: peak live bytes between reset_peak() and now.
static std::atomic<long long> g_live{0}, g_peak{0};
/**
 * @brief Account for an allocation and update the atomic high-water mark.
 * @param p Allocator-owned memory pointer.
 */
static void track(void* p) {
    long long now = g_live += (long long)usable_size(p);
    long long pk = g_peak.load();
    while (now > pk && !g_peak.compare_exchange_weak(pk, now)) {}
}
/**
 * @brief Allocate and account for heap storage in this benchmark executable.
 * @param n Requested allocation size in bytes.
 * @return Non-null allocated storage.
 * @throws std::bad_alloc If malloc fails.
 */
void* operator new(std::size_t n) {
    void* p = std::malloc(n ? n : 1);
    if (!p) throw std::bad_alloc();
    track(p);
    return p;
}
/**
 * @brief Subtract live allocation bytes and free benchmark heap storage.
 * @param p Allocator-owned memory pointer.
 * @note Null pointers are ignored; the sized overload delegates to the unsized overload.
 */
void operator delete(void* p) noexcept { if (p) { g_live -= (long long)usable_size(p); std::free(p); } }
/**
 * @brief Subtract live allocation bytes and free benchmark heap storage.
 * @param p Allocator-owned memory pointer.
 * @note Null pointers are ignored; the sized overload delegates to the unsized overload.
 */
void operator delete(void* p, std::size_t) noexcept { operator delete(p); }
/**
 * @brief Reset the benchmark high-water mark to current live heap usage.
 */
static void reset_peak() { g_peak = g_live.load(); }
/**
 * @brief Read peak heap usage relative to a caller-recorded baseline.
 * @param base Previously recorded live-byte baseline.
 * @return Additional peak live bytes since the baseline.
 */
static long long peak_extra(long long base) { return g_peak.load() - base; }

using namespace seam;
using Clock = std::chrono::steady_clock;

/**
 * @brief Generate a fixed-seed scene of gradients, bands and noise for benchmarks.
 * @param W Full row width.
 * @param H Number of rows.
 * @param seed Fixed seed for reproducible fixture pixels.
 * @return Owned RGB benchmark image.
 */
static Image synthetic_image(int W, int H, unsigned seed) {
    // Smooth gradients + sinusoids + noise: some strong edges, some flat regions.
    std::mt19937 rng(seed);
    std::normal_distribution<double> noise(0, 6);
    Image im(W, H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            double v = 128 + 70 * std::sin(x * 0.045) * std::cos(y * 0.031) + 30 * std::sin((x + y) * 0.013);
            if ((x / 97 + y / 61) % 5 == 0) v += 45;  // blocky edges
            for (int c = 0; c < 3; ++c)
                im.px(x, y)[c] = uint8_t(std::clamp(v + 12 * c + noise(rng), 0.0, 255.0));
        }
    return im;
}

/**
 * @brief Select the middle value after sorting a timing-sample copy.
 * @param v Nonempty timing-sample vector, copied for sorting.
 * @return Middle sample; for an even count, the upper middle sample.
 * @pre v is nonempty.
 */
static double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

/**
 * @brief Warm up a callable, then time repeated invocations.
 * @param reps Number of timed repetitions after warm-up.
 * @param f Callable whose runtime is measured.
 * @return Median elapsed milliseconds, excluding the warm-up call.
 * @pre reps is positive and f is callable.
 */
static double time_ms(int reps, const std::function<void()>& f) {
    f();  // warm-up
    std::vector<double> t;
    for (int i = 0; i < reps; ++i) {
        auto t0 = Clock::now();
        f();
        t.push_back(std::chrono::duration<double, std::milli>(Clock::now() - t0).count());
    }
    return median(t);
}

struct Size { int w, h, k, reps; const char* name; };

/**
 * @brief DP experiment runner with warm-up, median timings and heap tracking.
 * @param argc Argument count, including the executable name.
 * @param argv Null-terminated command-line argument strings.
 * @return Zero on normal completion; program-specific validation errors are reported or propagate.
 * @note Run benchmark/evidence tools from the repository root.
 */
int main(int argc, char** argv) {
    const std::string out = argc > 1 ? argv[1] : "results";
    const unsigned cores = std::thread::hardware_concurrency();
    const Size sizes[] = {{256, 192, 40, 31, "small"}, {1024, 768, 20, 15, "medium"}, {2048, 1536, 6, 9, "large"}};

    std::ofstream mem(out + "/improvements_memory.csv"), dp(out + "/improvements_dp.csv"),
        inc(out + "/improvements_incremental.csv"), thr(out + "/improvements_threads.csv");
    mem << "size,width,height,variant,peak_heap_bytes,peak_heap_MB,theory_dp_bytes\n";
    dp << "size,width,height,variant,ms_per_seam_search,speedup_vs_baseline,same_seam_as_baseline\n";
    inc << "size,width,height,seams,mode,energy_ms_per_seam,dp_ms_per_seam,remove_ms_per_seam,total_ms_per_seam,"
           "speedup_vs_full,identical_result\n";
    thr << "size,width,height,threads,hw_threads,energy_ms,energy_speedup,dp_ms,dp_speedup,"
           "pipeline_ms_per_seam,pipeline_speedup,pipeline_energy_only_ms_per_seam,pipeline_energy_only_speedup,"
           "original_sobel_energy_ms\n";

    for (const Size& S : sizes) {
        Image img = synthetic_image(S.w, S.h, 7);
        EnergyMap e = sobel_energy(img);
        const long long W = S.w, H = S.h;
        std::fprintf(stderr, "== %s %dx%d\n", S.name, S.w, S.h);

        // ---- (1) DP table: memory and time of the seam search alone.
        Seam base = find_seam_dp(e);
        struct V { const char* name; std::function<Seam()> f; long long theory; };
        V vs[] = {{"baseline_full_double_table", [&] { return find_seam_dp(e); }, W * H * 8},
                  {"lowmem_2rows_int8_dirs", [&] { return find_seam_dp_lowmem(e); }, 2 * W * 8 + W * H}};
        double base_ms = 0;
        for (auto& v : vs) {
            long long live0 = g_live.load();
            reset_peak();
            Seam s = v.f();
            long long pk = peak_extra(live0);
            double ms = time_ms(S.reps * 3, [&] { volatile auto n = v.f().size(); (void)n; });
            if (base_ms == 0) base_ms = ms;
            mem << S.name << ',' << W << ',' << H << ',' << v.name << ',' << pk << ',' << pk / 1048576.0 << ',' << v.theory << '\n';
            dp << S.name << ',' << W << ',' << H << ',' << v.name << ',' << ms << ',' << base_ms / ms << ',' << (s == base) << '\n';
        }

        // ---- (2) incremental energy: remove k vertical seams, full vs incremental (otherwise baseline).
        Image ref = shrink_width(img, S.k, {});
        for (bool incr : {false, true}) {
            OptOptions o; o.incremental = incr;
            std::vector<double> en, dpv, rm, tot;
            bool same = true;
            for (int r = 0; r < std::max(3, S.reps / 2); ++r) {
                StageTimes st;
                auto t0 = Clock::now();
                Image res = shrink_width(img, S.k, o, &st);
                double total = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
                same = same && res.rgb == ref.rgb;
                en.push_back(st.energy * 1e3 / S.k); dpv.push_back(st.dp * 1e3 / S.k);
                rm.push_back(st.remove * 1e3 / S.k); tot.push_back(total / S.k);
            }
            static double full_total = 0;
            if (!incr) full_total = median(tot);
            inc << S.name << ',' << W << ',' << H << ',' << S.k << ',' << (incr ? "incremental" : "full_recompute") << ','
                << median(en) << ',' << median(dpv) << ',' << median(rm) << ',' << median(tot) << ','
                << full_total / median(tot) << ',' << same << '\n';
        }

        // ---- (3) threads: energy rows, DP cells per row, and the full pipeline (energy+DP+removal).
        double e1 = 0, d1 = 0, p1 = 0, q1 = 0;
        double e0 = time_ms(S.reps, [&] { auto m = sobel_energy(img); volatile auto n = m.e.size(); (void)n; });
        for (int T : {1, 2, 4, 8}) {
            double em = time_ms(S.reps, [&] { auto m = sobel_energy_threaded(img, T); volatile auto n = m.e.size(); (void)n; });
            double dm = time_ms(S.reps * 2, [&] { volatile auto n = find_seam_dp_threaded(e, T).size(); (void)n; });
            OptOptions o; o.threads = T; o.own_energy = true;
            std::vector<double> pt;
            for (int r = 0; r < std::max(3, S.reps / 2); ++r) {
                auto t0 = Clock::now();
                Image res = shrink_width(img, S.k, o);
                pt.push_back(std::chrono::duration<double, std::milli>(Clock::now() - t0).count() / S.k);
                if (res.rgb != ref.rgb) { std::fprintf(stderr, "MISMATCH\n"); return 1; }
            }
            double pm = median(pt);
            // same pipeline, but only the energy rows are threaded (DP stays serial)
            o.thread_dp = false;
            std::vector<double> qt;
            for (int r = 0; r < std::max(3, S.reps / 2); ++r) {
                auto t0 = Clock::now();
                Image res = shrink_width(img, S.k, o);
                qt.push_back(std::chrono::duration<double, std::milli>(Clock::now() - t0).count() / S.k);
                if (res.rgb != ref.rgb) { std::fprintf(stderr, "MISMATCH\n"); return 1; }
            }
            double qm = median(qt);
            if (T == 1) { e1 = em; d1 = dm; p1 = pm; q1 = qm; }
            thr << S.name << ',' << W << ',' << H << ',' << T << ',' << cores << ',' << em << ',' << e1 / em << ','
                << dm << ',' << d1 / dm << ',' << pm << ',' << p1 / pm << ',' << qm << ',' << q1 / qm << ',' << e0 << '\n';
        }
    }
    return 0;
}
