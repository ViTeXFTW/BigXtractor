cmake_minimum_required(VERSION 3.20)
project(big VERSION 1.0.0 LANGUAGES CXX)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTING "Build tests" OFF)
option(BUILD_EXAMPLES "Build example programs" OFF)
option(INSTALL_STANDALONE "Install as standalone library" OFF)

# Create compile_commands.json link only when this is the top-level project
if(CMAKE_EXPORT_COMPILE_COMMANDS AND CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    file(CREATE_LINK
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
        COPY_ON_ERROR
        SYMBOLIC
    )
endif()

# Library sources
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
file(GLOB_RECURSE LIB_HEADERS "include/bigx/*.hpp")

# Create the library
add_library(big ${LIB_SOURCES} ${LIB_HEADERS})

# Add alias for namespaced targeting
add_library(big::big ALIAS big)

# Public include directories
target_include_directories(big
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Require C++20 for this library and its consumers
target_compile_features(big PUBLIC cxx_std_20)

# Compiler-specific flags (PRIVATE so they don't propagate to consuming projects)
if(MSVC)
  target_compile_options(big PRIVATE /W4 /permissive-)
else()
  target_compile_options(big PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# ============================================================
# Installation (for standalone use / find_package)
# ============================================================
if(INSTALL_STANDALONE)
  include(GNUInstallDirs)

  # Install the library
  install(TARGETS big
    EXPORT bigTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  # Install headers
  install(DIRECTORY include/bigx
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
  )

  # Create and install export targets
  install(EXPORT bigTargets
    FILE bigTargets.cmake
    NAMESPACE big::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/big
  )

  # Create config files for find_package(big)
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/big-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/big-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/big-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/big
  )

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/big-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/big-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/big
  )
endif()

# ============================================================
# Testing
# ============================================================
if(BUILD_TESTING)
  enable_testing()
  find_package(GTest QUIET)

  # If GTest not found, use the submodule
  if(NOT GTest_FOUND)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/googletest/CMakeLists.txt")
      message(STATUS "Using GTest from git submodule")
      set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
      add_subdirectory(lib/googletest EXCLUDE_FROM_ALL)
      set(GTEST_FOUND ON)
    else()
      message(WARNING "GTest not found and submodule not available. Run: git submodule update --init --recursive")
    endif()
  endif()

  if(GTEST_FOUND OR GTest_FOUND)
    add_subdirectory(tests)
  else()
    message(STATUS "GTest not found - skipping big library tests")
  endif()
endif()

# ============================================================
# Examples
# ============================================================
if(BUILD_EXAMPLES)
  add_executable(list_files examples/list_files.cpp)
  target_link_libraries(list_files PRIVATE big::big)

  add_executable(extract_files examples/extract_files.cpp)
  target_link_libraries(extract_files PRIVATE big::big)
endif()

# ============================================================
# Summary
# ============================================================
message(STATUS "")
message(STATUS "=== BIG Library Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "BUILD_TESTING: ${BUILD_TESTING}")
message(STATUS "BUILD_EXAMPLES: ${BUILD_EXAMPLES}")
message(STATUS "===================================")
message(STATUS "")
