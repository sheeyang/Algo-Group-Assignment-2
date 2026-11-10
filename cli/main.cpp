// Usage: seamcli <in> <out> <target_w> <target_h> [dp|greedy|ek|dinic]
#include <cstdlib>
#include <iostream>
#include <string>

#include "resize.h"

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "usage: seamcli <in> <out> <target_w> <target_h> [dp|greedy|ek|dinic]\n";
        return 2;
    }
    try {
        seam::Algorithm algo = seam::Algorithm::DP;
        if (argc > 5) {
            std::string a = argv[5];
            if (a == "greedy") algo = seam::Algorithm::Greedy;
            else if (a == "ek") algo = seam::Algorithm::GraphCutEK;
            else if (a == "dinic") algo = seam::Algorithm::GraphCutDinic;
            else if (a != "dp") {
                std::cerr << "unknown algorithm: " << a << "\n";
                return 2;
            }
        }
        seam::Image in = seam::load_image(argv[1]);
        seam::Image out = seam::resize(in, std::atoi(argv[3]), std::atoi(argv[4]), algo);
        seam::save_image(out, argv[2]);
        std::cout << in.width << "x" << in.height << " -> " << out.width << "x" << out.height << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }
}
