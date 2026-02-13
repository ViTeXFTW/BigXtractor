# Test data directory for all test suites
set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# ============================================================
# Memory Mapping Tests
# ============================================================
add_executable(mmap_tests test_mmap.cpp)
target_link_libraries(mmap_tests
  PRIVATE
    bigx::bigx
    GTest::gtest
    GTest::gtest_main
)
target_compile_definitions(mmap_tests PRIVATE
  TEST_DATA_DIR="${TEST_DATA_DIR}"
)
if(MSVC)
  target_compile_options(mmap_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(mmap_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME mmap_tests COMMAND mmap_tests)

# ============================================================
# Reader Tests
# ============================================================
add_executable(reader_tests test_reader.cpp)
target_link_libraries(reader_tests
  PRIVATE
    bigx::bigx
    GTest::gtest
    GTest::gtest_main
)
target_compile_definitions(reader_tests PRIVATE
  TEST_DATA_DIR="${TEST_DATA_DIR}"
)
if(MSVC)
  target_compile_options(reader_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(reader_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME reader_tests COMMAND reader_tests)

# ============================================================
# Writer Tests
# ============================================================
add_executable(writer_tests test_writer.cpp)
target_link_libraries(writer_tests
  PRIVATE
    bigx::bigx
    GTest::gtest
    GTest::gtest_main
)
target_compile_definitions(writer_tests PRIVATE
  TEST_DATA_DIR="${TEST_DATA_DIR}"
)
if(MSVC)
  target_compile_options(writer_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(writer_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME writer_tests COMMAND writer_tests)

# ============================================================
# Archive Integration Tests
# ============================================================
add_executable(archive_tests
  test_archive_incomplete_type.cpp
  test_real_archive.cpp
  test_w3d_archive.cpp
)
target_link_libraries(archive_tests
  PRIVATE
    bigx::bigx
    GTest::gtest
    GTest::gtest_main
)
target_compile_definitions(archive_tests PRIVATE
  TEST_DATA_DIR="${TEST_DATA_DIR}"
)
if(MSVC)
  target_compile_options(archive_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(archive_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME archive_tests COMMAND archive_tests)
