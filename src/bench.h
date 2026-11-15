#pragma once
#include <ostream>
#include <string>
#include <vector>

#include "image.h"
#include "resize.h"

namespace seam {

// One benchmark measurement: one algorithm on one image size.
// Shared by seamcli (--bench) and the GUI benchmark panel.
struct BenchRow {
    std::string algorithm;  // "dp", "greedy", "ek" or "dinic" (algorithm_name)
    int width = 0;          // input size at this scale, before any seam is removed
    int height = 0;
    int seams = 0;          // vertical seams actually removed (capped at width - 1)
    double ms = 0;          // median wall time of the whole run: energy + seam finding + removal
    double find_ms = 0;     // seam-finding share of that same run (the rest is energy and removal)
    double total_cost = 0;  // sum of the seam costs of every removed seam (GLOSSARY.md: Seam cost)
};

// "dp", "greedy", "ek", "dinic": the names used on the command line and in the CSV.
const char* algorithm_name(Algorithm a);
// Inverse of algorithm_name. Throws invalid_argument for an unknown name.
Algorithm parse_algorithm(const std::string& name);

// Shrinks img to round(scale * width) x round(scale * height) by averaging the source pixels each
// output pixel covers (plain scaling, not seam carving). Used to benchmark several image sizes from
// one test image. Requires 0 < scale <= 1; throws invalid_argument otherwise.
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
std::vector<BenchRow> run_benchmark(const Image& img, const std::vector<Algorithm>& algos,
                                    const std::vector<double>& scales, int seams, int repeats,
                                    double budget_ms = 0, std::vector<std::string>* skipped = nullptr);

// CSV with a header line, then one line per row:
// algorithm,width,height,seams,ms,find_ms,total_cost
void write_csv(const std::vector<BenchRow>& rows, std::ostream& out);

}  // namespace seam
