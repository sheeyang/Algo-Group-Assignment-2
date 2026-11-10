add_executable(seam_tests test_main.cpp test_dp.cpp)
target_include_directories(seam_tests PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
target_link_libraries(seam_tests PRIVATE seamlib)
add_test(NAME seam_tests COMMAND seam_tests)
