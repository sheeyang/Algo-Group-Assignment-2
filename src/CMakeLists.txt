# Every .cpp in src/ is part of seamlib. Globbing means adding a file never edits this
# shared list (fewer merge conflicts across the 5 tasks). CONFIGURE_DEPENDS re-checks on build.
file(GLOB SEAMLIB_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(seamlib STATIC ${SEAMLIB_SOURCES})
target_include_directories(seamlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(seamlib SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/stb)
target_link_libraries(seamlib PUBLIC Threads::Threads)
if(MSVC)
  target_compile_options(seamlib PRIVATE /W4)
else()
  target_compile_options(seamlib PRIVATE -Wall -Wextra -Wpedantic)
endif()
