/**
 * @brief Optimised DP variants, row barriers and incremental Sobel updates.
 * @file seam_dp_opt.cpp
 */
#include "seam_dp_opt.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>

// std::barrier needs C++20. On C++17 (or older toolchains) use a small portable barrier instead,
// so this file compiles whatever standard the project sets.
#if defined(_MSVC_LANG)
#define SEAM_CPLUSPLUS _MSVC_LANG
#else
#define SEAM_CPLUSPLUS __cplusplus
#endif
#if SEAM_CPLUSPLUS >= 202002L && defined(__has_include)
#if __has_include(<barrier>)
#include <barrier>
#define SEAM_HAS_STD_BARRIER 1
#endif
#endif

namespace seam {

#ifdef SEAM_HAS_STD_BARRIER
/** @brief Reusable DP-row rendezvous using C++20 standard barrier support. */
class RowBarrier {
public:
    /**
     * @brief Create a reusable rendezvous for a fixed worker count.
     * @param n Positive number of worker participants.
     * @pre n is positive and every participant calls wait once per generation.
     */
    explicit RowBarrier(int n) : b_(n) {}
    /**
     * @brief Wait until all participants finish the current DP row.
     * @note The last arriving worker releases the group and starts the next generation.
     */
    void wait() { b_.arrive_and_wait(); }

private:
    std::barrier<> b_;
};
#else
/** @brief Generation-counted C++17 barrier for the same DP-row contract. */
class RowBarrier {
public:
    /**
     * @brief Create a reusable rendezvous for a fixed worker count.
     * @param n Positive number of worker participants.
     * @pre n is positive and every participant calls wait once per generation.
     */
    explicit RowBarrier(int n) : n_(n) {}
    /**
     * @brief Wait until all participants finish the current DP row.
     * @note The last arriving worker releases the group and starts the next generation.
     */
    void wait() {
        std::unique_lock<std::mutex> lock(m_);
        const unsigned gen = gen_;
        if (++waiting_ == n_) {
            waiting_ = 0;
            ++gen_;
            cv_.notify_all();
        } else {
            cv_.wait(lock, [&] { return gen_ != gen; });
        }
    }

private:
    int n_, waiting_ = 0;
    unsigned gen_ = 0;
    std::mutex m_;
    std::condition_variable cv_;
};
#endif

// ---------------------------------------------------------------- DP variants

// One DP row for columns [x0, x1). Same candidate order and tie rules as find_seam_dp's
// backtracking (left wins ties with straight, right only when strictly cheaper), so the stored
// direction is exactly the step the baseline would take from this cell.
/**
 * @brief Fill DP costs and parent offsets for columns [x0,x1) of one row.
 * @param prev Previous row of cumulative DP costs.
 * @param cur Writable cumulative-cost buffer for the current row.
 * @param dir Parent offsets for backtracking.
 * @param erow Energy values for this row.
 * @param W Full row width.
 * @param x0 First included column.
 * @param x1 Exclusive last column.
 * @pre Buffers cover W cells, erow is the current energy row, and 0 <= x0 <= x1 <= W. Workers must
 * write disjoint ranges and finish before the next row.
 */
static inline void dp_row(const double* prev, double* cur, int8_t* dir, const double* erow, int W,
                          int x0, int x1) {
    // Branch-free interior: the compare results feed selects, so random energies do not cause
    // branch mispredictions (a first branchy version was ~4-5x slower than the baseline's std::min).
    auto cell = [&](int x, double l, double c, double r) {
        const bool takeL = l <= c;
        double best = takeL ? l : c;
        int8_t d = takeL ? int8_t(-1) : int8_t(0);
        const bool takeR = r < best;
        best = takeR ? r : best;
        d = takeR ? int8_t(1) : d;
        cur[x] = erow[x] + best;
        dir[x] = d;
    };
    int a = x0, b = x1;
    if (a == 0) {  // left border: no left parent
        if (W == 1) { cur[0] = erow[0] + prev[0]; dir[0] = 0; return; }
        const double c = prev[0];
        const bool takeR = prev[1] < c;
        cur[0] = erow[0] + (takeR ? prev[1] : c);
        dir[0] = takeR ? 1 : 0;
        a = 1;
    }
    bool right = false;
    if (b == W) { right = true; b = W - 1; }
    for (int x = a; x < b; ++x) cell(x, prev[x - 1], prev[x], prev[x + 1]);
    if (right && W > 1 && a <= W - 1) {  // right border: no right parent
        const int x = W - 1;
        const bool takeL = prev[x - 1] <= prev[x];
        cur[x] = erow[x] + (takeL ? prev[x - 1] : prev[x]);
        dir[x] = takeL ? -1 : 0;
    }
}

/**
 * @brief Validate dimensions and storage before DP buffer allocation.
 * @param e Input row-major energy map.
 * @param who Function name included in validation errors.
 * @throws std::invalid_argument For an empty or inconsistent energy map.
 */
static void check_map(const EnergyMap& e, const char* who) {
    if (e.width < 1 || e.height < 1) throw std::invalid_argument(std::string(who) + ": energy map is empty");
    if (e.e.size() != size_t(e.width) * e.height)
        throw std::invalid_argument(std::string(who) + ": energy map size mismatch");
}

/**
 * @brief Recover a leftmost optimal seam from final costs and parent directions.
 * @param last Final DP row of cumulative costs.
 * @param dir Parent offsets for backtracking.
 * @param W Full row width.
 * @param H Number of rows.
 * @return One column per row.
 * @pre Cost and direction buffers are consistent with positive W and H.
 */
static Seam backtrack(const double* last, const std::vector<int8_t>& dir, int W, int H) {
    Seam s(H);
    int x = 0;
    for (int i = 1; i < W; ++i)
        if (last[i] < last[x]) x = i;  // strict <: leftmost minimum
    s[H - 1] = x;
    for (int y = H - 1; y > 0; --y) {
        x += dir[size_t(y) * W + x];
        s[y - 1] = x;
    }
    return s;
}

/**
 * @brief Find the baseline DP seam using rolling costs and byte directions.
 * @param e Input row-major energy map.
 * @return Same seam and tie rule as full-table DP.
 * @throws std::invalid_argument For an empty or inconsistent map.
 * @par Complexity
 * O(W H) time; O(W) double costs plus O(W H) byte directions.
 */
Seam find_seam_dp_lowmem(const EnergyMap& e) {
    check_map(e, "find_seam_dp_lowmem");
    const int W = e.width, H = e.height;
    std::vector<double> rows(2 * size_t(W));
    std::vector<int8_t> dir(size_t(W) * H, 0);
    double* r[2] = {rows.data(), rows.data() + W};
    std::copy(e.e.begin(), e.e.begin() + W, r[0]);
    for (int y = 1; y < H; ++y)
        dp_row(r[(y - 1) & 1], r[y & 1], &dir[size_t(y) * W], &e.e[size_t(y) * W], W, 0, W);
    return backtrack(r[(H - 1) & 1], dir, W, H);
}

/**
 * @brief Compute disjoint cell ranges in each DP row with a barrier between rows.
 * @param e Input row-major energy map.
 * @param threads Requested worker count; serial fallback applies when the effective count is one.
 * @return Same seam as serial DP; small or single-thread cases use low-memory DP.
 * @throws std::invalid_argument For an empty or inconsistent map.
 * @note Workers are capped by width and 256. Row dependencies prevent parallelising across rows.
 */
Seam find_seam_dp_threaded(const EnergyMap& e, int threads) {
    check_map(e, "find_seam_dp_threaded");
    const int W = e.width, H = e.height;
    const int T = std::min({threads, W, 256});
    if (T <= 1 || H < 2) return find_seam_dp_lowmem(e);

    std::vector<double> rows(2 * size_t(W));
    std::vector<int8_t> dir(size_t(W) * H, 0);
    double* r[2] = {rows.data(), rows.data() + W};
    std::copy(e.e.begin(), e.e.begin() + W, r[0]);

    // Row y needs the finished row y-1, so every thread waits at a barrier after each row:
    // H-1 barriers per seam. Within a row the W cells are independent.
    RowBarrier bar(T);
    auto worker = [&](int t) {
        const int x0 = int(int64_t(W) * t / T), x1 = int(int64_t(W) * (t + 1) / T);
        for (int y = 1; y < H; ++y) {
            dp_row(r[(y - 1) & 1], r[y & 1], &dir[size_t(y) * W], &e.e[size_t(y) * W], W, x0, x1);
            bar.wait();
        }
    };
    std::vector<std::thread> pool;
    for (int t = 1; t < T; ++t) pool.emplace_back(worker, t);
    worker(0);
    for (auto& th : pool) th.join();
    return backtrack(r[(H - 1) & 1], dir, W, H);
}

// ---------------------------------------------------------------- energy

// Same expression order as energy.cpp so results are bit-identical.
/**
 * @brief Sample grayscale in the same arithmetic order as baseline Sobel.
 * @param img RGB image with storage matching its dimensions.
 * @param x Zero-based column coordinate.
 * @param y Zero-based row coordinate.
 * @return Clamped grayscale intensity.
 */
static inline double gray_at(const Image& img, int x, int y) {
    x = std::clamp(x, 0, img.width - 1);
    y = std::clamp(y, 0, img.height - 1);
    const uint8_t* p = img.px(x, y);
    return 0.299 * p[0] + 0.587 * p[1] + 0.114 * p[2];
}

/**
 * @brief Compute one backward Sobel energy using clamped grayscale samples.
 * @param img RGB image with storage matching its dimensions.
 * @param x Zero-based column coordinate.
 * @param y Zero-based row coordinate.
 * @return abs(Gx) + abs(Gy) at the requested pixel.
 */
static inline double energy_px(const Image& img, int x, int y) {
    double gx = -gray_at(img, x - 1, y - 1) - 2 * gray_at(img, x - 1, y) - gray_at(img, x - 1, y + 1) +
                gray_at(img, x + 1, y - 1) + 2 * gray_at(img, x + 1, y) + gray_at(img, x + 1, y + 1);
    double gy = -gray_at(img, x - 1, y - 1) - 2 * gray_at(img, x, y - 1) - gray_at(img, x + 1, y - 1) +
                gray_at(img, x - 1, y + 1) + 2 * gray_at(img, x, y + 1) + gray_at(img, x + 1, y + 1);
    return std::abs(gx) + std::abs(gy);
}

/**
 * @brief Compute the baseline Sobel expression on disjoint row ranges.
 * @param img RGB image with storage matching its dimensions.
 * @param threads Requested worker count; serial fallback applies when the effective count is one.
 * @return Same energy values as baseline Sobel for valid input.
 * @pre img has positive dimensions and consistent RGB storage.
 * @note Worker count is clamped to 1..height; workers are joined before return.
 */
EnergyMap sobel_energy_threaded(const Image& img, int threads) {
    EnergyMap m;
    m.width = img.width;
    m.height = img.height;
    m.e.assign(size_t(m.width) * m.height, 0.0);
    const int T = std::max(1, std::min(threads, img.height));
    auto rows = [&](int y0, int y1) {
        for (int y = y0; y < y1; ++y)
            for (int x = 0; x < img.width; ++x) m.at(x, y) = energy_px(img, x, y);
    };
    std::vector<std::thread> pool;
    for (int t = 1; t < T; ++t)
        pool.emplace_back(rows, int(int64_t(img.height) * t / T), int(int64_t(img.height) * (t + 1) / T));
    rows(0, int(int64_t(img.height) / T));
    for (auto& th : pool) th.join();
    return m;
}

/**
 * @brief Shift cached energy and recompute pixels whose Sobel neighbourhood changed.
 * @param old_e Cached energy map before the vertical seam removal.
 * @param new_img Image after the vertical seam has been removed.
 * @param s Selected seam in current image coordinates.
 * @return Energy map for the new image.
 * @pre removed seam coordinates are valid for the old image; both maps have consistent storage.
 * @throws std::invalid_argument If old/new dimensions or seam length do not match.
 * @note At most six candidate columns per row are recomputed. Shifting/copying still costs O(W H).
 */
EnergyMap sobel_energy_after_removal(const EnergyMap& old_e, const Image& new_img, const Seam& s) {
    const int W = new_img.width, H = new_img.height;
    if (old_e.width != W + 1 || old_e.height != H || int(s.size()) != H)
        throw std::invalid_argument("sobel_energy_after_removal: sizes do not match");
    EnergyMap out;
    out.width = W;
    out.height = H;
    out.e.resize(size_t(W) * H);
    for (int y = 0; y < H; ++y) {
        // 1. Shift: pixels right of the seam move one column left; their energy is unchanged
        //    unless step 2 says otherwise.
        const double* src = &old_e.e[size_t(y) * (W + 1)];
        double* dst = &out.e[size_t(y) * W];
        std::copy(src, src + s[y], dst);
        std::copy(src + s[y] + 1, src + W + 1, dst + s[y]);
        // 2. Recompute pixels whose 3x3 window now straddles a removed pixel. In row r the gap sits
        //    between new columns s[r]-1 and s[r], so columns s[r]-1 and s[r] are affected, for
        //    every row r in y-1..y+1 (this also covers the clamped borders).
        int cols[6], n = 0;
        for (int r = std::max(0, y - 1); r <= std::min(H - 1, y + 1); ++r) {
            cols[n++] = s[r] - 1;
            cols[n++] = s[r];
        }
        for (int i = 1; i < n; ++i)  // insertion sort: n <= 6
            for (int j = i; j > 0 && cols[j - 1] > cols[j]; --j) std::swap(cols[j - 1], cols[j]);
        int prev = -2;
        for (int i = 0; i < n; ++i) {
            if (cols[i] == prev) continue;
            prev = cols[i];
            if (cols[i] >= 0 && cols[i] < W) dst[cols[i]] = energy_px(new_img, cols[i], y);
        }
    }
    return out;
}

// ---------------------------------------------------------------- driver

using Clock = std::chrono::steady_clock;
/**
 * @brief Measure elapsed wall-clock time for an optimisation stage.
 * @param t0 Steady-clock start timestamp.
 * @return Elapsed seconds, not milliseconds.
 */
static double since(Clock::time_point t0) {
    return std::chrono::duration<double>(Clock::now() - t0).count();
}

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
Image shrink_width(const Image& img, int k, const OptOptions& opt, StageTimes* times) {
    if (k < 0 || k >= img.width) throw std::invalid_argument("shrink_width: k out of range");
    Image cur = img;
    StageTimes st;
    EnergyMap e;
    bool have_e = false;
    for (int i = 0; i < k; ++i) {
        auto t0 = Clock::now();
        if (!(opt.incremental && have_e))
            e = (opt.threads > 1 || opt.own_energy) ? sobel_energy_threaded(cur, opt.threads) : sobel_energy(cur);
        st.energy += since(t0);

        t0 = Clock::now();
        Seam s = (opt.threads > 1 && opt.thread_dp) ? find_seam_dp_threaded(e, opt.threads)
                 : opt.lowmem    ? find_seam_dp_lowmem(e)
                                 /**
                                  * @brief Find an optimal vertical seam using a full
                                  * cumulative-cost table.
                                  * @param e Input row-major energy map.
                                  * @return Minimum-cost seam; ties prefer leftmost columns from the
                                  * bottom upward.
                                  * @throws std::invalid_argument For an empty or inconsistent map.
                                  * @par Complexity
                                  * O(W H) time and storage.
                                  */
                                 : find_seam_dp(e);
        st.dp += since(t0);

        t0 = Clock::now();
        cur = remove_vertical_seam(cur, s);
        st.remove += since(t0);

        if (opt.incremental) {
            t0 = Clock::now();
            e = sobel_energy_after_removal(e, cur, s);
            st.energy += since(t0);
            have_e = true;
        }
    }
    if (times) *times = st;
    return cur;
}

}  // namespace seam
