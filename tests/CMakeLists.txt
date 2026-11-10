# Every .cpp in tests/ is part of the test binary (name files test_<topic>.cpp).
file(GLOB SEAM_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(seam_tests ${SEAM_TEST_SOURCES})
target_include_directories(seam_tests PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
target_link_libraries(seam_tests PRIVATE seamlib)
add_test(NAME seam_tests COMMAND seam_tests)
