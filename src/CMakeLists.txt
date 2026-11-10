add_library(seamlib STATIC
  image.cpp
  energy.cpp
  seam_dp.cpp
  seam_greedy.cpp
  seam_graphcut.cpp
  resize.cpp
)
target_include_directories(seamlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(seamlib SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/stb)
if(MSVC)
  target_compile_options(seamlib PRIVATE /W4)
else()
  target_compile_options(seamlib PRIVATE -Wall -Wextra -Wpedantic)
endif()
