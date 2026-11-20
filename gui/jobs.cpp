/**
 * @brief Asynchronous seamlib workers with cooperative cancellation and error capture.
 * @file jobs.cpp
 */
#include "jobs.h"

#include <stdexcept>
#include <utility>

namespace seam::gui {
namespace {
using Clock = std::chrono::steady_clock;
/** @brief Internal control-flow signal caught before leaving a worker job. */
struct Cancelled {};
/**
 * @brief Measure elapsed wall-clock time from a steady-clock timestamp.
 * @param start Steady-clock timestamp marking measurement start.
 * @return Elapsed milliseconds.
 */
double milliseconds(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}
}  // namespace

/**
 * @brief Cancel and join any worker before destroying the progress atomics.
 * @note Cooperative cancellation may wait for a running seam or benchmark.
 */
Jobs::~Jobs() {
    cancel();
    // Keep this object and its atomics alive until the callback has stopped.
    // Cancellation is cooperative: an in-flight seam/benchmark must finish.
    if (future_.valid()) future_.wait();
}

/**
 * @brief Initialise progress and timing for a new job.
 * @param kind New background job kind.
 * @param total Expected progress-unit count.
 * @throws std::logic_error If a previous future has not been consumed.
 */
void Jobs::begin(JobKind kind, int total) {
    if (busy()) throw std::logic_error("A job is already running.");
    kind_ = kind;
    total_ = total;
    completed_.store(0);
    cost_.store(0);
    cancel_.store(false);
    started_ = Clock::now();
}

/**
 * @brief Measure time since begin started the current job.
 * @return Elapsed wall-clock milliseconds.
 * @pre A job has been started before interpreting this value.
 */
double Jobs::elapsed_ms() const { return milliseconds(started_); }

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
void Jobs::start_resize(Image input, int width, int height, Algorithm algorithm) {
    begin(JobKind::Resize, input.width - width + input.height - height);
    future_ = std::async(std::launch::async,
        [this, input = std::move(input), width, height, algorithm] {
            JobResult result;
            result.kind = JobKind::Resize;
            result.algorithm = algorithm;
            const auto start = Clock::now();
            try {
                if (cancel_.load()) throw Cancelled{};
                SeamTracker tracker(input.width, input.height);
                result.image = resize(input, width, height, algorithm,
                    [&](const Image& before, const Seam& seam, bool vertical) {
                        if (cancel_.load()) throw Cancelled{};
                        // The shared callback supplies the seam, but not its
                        // energy map. Measure cost without changing C1's API.
                        const auto energy = vertical ? sobel_energy(before) : sobel_energy(transpose(before));
                        result.total_cost += seam_cost(energy, seam);
                        tracker.record(seam, vertical);
                        cost_.store(result.total_cost);
                        completed_.fetch_add(1);
                    });
                if (cancel_.load()) throw Cancelled{};
                result.overview = tracker.overview(input);
                result.history = tracker.history();
            } catch (const Cancelled&) {
                result.cancelled = true;
                result.image = {};
            } catch (const std::exception& ex) {
                result.error = ex.what();
            }
            result.ms = milliseconds(start);
            return result;
        });
}

/**
 * @brief Launch background benchmarks, checking cancellation between algorithms.
 * @param input Owned input image transferred to the background worker.
 * @param options Algorithms, scales, repetitions, seam count and cooperative budget.
 * @throws std::invalid_argument For an empty algorithm list.
 * @throws std::logic_error If a job has not been consumed.
 * @note Cancelled benchmarks may retain completed rows.
 */
void Jobs::start_benchmark(Image input, BenchmarkOptions options) {
    if (options.algorithms.empty()) throw std::invalid_argument("Select at least one algorithm.");
    begin(JobKind::Benchmark, static_cast<int>(options.algorithms.size()));
    future_ = std::async(std::launch::async,
        [this, input = std::move(input), options = std::move(options)] {
            JobResult result;
            result.kind = JobKind::Benchmark;
            const auto start = Clock::now();
            try {
                for (Algorithm algorithm : options.algorithms) {
                    if (cancel_.load()) break;
                    // One algorithm across all scales preserves C2's budget
                    // and skip-larger-sizes rules. Cancel between algorithms.
                    auto rows = run_benchmark(input, {algorithm}, options.scales,
                        options.seams, options.repeats, options.budget_ms, &result.notes);
                    result.rows.insert(result.rows.end(), rows.begin(), rows.end());
                    completed_.fetch_add(1);
                }
                result.cancelled = cancel_.load();
            } catch (const std::exception& ex) {
                result.error = ex.what();
            }
            result.ms = milliseconds(start);
            return result;
        });
}

/**
 * @brief Consume the worker result without waiting for an unfinished job.
 * @return Completed result, or nullopt while idle/not ready.
 * @note The controller is owned by the UI thread. Consuming the future makes it reusable.
 */
std::optional<JobResult> Jobs::take_ready() {
    if (!busy() || future_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        return std::nullopt;
    kind_ = JobKind::None;
    return future_.get();
}
}  // namespace seam::gui
