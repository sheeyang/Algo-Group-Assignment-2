/**
 * @brief Owned RGB image storage, image I/O and transposition interfaces.
 * @file image.h
 */
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace seam {

/**
 * @brief Owned row-major, interleaved 8-bit RGB pixels.
 * @details Pixel (x,y) starts at (y * width + x) * 3. Data is copied with the
 * object; accessors do not validate indices or storage consistency.
 */
struct Image {
    int width = 0;  ///< Number of pixel columns.
    int height = 0;  ///< Number of pixel rows.
    std::vector<uint8_t> rgb;  ///< Interleaved RGB bytes; size is width * height * 3.

    /**
     * @brief Initialise empty storage, or allocate a zero-filled RGB image.
     * @pre For the sized constructor, dimensions are nonnegative and their RGB product fits the
     * allocation size.
     */
    Image() = default;
    /**
     * @brief Initialise empty storage, or allocate a zero-filled RGB image.
     * @param w Image/map width.
     * @param h Image/map height.
     * @pre For the sized constructor, dimensions are nonnegative and their RGB product fits the
     * allocation size.
     */
    Image(int w, int h) : width(w), height(h), rgb(size_t(w) * h * 3, 0) {}

    /**
     * @brief Access the first of three contiguous RGB bytes without bounds checking.
     * @param x Zero-based column coordinate.
     * @param y Zero-based row coordinate.
     * @return Pointer to this pixel; const overload prohibits writes.
     * @pre Coordinates lie inside the image and rgb contains width * height * 3 bytes.
     * @note Reallocating rgb invalidates returned pointers.
     */
    uint8_t* px(int x, int y) { return &rgb[(size_t(y) * width + x) * 3]; }
    /**
     * @brief Access the first of three contiguous RGB bytes without bounds checking.
     * @param x Zero-based column coordinate.
     * @param y Zero-based row coordinate.
     * @return Pointer to this pixel; const overload prohibits writes.
     * @pre Coordinates lie inside the image and rgb contains width * height * 3 bytes.
     * @note Reallocating rgb invalidates returned pointers.
     */
    const uint8_t* px(int x, int y) const { return &rgb[(size_t(y) * width + x) * 3]; }
};

/**
 * @brief Decode an image file into owned, three-channel RGB storage.
 * @param path Input/output filesystem path.
 * @return Decoded RGB image; source channel count is converted to three.
 * @throws std::runtime_error If stb cannot decode or open the input.
 */
Image load_image(const std::string& path);                   // throws std::runtime_error
/**
 * @brief Write RGB data as JPEG for lowercase .jpg/.jpeg suffixes, otherwise PNG.
 * @param img RGB image with storage matching its dimensions.
 * @param path Input/output filesystem path.
 * @pre img has valid dimensions and matching RGB storage.
 * @throws std::runtime_error If the image cannot be written.
 * @note JPEG uses quality 95 and is lossy; PNG preserves RGB values. Parent directories are not
 * created.
 */
void save_image(const Image& img, const std::string& path);  // .png or .jpg
/**
 * @brief Swap image axes while preserving every RGB pixel.
 * @param img RGB image with storage matching its dimensions.
 * @return A new image with swapped width/height; input is unchanged.
 * @pre Input RGB storage matches the dimensions.
 * @par Complexity
 * O(W H) time and output storage.
 */
Image transpose(const Image& img);                           // swap x/y (for horizontal seams)

}  // namespace seam
