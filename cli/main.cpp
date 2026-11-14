// Usage: seamcli <in> <out> <target_w> <target_h> [dp|greedy|ek|dinic] [--steps <dir>]
//   --steps <dir>  also write <dir>/step_0001.png, step_0002.png, ... (the image before each removal
//                  with that seam painted red) and <dir>/overview.png (all seams on the original image).
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "resize.h"
#include "visualize.h"

int main(int argc, char** argv) {
    // Pull "--steps <dir>" out of the arguments; what is left are the positional arguments.
    std::vector<std::string> args;
    std::string steps_dir;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--steps") {
            if (i + 1 >= argc) {
                std::cerr << "error: --steps needs a directory\n";
                return 2;
            }
            steps_dir = argv[++i];
        }
        else {
            args.push_back(a);
        }
    }
    if (args.size() < 4) {
        std::cerr << "usage: seamcli <in> <out> <target_w> <target_h> [dp|greedy|ek|dinic] [--steps <dir>]\n";
        return 2;
    }
    try {
        seam::Algorithm algo = seam::Algorithm::DP;
        if (args.size() > 4) {
            const std::string& a = args[4];
            if (a == "greedy") algo = seam::Algorithm::Greedy;
            else if (a == "ek") algo = seam::Algorithm::GraphCutEK;
            else if (a == "dinic") algo = seam::Algorithm::GraphCutDinic;
            else if (a != "dp") {
                std::cerr << "unknown algorithm: " << a << "\n";
                return 2;
            }
        }
        seam::Image in = seam::load_image(args[0]);
        const int tw = std::atoi(args[2].c_str()), th = std::atoi(args[3].c_str());

        seam::Image out;
        if (steps_dir.empty()) {
            out = seam::resize(in, tw, th, algo);
        }
        else {
            std::filesystem::create_directories(steps_dir);
            seam::SeamTracker tracker(in.width, in.height);  // maps seams back to original coordinates
            int n = 0;
            out = seam::resize(in, tw, th, algo, [&](const seam::Image& before, const seam::Seam& s, bool vertical) {
                char name[32];
                std::snprintf(name, sizeof name, "step_%04d.png", ++n);
                seam::save_image(seam::draw_seam(before, s, vertical),
                    (std::filesystem::path(steps_dir) / name).string());
                tracker.record(s, vertical);
                });
            seam::save_image(tracker.overview(in), (std::filesystem::path(steps_dir) / "overview.png").string());
        }
        seam::save_image(out, args[1]);
        std::cout << in.width << "x" << in.height << " -> " << out.width << "x" << out.height << "\n";
    }
    catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }
}
