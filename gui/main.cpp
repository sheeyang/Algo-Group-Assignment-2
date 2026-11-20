/**
 * @brief ImGui/ImPlot frontend with UI-thread OpenGL resource ownership.
 * @file main.cpp
 */
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "jobs.h"

namespace {
using seam::Algorithm;
using seam::Image;
using seam::gui::JobKind;
constexpr std::array<Algorithm, 4> algorithms{
    Algorithm::DP, Algorithm::Greedy, Algorithm::GraphCutEK, Algorithm::GraphCutDinic};
constexpr const char* names[]{
    "Dynamic programming", "Greedy", "Graph cut / Edmonds-Karp", "Graph cut / Dinic"};

/**
 * @brief Copy a path into a fixed-size, null-terminated UI buffer.
 * @param buffer Fixed-size destination buffer, including its null terminator.
 * @param path Input/output filesystem path.
 * @tparam N Capacity including the terminating null byte.
 * @throws std::runtime_error If the path does not fit.
 */
template <size_t N>
void set_path(char (&buffer)[N], const std::string& path) {
    if (path.size() >= N) throw std::runtime_error("That file path is too long.");
    std::memcpy(buffer, path.c_str(), path.size() + 1);
}

/**
 * @brief Validate an output path and create its parent directories.
 * @param path Input/output filesystem path.
 * @throws std::runtime_error For an empty path.
 * @note Filesystem exceptions propagate to the UI error handler.
 */
void prepare_output(const char* path) {
    if (!*path) throw std::runtime_error("Enter an output file path.");
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
}

// Textures belong to the main thread and die before the OpenGL context does.
/**
 * @brief Unique OpenGL texture owner whose operations stay on the UI thread.
 * @details Uploads preserve caller binding/alignment. Destruction must precede
 * destruction of the associated current OpenGL context.
 */
class Texture {
public:
    /**
     * @brief Initialise unique UI-thread ownership of an OpenGL texture.
     * @note Copy construction/assignment are intentionally deleted.
     */
    Texture() = default;
    /**
     * @brief Initialise unique UI-thread ownership of an OpenGL texture.
     * @note Copy construction/assignment are intentionally deleted.
     */
    Texture(const Texture&) = delete;
    /**
     * @brief Prevent copying of uniquely owned OpenGL texture resources.
     */
    Texture& operator=(const Texture&) = delete;
    /**
     * @brief Release the owned GPU texture before its OpenGL context is destroyed.
     */
    ~Texture() { clear(); }
    /**
     * @brief Exchange texture handles without uploading pixels.
     * @param other Texture whose handle is exchanged.
     */
    void swap(Texture& other) { std::swap(id_, other.id_); }
    /**
     * @brief Delete the owned GPU texture, if any, and reset the handle.
     * @pre Called on the UI thread with its OpenGL context current.
     */
    void clear() {
        if (id_) glDeleteTextures(1, &id_);
        id_ = 0;
    }
    /**
     * @brief Read the texture handle used by ImGui.
     * @return ImGui texture identifier; zero when empty.
     */
    ImTextureID id() const { return static_cast<ImTextureID>(id_); }
    /**
     * @brief Upload RGB pixels, restoring prior GL binding and unpack alignment.
     * @param image Input RGB pixels.
     * @throws std::runtime_error For invalid texture dimensions or a GL upload error.
     * @pre Called on the UI thread with a current GL context and valid RGB storage.
     */
    void assign(const Image& image) {
        GLint limit = 0, binding = 0, alignment = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
        if (image.width < 1 || image.height < 1 || image.width > limit || image.height > limit)
            throw std::runtime_error("This image exceeds the graphics card's texture size limit.");
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
        GLuint next = 0;
        glGenTextures(1, &next);
        glBindTexture(GL_TEXTURE_2D, next);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Windows' system GL header only declares 1.1 constants; this is the
        // standard GL_CLAMP_TO_EDGE value, supported by our 3.3 context.
        constexpr GLint clamp_to_edge = 0x812F;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp_to_edge);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp_to_edge);
        // RGB rows need not be a multiple of OpenGL's default four-byte alignment.
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width, image.height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, image.rgb.data());
        const GLenum error = glGetError();
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(binding));
        if (error != GL_NO_ERROR) {
            glDeleteTextures(1, &next);
            throw std::runtime_error("Could not upload the image to the graphics card.");
        }
        clear();
        id_ = next;
    }
private:
    GLuint id_ = 0;
};

/**
 * @brief Display an image fitted within an ImGui child panel without stretching.
 * @param title Panel heading and ImGui child identifier.
 * @param texture UI-owned texture backing the displayed image.
 * @param image Input RGB pixels.
 * @param size Available panel dimensions.
 * @param empty Placeholder text when no image is present.
 * @pre Called within an ImGui frame on the UI thread.
 */
void image_panel(const char* title, const Texture& texture, const Image& image,
                 ImVec2 size, const char* empty) {
    ImGui::TextUnformatted(title);
    if (image.width) {
        ImGui::SameLine();
        ImGui::TextDisabled("%d x %d", image.width, image.height);
    }
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.055f, 0.075f, 0.105f, 1));
    ImGui::BeginChild(title, size, ImGuiChildFlags_Borders);
    const auto available = ImGui::GetContentRegionAvail();
    if (image.width > 0) {
        const float scale = std::max(0.001f, std::min({
            available.x / float(image.width), available.y / float(image.height), 1.0f}));
        const ImVec2 display(float(image.width) * scale, float(image.height) * scale);
        const auto cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(cursor.x + std::max(0.0f, (available.x - display.x) / 2),
                                  cursor.y + std::max(0.0f, (available.y - display.y) / 2)));
        ImGui::Image(texture.id(), display);
    } else {
        ImGui::TextWrapped("%s", empty);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

/**
 * @brief Configure the frontend colours, spacing and widget appearance.
 * @pre An ImGui context exists on the calling thread.
 */
void apply_style() {
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(20, 18);
    style.FramePadding = ImVec2(10, 7);
    style.ItemSpacing = ImVec2(10, 10);
    style.WindowRounding = 0;
    style.ChildRounding = 8;
    style.FrameRounding = 5;
    style.GrabRounding = 5;
    style.ScrollbarRounding = 5;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.075f, 0.095f, 0.13f, 1);
    style.Colors[ImGuiCol_Text] = ImVec4(0.91f, 0.94f, 0.97f, 1);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.54f, 0.62f, 0.72f, 1);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.16f, 0.21f, 1);
    style.Colors[ImGuiCol_Button] = ImVec4(0.12f, 0.28f, 0.34f, 1);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.15f, 0.41f, 0.47f, 1);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.49f, 0.54f, 1);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.25f, 0.77f, 0.77f, 1);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.39f, 0.91f, 0.88f, 1);
    style.Colors[ImGuiCol_CheckMark] = style.Colors[ImGuiCol_SliderGrab];
    style.Colors[ImGuiCol_TabSelected] = ImVec4(0.13f, 0.29f, 0.36f, 1);
}

/**
 * @brief UI-thread owner of scene data, textures, playback and worker results.
 * @details Long CPU operations are delegated to Jobs; only completed results
 * are uploaded to OpenGL. User-visible errors are captured by attempt().
 */
class App {
public:
    /**
     * @brief Attach application state to an existing GLFW window.
     * @param window Borrowed GLFW window; its lifetime must exceed the application state.
     * @pre window and its current GL context outlive the application and textures.
     */
    explicit App(GLFWwindow* window) : window_(window) {}
    /**
     * @brief Run a UI action and convert standard exceptions into visible error state.
     * @param action Callable UI operation whose errors become visible status.
     * @tparam Fn Callable action type.
     */
    template <class Fn> void attempt(Fn&& action) {
        try { action(); }
        catch (const std::exception& ex) { message_ = ex.what(); error_ = true; }
    }
    /**
     * @brief Prepare CPU/GPU image data before replacing the current scene and results.
     * @param path Input/output filesystem path.
     * @throws std::runtime_error If a job is active, path is too long or image/texture loading
     * fails.
     */
    void load(const std::string& path) {
        if (jobs_.busy()) throw std::runtime_error("Wait for the current job before loading another image.");
        // Prepare both CPU and GPU data before replacing the current image.
        Image next = seam::load_image(path);
        Texture texture;
        texture.assign(next);
        set_path(input_path_, path);
        original_ = std::move(next);
        original_texture_.swap(texture);
        result_ = {};
        result_texture_.clear();
        overview_ = {};
        overview_texture_.clear();
        step_image_ = {};
        step_texture_.clear();
        history_.clear();
        replay_image_ = {};
        cached_step_ = -1;
        playing_ = false;
        rows_.clear();
        notes_.clear();
        last_ms_ = last_cost_ = 0;
        width_ = std::max(1, original_.width * 3 / 4);
        height_ = original_.height;
        message_ = "Image loaded. Choose a target size, then resize.";
        error_ = false;
    }
    /**
     * @brief Request cooperative cancellation before allowing the window to close.
     * @note A running seam must finish before cancellation can be observed.
     */
    void request_close() {
        if (jobs_.busy()) {
            closing_ = true;
            playing_ = false;
            jobs_.cancel();
            glfwSetWindowShouldClose(window_, GLFW_FALSE);
        }
    }
    /**
     * @brief Poll worker completion and finish a deferred close when safe.
     */
    void update() {
        attempt([&] { poll(); });
        if (closing_ && !jobs_.busy()) glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }
    /**
     * @brief Render controls and workspace tabs for the current ImGui frame.
     */
    void draw() {
        const auto* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("Seam Studio", nullptr, ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextColored(ImVec4(0.35f, 0.84f, 0.82f, 1), "SEAM STUDIO");
        ImGui::SameLine();
        ImGui::TextDisabled("Content-aware image resizing");
        ImGui::Separator();
        if (ImGui::BeginTable("layout", 2, ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthFixed, 340);
            ImGui::TableSetupColumn("Workspace", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::BeginChild("controls", ImVec2(0, 0));
            ImGui::BeginDisabled(closing_);
            controls();
            ImGui::EndDisabled();
            ImGui::EndChild();
            ImGui::TableSetColumnIndex(1);
            ImGui::BeginChild("workspace", ImVec2(0, 0));
            if (closing_) ImGui::TextWrapped("Stopping the current job before closing...");
            else if (error_) ImGui::TextColored(ImVec4(1, 0.55f, 0.5f, 1), "Something needs attention");
            ImGui::PushTextWrapPos();
            ImGui::TextUnformatted(message_.c_str());
            ImGui::PopTextWrapPos();
            if (ImGui::BeginTabBar("views")) {
                if (ImGui::BeginTabItem("Preview")) {
                    preview();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Removed seams")) {
                    attempt([&] { playback(); });
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Benchmarks")) {
                    benchmark();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndChild();
            ImGui::EndTable();
        }
        ImGui::End();
    }
private:
    /**
     * @brief Transfer a completed worker result into UI-owned textures and state.
     * @throws std::runtime_error If the worker reported an error.
     * @note Cancelled resizes retain the previously completed result.
     */
    void poll() {
        auto ready = jobs_.take_ready();
        if (!ready) return;
        if (!ready->error.empty()) throw std::runtime_error(ready->error);
        error_ = false;
        if (ready->kind == JobKind::Resize) {
            if (ready->cancelled) {
                message_ = "Resize cancelled. The last completed result is still shown.";
                return;
            }
            Texture result_texture, overview_texture;
            result_texture.assign(ready->image);
            overview_texture.assign(ready->overview);
            result_ = std::move(ready->image);
            overview_ = std::move(ready->overview);
            result_texture_.swap(result_texture);
            overview_texture_.swap(overview_texture);
            history_ = std::move(ready->history);
            last_ms_ = ready->ms;
            last_cost_ = ready->total_cost;
            last_algorithm_ = ready->algorithm;
            step_number_ = 1;
            cached_step_ = -1;
            replay_image_ = {};
            playing_ = false;
            message_ = "Resize complete. Export the result or explore the removed seams.";
        } else {
            rows_ = std::move(ready->rows);
            notes_ = std::move(ready->notes);
            message_ = ready->cancelled
                ? "Benchmark stopped. Completed measurements are available for export."
                : "Benchmark complete. The chart and CSV use the same measurements.";
        }
    }
    /**
     * @brief Render image loading, resize settings, progress, cancellation and export controls.
     */
    void controls() {
        ImGui::PushTextWrapPos();
        ImGui::SeparatorText("1  Open an image");
        ImGui::BeginDisabled(jobs_.busy());
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##input", "Image file path", input_path_, sizeof(input_path_));
        if (ImGui::Button("Load image", ImVec2(-1, 0))) attempt([&] { load(input_path_); });
        ImGui::TextDisabled("Or drop an image here.");
        ImGui::EndDisabled();

        ImGui::SeparatorText("2  Resize");
        ImGui::BeginDisabled(original_.width == 0 || jobs_.busy());
        ImGui::TextUnformatted("Width");
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderInt("##width", &width_, 1, std::max(1, original_.width), "%d px",
                         ImGuiSliderFlags_AlwaysClamp);
        ImGui::TextUnformatted("Height");
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderInt("##height", &height_, 1, std::max(1, original_.height), "%d px",
                         ImGuiSliderFlags_AlwaysClamp);
        for (int percent : {90, 75, 50}) {
            if (percent != 90) ImGui::SameLine();
            const std::string label = std::to_string(percent) + "%";
            if (ImGui::Button(label.c_str())) {
                width_ = std::max(1, original_.width * percent / 100);
                height_ = std::max(1, original_.height * percent / 100);
            }
        }
        ImGui::TextUnformatted("Algorithm");
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##algorithm", &algorithm_, names, 4);
        if (algorithm_ == 0) ImGui::TextWrapped("Finds a seam with minimum total cost.");
        else if (algorithm_ == 1) ImGui::TextWrapped("Chooses each next pixel locally, with no look-ahead.");
        else ImGui::TextWrapped("Finds a minimum cut. Start with a small image; max-flow can be slow.");
        if (ImGui::Button("Resize image", ImVec2(-1, 36))) attempt([&] {
            playing_ = false;
            jobs_.start_resize(original_, width_, height_, algorithms[size_t(algorithm_)]);
            message_ = "Resizing in the background. The previous result stays visible until completion.";
            error_ = false;
        });
        ImGui::TextDisabled("Resizes start from the original.");
        ImGui::EndDisabled();

        if (jobs_.busy()) {
            ImGui::SeparatorText("Working");
            const int total = jobs_.total(), done = jobs_.completed();
            ImGui::Text("%d / %d %s", done, total,
                jobs_.kind() == JobKind::Resize ? "seams" : "algorithms");
            ImGui::ProgressBar(total > 0 ? float(done) / float(total) : 0, ImVec2(-1, 0));
            ImGui::Text("Elapsed: %.2f s", jobs_.elapsed_ms() / 1000);
            if (jobs_.kind() == JobKind::Resize) ImGui::Text("Seam cost so far: %.3f", jobs_.cost());
            ImGui::BeginDisabled(jobs_.cancelling());
            if (ImGui::Button(jobs_.cancelling() ? "Stopping..." : "Cancel", ImVec2(-1, 0)))
                jobs_.cancel();
            ImGui::EndDisabled();
            ImGui::TextWrapped(jobs_.kind() == JobKind::Resize
                ? "Cancellation waits for the current seam to finish."
                : "Cancellation waits for the current algorithm to finish.");
        }
        if (result_.width) {
            ImGui::SeparatorText("Last completed resize");
            ImGui::TextWrapped("%s", names[static_cast<size_t>(last_algorithm_)]);
            ImGui::Text("%d x %d  |  %zu seams", result_.width, result_.height, history_.size());
            ImGui::Text("Elapsed: %.2f ms", last_ms_);
            ImGui::Text("Total seam cost: %.3f", last_cost_);
            ImGui::TextDisabled("Time includes tracking and cost.");
        }
        ImGui::SeparatorText("3  Export result");
        ImGui::BeginDisabled(result_.width == 0 || jobs_.busy());
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##output", "Output PNG or JPG path", output_path_, sizeof(output_path_));
        if (ImGui::Button("Export image", ImVec2(-1, 0))) attempt([&] {
            prepare_output(output_path_);
            seam::save_image(result_, output_path_);
            message_ = std::string("Image saved to ") + output_path_;
            error_ = false;
        });
        ImGui::EndDisabled();
        ImGui::PopTextWrapPos();
    }
    /**
     * @brief Render original/result image panels fitted to available space.
     */
    void preview() {
        ImGui::TextDisabled("Original and result are fitted to their panels without stretching.");
        if (ImGui::BeginTable("images", 2, ImGuiTableFlags_SizingStretchSame)) {
            const float panel_height = std::max(180.0f, ImGui::GetContentRegionAvail().y - 45);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            image_panel("Original", original_texture_, original_, ImVec2(0, panel_height),
                        "Load an image using the controls on the left.");
            ImGui::TableSetColumnIndex(1);
            image_panel("Result", result_texture_, result_, ImVec2(0, panel_height),
                        "Choose a target size and press Resize image.");
            ImGui::EndTable();
        }
    }
    /**
     * @brief Replay seam history with cached state and render removal controls.
     */
    void playback() {
        if (history_.empty()) {
            ImGui::TextWrapped("Resize an image to record the seams. A run that removes no pixels has no steps.");
            return;
        }
        ImGui::Checkbox("Show all removed seams", &show_overview_);
        if (show_overview_) {
            ImGui::TextDisabled("Red: vertical seams   |   Cyan: horizontal seams");
            image_panel("Seam overview", overview_texture_, overview_,
                ImVec2(0, std::max(150.0f, ImGui::GetContentRegionAvail().y - 35)), "");
            return;
        }
        if (ImGui::Button(playing_ ? "Pause" : "Play")) {
            playing_ = !playing_;
            if (playing_ && step_number_ == int(history_.size())) step_number_ = 1;
            next_step_time_ = ImGui::GetTime() + 0.2;
        }
        ImGui::SameLine();
        if (ImGui::Button("Previous")) { step_number_ = std::max(1, step_number_ - 1); playing_ = false; }
        ImGui::SameLine();
        if (ImGui::Button("Next")) {
            step_number_ = std::min(int(history_.size()), step_number_ + 1);
            playing_ = false;
        }
        ImGui::SetNextItemWidth(-1);
        if (ImGui::SliderInt("##step", &step_number_, 1, int(history_.size()), "Step %d",
                             ImGuiSliderFlags_AlwaysClamp)) playing_ = false;
        if (playing_ && ImGui::GetTime() >= next_step_time_) {
            if (step_number_ < int(history_.size())) ++step_number_;
            else playing_ = false;
            next_step_time_ = ImGui::GetTime() + 0.2;
        }
        const int index = step_number_ - 1;
        if (cached_step_ != index) {
            // Keep one replay checkpoint, rather than a full RGB frame for
            // every removed seam. Seeking backward replays from the original.
            const bool forward = cached_step_ >= 0 && index >= cached_step_;
            replay_image_ = seam::replay(forward ? replay_image_ : original_, history_,
                                         forward ? cached_step_ : 0, index);
            const auto& record = history_[size_t(index)];
            step_image_ = seam::draw_seam(replay_image_, record.seam, record.vertical);
            step_texture_.assign(step_image_);
            cached_step_ = index;
        }
        ImGui::Text("Step %d of %zu: %s seam, shown before removal", step_number_, history_.size(),
                    history_[size_t(index)].vertical ? "vertical" : "horizontal");
        image_panel("Step frame", step_texture_, step_image_,
            ImVec2(0, std::max(150.0f, ImGui::GetContentRegionAvail().y - 35)), "");
    }
    /**
     * @brief Render benchmark selection, measured plots, table and CSV export controls.
     */
    void benchmark() {
        ImGui::TextWrapped("Compare the same number of vertical seam removals at several image sizes.");
        ImGui::BeginDisabled(original_.width == 0 || jobs_.busy() || closing_);
        for (size_t i = 0; i < algorithms.size(); ++i) {
            if (i % 2) ImGui::SameLine();
            ImGui::Checkbox(names[i], &bench_selected_[i]);
        }
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Seams per run", &bench_seams_);
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Repeats", &bench_repeats_);
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Time budget per run (ms)", &bench_budget_);
        bench_seams_ = std::clamp(bench_seams_, 1, 1000);
        bench_repeats_ = std::clamp(bench_repeats_, 1, 25);
        bench_budget_ = std::clamp(bench_budget_, 1, 60000);
        if (ImGui::Button("Run benchmark")) attempt([&] {
            seam::gui::BenchmarkOptions options;
            options.algorithms.clear();
            for (size_t i = 0; i < algorithms.size(); ++i)
                if (bench_selected_[i]) options.algorithms.push_back(algorithms[i]);
            options.seams = bench_seams_;
            options.repeats = bench_repeats_;
            options.budget_ms = bench_budget_;
            jobs_.start_benchmark(original_, std::move(options));
            message_ = "Benchmark running. Completed results will replace the previous chart.";
            error_ = false;
        });
        ImGui::EndDisabled();
        ImGui::TextDisabled("Sizes: 12.5%%, 25%%, 50%%, 100%%. Median of repeated runs.");
        ImGui::TextWrapped("The time budget is checked between seams; one slow max-flow call can exceed it.");
        if (!rows_.empty()) {
            if (ImPlot::BeginPlot("Resize time by image size", ImVec2(-1, 260))) {
                ImPlot::SetupAxes("Input pixels", "Time (ms)", ImPlotAxisFlags_AutoFit,
                                   ImPlotAxisFlags_AutoFit);
                for (Algorithm algorithm : algorithms) {
                    std::vector<double> x, y;
                    for (const auto& row : rows_) if (row.algorithm == seam::algorithm_name(algorithm)) {
                        x.push_back(double(row.width) * row.height);
                        y.push_back(row.ms);
                    }
                    if (!x.empty()) {
                        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
                        ImPlot::PlotLine(seam::algorithm_name(algorithm), x.data(), y.data(), int(x.size()));
                    }
                }
                ImPlot::EndPlot();
            }
            ImGui::BeginDisabled(jobs_.busy());
            ImGui::SetNextItemWidth(std::max(160.0f, ImGui::GetContentRegionAvail().x - 120));
            ImGui::InputText("##csv", csv_path_, sizeof(csv_path_));
            ImGui::SameLine();
            if (ImGui::Button("Export CSV")) attempt([&] {
                prepare_output(csv_path_);
                std::ofstream file(csv_path_);
                if (!file) throw std::runtime_error("Cannot open the CSV output file.");
                seam::write_csv(rows_, file);
                file.close();
                if (!file) throw std::runtime_error("Could not finish writing the CSV.");
                message_ = std::string("Benchmark CSV saved to ") + csv_path_;
                error_ = false;
            });
            ImGui::EndDisabled();
            if (ImGui::BeginTable("measurements", 5,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
                ImVec2(0, 145))) {
                for (const char* column : {"Algorithm", "Size", "Seams", "Time (ms)", "Seam cost"})
                    ImGui::TableSetupColumn(column);
                ImGui::TableHeadersRow();
                for (const auto& row : rows_) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(row.algorithm.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%d x %d", row.width, row.height);
                    ImGui::TableNextColumn(); ImGui::Text("%d", row.seams);
                    ImGui::TableNextColumn(); ImGui::Text("%.3f", row.ms);
                    ImGui::TableNextColumn(); ImGui::Text("%.3f", row.total_cost);
                }
                ImGui::EndTable();
            }
        }
        for (const auto& note : notes_) ImGui::TextWrapped("%s", note.c_str());
    }

    GLFWwindow* window_;
    seam::gui::Jobs jobs_;
    Image original_, result_, overview_, replay_image_, step_image_;
    Texture original_texture_, result_texture_, overview_texture_, step_texture_;
    std::vector<seam::SeamRecord> history_;
    std::vector<seam::BenchRow> rows_;
    std::vector<std::string> notes_;
    char input_path_[4096] = "testdata/landscape-tower.png";
    char output_path_[4096] = "out/resized.png";
    char csv_path_[4096] = "results/gui-benchmark.csv";
    std::string message_ = "Open an image to begin.";
    bool error_ = false, closing_ = false, playing_ = false, show_overview_ = false;
    int width_ = 1, height_ = 1, algorithm_ = 0;
    int step_number_ = 1, cached_step_ = -1;
    bool bench_selected_[4]{true, true, false, false};
    int bench_seams_ = 10, bench_repeats_ = 3, bench_budget_ = 1500;
    Algorithm last_algorithm_ = Algorithm::DP;
    double last_ms_ = 0, last_cost_ = 0, next_step_time_ = 0;
};
}  // namespace

/**
 * @brief ImGui/ImPlot frontend with UI-thread OpenGL resource ownership.
 * @param argc Argument count, including the executable name.
 * @param argv Null-terminated command-line argument strings.
 * @return 0 on normal shutdown/help, 1 if graphics initialisation fails.
 * @note Run benchmark/evidence tools from the repository root.
 */
int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--help") {
        std::puts("Usage: seamgui [image.png]\nOpen an image by path or drag-and-drop, select a target size, and resize.");
        return 0;
    }
    glfwSetErrorCallback([](int code, const char* description) {
        std::fprintf(stderr, "GLFW error %d: %s\n", code, description);
    });
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(1440, 900, "Seam Studio - Seam Carving", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwSetWindowSizeLimits(window, 1050, 700, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::GetIO().IniFilename = nullptr;
    ImFontConfig font;
    font.SizePixels = 18;
    ImGui::GetIO().Fonts->AddFontDefault(&font);
    apply_style();
    const bool platform_ready = ImGui_ImplGlfw_InitForOpenGL(window, true);
    const bool renderer_ready = platform_ready && ImGui_ImplOpenGL3_Init("#version 330");
    if (renderer_ready) {
        App app(window);
        glfwSetWindowUserPointer(window, &app);
        glfwSetDropCallback(window, [](GLFWwindow* target, int count, const char** paths) {
            if (count > 0) {
                auto& state = *static_cast<App*>(glfwGetWindowUserPointer(target));
                state.attempt([&] { state.load(paths[0]); });
            }
        });
        if (argc > 1) app.attempt([&] { app.load(argv[1]); });
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            if (glfwWindowShouldClose(window)) app.request_close();
            app.update();
            if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
                glfwWaitEventsTimeout(0.05);
                continue;
            }
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            app.draw();
            ImGui::Render();
            int width = 0, height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            glViewport(0, 0, width, height);
            glClearColor(0.075f, 0.095f, 0.13f, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }
    } else {
        std::fputs("Could not initialize the GUI graphics backend.\n", stderr);
    }
    if (renderer_ready) ImGui_ImplOpenGL3_Shutdown();
    if (platform_ready) ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return renderer_ready ? 0 : 1;
}
