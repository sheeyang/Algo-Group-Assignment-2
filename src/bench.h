/**
 * @brief Shared benchmark measurements, input scaling and CSV interfaces.
 * @file bench.h
 */
#pragma once
#include <ostream>
#include <string>
#include <vector>

#include "image.h"
#include "resize.h"

namespace seam {

/**
 * @brief One consistent algorithm/size measurement used by CLI and GUI.
 * @details Every timing and cost belongs to the same selected median run.
 * find_ms includes the complete seam finder, including graph construction
 * and extraction for graph cut; it does not isolate just max-flow.
 */
struct BenchRow {
    std::string algorithm;  ///< "dp", "greedy", "ek" or "dinic" (algorithm_name)
    int width = 0;          ///< input size at this scale, before any seam is removed
    int height = 0;
    int seams = 0;          ///< vertical seams actually removed (capped at width - 1)
    double ms = 0;          ///< median wall time of the whole run: energy + seam finding + removal
    double find_ms = 0;     ///< seam-finding share of that same run (the rest is energy and removal)
    double total_cost = 0;  ///< sum of the seam costs of every removed seam (GLOSSARY.md: Seam cost)
};

// "dp", "greedy", "ek", "dinic": the names used on the command line and in the CSV.
/**
 * @brief Convert an algorithm enum to its stable CLI/CSV name.
 * @param a Seam-finding algorithm to dispatch.
 * @return Static name string, such as dp, greedy, ek or dinic.
 * @throws std::invalid_argument For an unknown enum value.
 */
const char* algorithm_name(Algorithm a);
// Inverse of algorithm_name. Throws invalid_argument for an unknown name.
/**
 * @brief Parse a case-sensitive CLI algorithm name.
 * @param name Case-sensitive algorithm name.
 * @return Matching algorithm enum.
 * @throws std::invalid_argument For an unknown name.
 */
Algorithm parse_algorithm(const std::string& name);

// Shrinks img to round(scale * width) x round(scale * height) by averaging the source pixels each
// output pixel covers (plain scaling, not seam carving). Used to benchmark several image sizes from
// one test image. Requires 0 < scale <= 1; throws invalid_argument otherwise.
/**
 * @brief Box-average source blocks into a smaller benchmark input.
 * @param img RGB image with storage matching its dimensions.
 * @param scale Ordinary image scale in (0,1].
 * @return Owned image with rounded scaled dimensions, each at least one pixel.
 * @throws std::invalid_argument For an empty image or scale outside (0,1].
 * @pre RGB storage matches dimensions.
 * @note This is ordinary scaling, not seam carving; the input is unchanged.
 */
Image scale_image(const Image& img, double scale);

// For every scale (smallest first) and every algorithm: scale the image, then remove `seams` vertical
// seams one at a time (energy map, seam finder, removal, as in resize), `repeats` times, and report the
// median run. Only vertical seams are removed, so every algorithm does exactly the same amount of work;
// horizontal seams cost the same after the transpose.
//
// An algorithm that throws std::logic_error (not implemented yet) is skipped, not fatal.
// If budget_ms > 0, Edmonds-Karp and other slow algorithms are kept in check: a run that takes
// longer than budget_ms is stopped after the current seam and gets no row (the note says how many
// seams it managed in what time), and the larger scales of that algorithm are skipped. A run over
// budget that still finished all its seams keeps its row. Each skip is described in *skipped when
// it is given.
//
// Throws invalid_argument for an empty image or algorithm list, a scale outside (0, 1],
// seams < 1 or repeats < 1.
/**
 * @brief Run algorithms at ascending image scales and select median complete runs.
 * @param img RGB image with storage matching its dimensions.
 * @param algos Algorithms to measure.
 * @param scales Requested input scales in (0,1].
 * @param seams Requested vertical seam removals.
 * @param repeats Number of fresh repeated runs.
 * @param budget_ms Cooperative time limit in milliseconds; nonpositive disables it.
 * @param skipped Optional destination for skip/budget diagnostic messages.
 * @return One row per completed algorithm/scale; lower median for even repeat counts.
 * @throws std::invalid_argument For empty inputs/lists, invalid scales or nonpositive seam/repeat
 * counts.
 * @note Time budgets are cooperative at seam boundaries. Over-budget algorithms skip larger sizes.
 * A partially completed run produces a note, not a CSV row. find_ms and cost come from the same
 * selected total-time run.
 */
std::vector<BenchRow> run_benchmark(const Image& img, const std::vector<Algorithm>& algos,
                                    const std::vector<double>& scales, int seams, int repeats,
                                    double budget_ms = 0, std::vector<std::string>* skipped = nullptr);

// CSV with a header line, then one line per row:
// algorithm,width,height,seams,ms,find_ms,total_cost
/**
 * @brief Serialise benchmark rows with a header and three decimal places.
 * @param rows Completed benchmark rows.
 * @param out Destination output stream.
 * @note Formatting is prepared in a separate stream; caller formatting flags are preserved. Output
 * stream failure state remains for the caller to inspect.
 */
void write_csv(const std::vector<BenchRow>& rows, std::ostream& out);

}  // namespace seam
