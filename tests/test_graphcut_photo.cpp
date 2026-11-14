#include <cmath>
#include <filesystem>

#include "doctest.h"
#include "resize.h"

using namespace seam;

TEST_CASE("EK agrees with DP while resizing the supplied castle photo") {
    // Keep the original JPEG in the repo. Sample the whole scene into a small
    // image so the normal test suite can exercise EK without a long runtime.
    const auto path = std::filesystem::path(SEAM_TESTDATA_DIR) / "c3" / "castle.jpeg";
    const Image original = load_image(path.string());
    REQUIRE(original.width >= 64);
    REQUIRE(original.height >= 43);
    Image sample(64, 43);
    for (int y = 0; y < sample.height; ++y)
        for (int x = 0; x < sample.width; ++x) {
            const auto* source = original.px(x * original.width / sample.width,
                                             y * original.height / sample.height);
            for (int c = 0; c < 3; ++c) sample.px(x, y)[c] = source[c];
        }

    int vertical = 0, horizontal = 0;
    const Image result = resize(sample, 62, 41, Algorithm::GraphCutEK,
        [&](const Image& before, const Seam& selected, bool is_vertical) {
            // Horizontal selection runs on the transpose. Compare costs on
            // this exact current image, since each removal changes its energy.
            const auto energy = sobel_energy(is_vertical ? before : transpose(before));
            REQUIRE(selected.size() == size_t(energy.height));
            for (int y = 0; y < energy.height; ++y) {
                REQUIRE(selected[y] >= 0);
                REQUIRE(selected[y] < energy.width);
                if (y > 0) CHECK(std::abs(selected[y] - selected[y - 1]) <= 1);
            }
            CHECK(seam_cost(energy, selected) ==
                  doctest::Approx(seam_cost(energy, find_seam_dp(energy))));
            if (is_vertical) ++vertical;
            else ++horizontal;
        });
    CHECK(result.width == 62);
    CHECK(result.height == 41);
    CHECK(vertical == 2);
    CHECK(horizontal == 2);
}
