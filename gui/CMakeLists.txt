# Dependencies are fetched only when SEAM_BUILD_GUI is enabled. Fixed release
# tags ensure teammates build against the same frontend APIs.
include(FetchContent)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
enable_language(C) # GLFW is C; the rest of the project remains C++17.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
# Matches the X11 packages installed by the existing optional GUI CI job.
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
  URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz)
FetchContent_Declare(imgui
  URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.9b.tar.gz)
FetchContent_Declare(implot
  URL https://github.com/epezent/implot/archive/refs/tags/v0.17.tar.gz)
FetchContent_MakeAvailable(glfw imgui implot)
find_package(OpenGL REQUIRED)

add_library(seam_imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(seam_imgui SYSTEM PUBLIC
  ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(seam_imgui PUBLIC glfw OpenGL::GL ${CMAKE_DL_LIBS})
add_library(seam_implot STATIC
  ${implot_SOURCE_DIR}/implot.cpp ${implot_SOURCE_DIR}/implot_items.cpp)
target_include_directories(seam_implot SYSTEM PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(seam_implot PUBLIC seam_imgui)

# The worker has no graphics dependency, so its lifecycle can be tested without
# opening a window. It calls seamlib instead of reimplementing the algorithms.
add_library(seam_gui_jobs STATIC jobs.cpp)
target_include_directories(seam_gui_jobs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(seam_gui_jobs PUBLIC seamlib)
add_executable(seamgui main.cpp)
target_link_libraries(seamgui PRIVATE seam_gui_jobs seam_implot)
foreach(target seam_gui_jobs seamgui)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endforeach()
if(SEAM_BUILD_TESTS)
  add_executable(seam_gui_tests test_jobs.cpp)
  target_include_directories(seam_gui_tests PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
  target_link_libraries(seam_gui_tests PRIVATE seam_gui_jobs)
  add_test(NAME seam_gui_tests COMMAND seam_gui_tests)
  set_tests_properties(seam_gui_tests PROPERTIES TIMEOUT 30)
endif()
