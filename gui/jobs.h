#pragma once

#include <atomic>
#include <chrono>
#include <future>
#include <optional>
#include <string>
#include <vector>

#include "bench.h"
#include "visualize.h"

namespace seam::gui {
enum class JobKind { None, Resize, Benchmark };

struct BenchmarkOptions {
    std::vector<Algorithm> algorithms{Algorithm::DP, Algorithm::Greedy};
    std::vector<double> scales{0.125, 0.25, 0.5, 1.0};
    int seams = 10;
    int repeats = 3;
    double budget_ms = 1500;
};

struct JobResult {
    JobKind kind = JobKind::None;
    Algorithm algorithm = Algorithm::DP;
    bool cancelled = false;
    std::string error;
    Image image;
    Image overview;
    std::vector<SeamRecord> history;
    std::vector<BenchRow> rows;
    std::vector<std::string> notes;
    double ms = 0;
    double total_cost = 0;
};

// Owned by the UI thread. Only progress/cancellation atomics cross threads;
// images and result vectors are handed back once via the future. OpenGL calls
// stay on the UI thread, after take_ready() returns a result.
class Jobs {
public:
    ~Jobs();
    void start_resize(Image input, int width, int height, Algorithm algorithm);
    void start_benchmark(Image input, BenchmarkOptions options);
    void cancel() { cancel_.store(true); }
    bool busy() const { return future_.valid(); }
    bool cancelling() const { return cancel_.load(); }
    JobKind kind() const { return kind_; }
    int completed() const { return completed_.load(); }
    int total() const { return total_; }
    double cost() const { return cost_.load(); }
    double elapsed_ms() const;
    std::optional<JobResult> take_ready();

private:
    void begin(JobKind kind, int total);
    std::future<JobResult> future_;
    std::atomic<bool> cancel_{false};
    std::atomic<int> completed_{0};
    std::atomic<double> cost_{0};
    JobKind kind_ = JobKind::None;
    int total_ = 0;
    std::chrono::steady_clock::time_point started_{};
};
}  // namespace seam::gui
