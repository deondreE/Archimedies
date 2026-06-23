set(GTEST_VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/gtest")

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

add_subdirectory(${GTEST_VENDOR_DIR} "${CMAKE_BINARY_DIR}/googletest_build")

add_executable(archimedes_unit_tests
  test_kernel.cpp
  test_entity.cpp
  test_maths.cpp
  test_serializer.cpp
)

target_link_libraries(archimedes_unit_tests
  PRIVATE
  archimedes_core
  GTest::gtest
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(archimedes_unit_tests)
