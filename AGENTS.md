# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a C++20 library for reading/writing the `.big` archive format used by Command & Conquer Generals and other Westwood Studios games. Key design: **zero-copy file access** via memory-mapped I/O.

## Build Commands

```bash
# Basic build
cmake -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release

# Build with tests
cmake -B build/test -DBUILD_TESTING=ON
cmake --build build/test

# Run tests
ctest --test-dir build/test

# Build with examples
cmake -B build/examples -DBUILD_EXAMPLES=ON
cmake --build build/examples

# Using CMake presets (configured for Ninja + clang)
cmake --preset release
cmake --build --preset release
cmake --preset test  # Debug + BUILD_TESTING=ON
ctest --preset test

# Using vcpkg for dependencies (requires VCPKG_ROOT env var)
cmake --preset vcpkg-test  # Builds with vcpkg-provided GTest
ctest --preset vcpkg-test
```

## Architecture

The library has a **3-layer API design**:

1. **Low-level**: `Reader` / `Writer` classes in [reader.hpp](include/bigx/reader.hpp), [writer.hpp](include/bigx/writer.hpp)
   - `Reader` opens archives via memory-mapped I/O, provides zero-copy `getFileView()`
   - `Writer` accumulates files in memory, writes final archive

2. **High-level**: `Archive` class in [archive.hpp](include/bigx/archive.hpp)
   - Unifies read/write: uses `Archive::open()` to read, `Archive::create()` to write
   - Internally holds either a `Reader` or `Writer` (exclusive states)

3. **Entry point**: [big.hpp](include/bigx/big.hpp) includes all public headers

## Key Implementation Details

- **BIG file format**: 16-byte header + file entries (offset, size, null-terminated path), stored **big-endian**
- **Case-insensitive lookup**: `FileEntry` stores both `path` (original case) and `lowercasePath` for lookup
- **Path normalization**: Backslashes converted to forward slashes, original case preserved for display
- **Error handling**: Functions return `std::optional` or `bool` with optional `outError` string parameter (no exceptions except `ParseError`)
- **Memory mapping**: Cross-platform implementation in [mmap.hpp](include/bigx/mmap.hpp) / [mmap.cpp](src/mmap.cpp)

## Project Structure

```
include/bigx/     # Public headers (header-only API consumers only need these)
src/              # Implementation files (.cpp)
tests/            # GTest tests (built when BUILD_TESTING=ON)
examples/         # Example programs (built when BUILD_EXAMPLES=ON)
cmake/            # CMake config templates
```

## Code Style

- Format with `.clang-format` (LLVM-based, 100 char line limit)
- C++20 features: `std::span`, `std::optional`, `std::filesystem`
- No external dependencies (except GTest for testing)
