# Every .cpp in tests/ is part of the test binary (name files test_<topic>.cpp).
file(GLOB SEAM_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(seam_tests ${SEAM_TEST_SOURCES})
target_include_directories(seam_tests PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
target_link_libraries(seam_tests PRIVATE seamlib)
# Resolve photo fixtures independently of the working directory used by CTest.
target_compile_definitions(seam_tests PRIVATE SEAM_TESTDATA_DIR="${PROJECT_SOURCE_DIR}/testdata")
# Use doctest's standard-header mode instead of forward declarations in std,
# which newer MSVC versions diagnose with C5285.
target_compile_definitions(seam_tests PRIVATE DOCTEST_CONFIG_USE_STD_HEADERS)
add_test(NAME seam_tests COMMAND seam_tests)

# Exercise the executable's parser, including omitted versus explicit dimensions.
add_test(NAME seam_cli_dimensions COMMAND ${CMAKE_COMMAND}
  -DCLI=$<TARGET_FILE:seamcli>
  -DINPUT=${PROJECT_SOURCE_DIR}/testdata/c3/building-small.png
  -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/cli-dimensions
  -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cli_dimensions.cmake)
