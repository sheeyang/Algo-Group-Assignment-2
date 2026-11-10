# TODO(GUI owner): Dear ImGui + ImPlot on GLFW + OpenGL3 (see docs/adr/0002).
# Pull imgui, implot and glfw with FetchContent pinned to release tags, then:
#   add_executable(seamgui main.cpp)
#   target_link_libraries(seamgui PRIVATE seamlib imgui implot glfw)
# Enable with: cmake -S . -B build -DSEAM_BUILD_GUI=ON
message(STATUS "gui/: not implemented yet")
