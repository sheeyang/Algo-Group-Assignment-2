/**
 * @brief Background job ownership, atomic progress and result-transfer contracts.
 * @file jobs.h
 */
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
/** @brief Work represented by the controller's current future. */
enum class JobKind { None, Resize, Benchmark };

/** @brief Value-owned worker settings; units match seamlib run_benchmark. */
struct BenchmarkOptions {
    std::vector<Algorithm> algorithms{Algorithm::DP, Algorithm::Greedy};
    std::vector<double> scales{0.125, 0.25, 0.5, 1.0};
    int seams = 10;
    int repeats = 3;
    double budget_ms = 1500;
};

/**
 * @brief Worker-to-UI value transferred only after computation completes.
 * @details Inspect error and cancelled before using resize pixels. Cancelled
 * benchmarks may contain completed rows. ms is worker wall time; total_cost
 * is the sum along the resize sequence, not a whole-sequence optimality claim.
 */
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
/**
 * @brief Single-job asynchronous controller owned by the UI thread.
 * @details Only atomic progress and cancellation values cross threads during
 * execution. Result images/vectors transfer via the future. It performs no
 * OpenGL work; the UI uploads completed results after take_ready().
 */
class Jobs {
public:
    /**
     * @brief Request cancellation and join any worker before destroying state.
     * @note May block until an in-progress seam or algorithm finishes.
     */
    ~Jobs();
    /**
     * @brief Launch a background resize using an owned copy of the input.
     * @param input Owned input image transferred to the background worker.
     * @param width Requested final width, between one and the input width.
     * @param height Requested final height, between one and the input height.
     * @param algorithm Seam-finding algorithm.
     * @throws std::logic_error If a job has not been consumed.
     * @note Worker-side errors are captured in JobResult. A cancelled resize publishes no partial
     * output image; cancellation is observed at seam boundaries.
     */
    void start_resize(Image input, int width, int height, Algorithm algorithm);
    /**
     * @brief Launch background benchmarks, checking cancellation between algorithms.
     * @param input Owned input image transferred to the background worker.
     * @param options Algorithms, scales, repetitions, seam count and cooperative budget.
     * @throws std::invalid_argument For an empty algorithm list.
     * @throws std::logic_error If a job has not been consumed.
     * @note Cancelled benchmarks may retain completed rows.
     */
    void start_benchmark(Image input, BenchmarkOptions options);
    /**
     * @brief Request cooperative cancellation without blocking the UI thread.
     */
    void cancel() { cancel_.store(true); }
    /**
     * @brief Check whether a worker future still belongs to this controller.
     * @return True until take_ready consumes a result, even if computation has finished.
     */
    bool busy() const { return future_.valid(); }
    /**
     * @brief Read whether cancellation has been requested.
     * @return Current atomic cancellation flag.
     */
    bool cancelling() const { return cancel_.load(); }
    /**
     * @brief Read the kind of the current unconsumed job.
     * @return JobKind::None when no result is pending after consumption.
     */
    JobKind kind() const { return kind_; }
    /**
     * @brief Read atomically published progress units.
     * @return Resize seam count, or benchmark algorithm count.
     */
    int completed() const { return completed_.load(); }
    /**
     * @brief Read the expected progress units for the current job.
     * @return Requested seam count for resize, algorithm count for benchmarks.
     */
    int total() const { return total_; }
    /**
     * @brief Read atomically published resize seam cost.
     * @return Cumulative cost so far; benchmarks use their own result rows.
     */
    double cost() const { return cost_.load(); }
    /**
     * @brief Measure time since begin started the current job.
     * @return Elapsed wall-clock milliseconds.
     * @pre A job has been started before interpreting this value.
     */
    double elapsed_ms() const;
    /**
     * @brief Consume the worker result without waiting for an unfinished job.
     * @return Completed result, or nullopt while idle/not ready.
     * @note The controller is owned by the UI thread. Consuming the future makes it reusable.
     */
    std::optional<JobResult> take_ready();

private:
    /**
     * @brief Initialise progress and timing for a new job.
     * @param kind New background job kind.
     * @param total Expected progress-unit count.
     * @throws std::logic_error If a previous future has not been consumed.
     */
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
