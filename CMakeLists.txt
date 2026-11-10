cmake_minimum_required(VERSION 3.20)
project(SeamCarving LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# MinGW: link the runtime statically so a different libstdc++ DLL on PATH can't break startup.
if(MINGW)
  add_link_options(-static)
endif()

option(SEAM_BUILD_GUI "Build the Dear ImGui / ImPlot frontend (needs network on first configure)" OFF)
option(SEAM_BUILD_TESTS "Build unit tests" ON)

add_subdirectory(src)
add_subdirectory(cli)

if(SEAM_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(SEAM_BUILD_GUI)
  add_subdirectory(gui)
endif()
