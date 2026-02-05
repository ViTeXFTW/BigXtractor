# BIG Archive Library

A modern C++20 library for reading and writing the `.big` file archive format used by Command & Conquer Generals and other Westwood Studios games.

## Features

- **Zero-copy file access** - Memory-mapped I/O for efficient handling of large archives
- **Full read/write support** - Create, read, and modify BIG archives
- **Cross-platform** - Windows, Linux, macOS
- **Header-only public API** - Simple integration with `#include <big/big.hpp>`
- **Case-insensitive file lookup** - Compatible with original game behavior
- **No external dependencies** - Standard library only (except for testing)

## Building

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build library only
cmake --build build

# Build with tests
cmake -B build -DBUILD_TESTING=ON
cmake --build build

# Build with examples
cmake -B build -DBUILD_EXAMPLES=ON
cmake --build build

# Run tests
ctest --test-dir build
```

## Installing

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

## Usage in Other Projects

### Option 1: Using find_package (after installation)

```cmake
# CMakeLists.txt
find_package(big REQUIRED)

add_executable(mytool main.cpp)
target_link_libraries(mytool PRIVATE big::big)
```

```cpp
// main.cpp
#include <big/big.hpp>

int main() {
    auto archive = big::Archive::open("myfile.big");
    if (archive) {
        for (const auto& file : archive->files()) {
            std::cout << file.path << "\n";
        }
    }
}
```

### Option 2: As a subdirectory

```cmake
# Add as subdirectory
add_subdirectory(external/big-lib)

# Link against the library
target_link_libraries(mytool PRIVATE big::big)
```

### Option 3: Git submodule

```bash
git submodule add https://github.com/yourusername/big-lib.git external/big-lib
git submodule update --init --recursive
```

Then use Option 2 above.

### Option 4: FetchContent (CMake 3.14+)

```cmake
include(FetchContent)
FetchContent_Declare(
  big
  GIT_REPOSITORY https://github.com/yourusername/big-lib.git
  GIT_TAG main
)
FetchContent_MakeAvailable(big)

target_link_libraries(mytool PRIVATE big::big)
```

## API Examples

### Reading an Archive

```cpp
#include <big/big.hpp>

// Open archive
auto archive = big::Archive::open("archive.big");
if (!archive) {
    std::cerr << "Failed to open archive\n";
    return 1;
}

// List all files
for (const auto& file : archive->files()) {
    std::cout << file.path << " (" << file.size << " bytes)\n";
}

// Find a file (case-insensitive)
const auto* file = archive->findFile("Data/Texture.tga");
if (file) {
    // Extract to disk
    archive->extract(*file, "output.tga");

    // Or read into memory
    auto data = archive->extractToMemory(*file);
    if (data) {
        // Use data->data(), data->size()
    }
}
```

### Creating an Archive

```cpp
#include <big/big.hpp>

auto archive = big::Archive::create();

// Add files from disk
archive.addFile("source.txt", "data/source.txt");

// Add files from memory
std::vector<uint8_t> data = {0, 1, 2, 3};
archive.addFile(data, "binary/data.bin");

// Write to disk
std::string error;
if (!archive.write("output.big", &error)) {
    std::cerr << "Error: " << error << "\n";
}
```

### Low-Level API

For more control, use the `Reader` and `Writer` classes directly:

```cpp
#include <big/reader.hpp>

auto reader = big::Reader::open("archive.big");
if (reader) {
    // Get zero-copty view of a file
    const auto* file = reader->findFile("path/file.txt");
    auto view = reader->getFileView(*file);
    // view.data() and view.size() give direct access
}
```

## BIG File Format

```
Header (16 bytes):
+0x00  char[4]    Magic: "BIGF"
+0x04  uint32     Archive size (big-endian, mostly unused)
+0x08  uint32     Number of files (big-endian)
+0x0C  uint32     Padding

File Entries (starting at 0x10):
+0x00  uint32     File offset in archive (big-endian)
+0x04  uint32     File size in bytes (big-endian)
+0x08  char[]     Null-terminated full path
```

## License

MIT

## Contributing

Contributions are welcome! Please ensure tests pass before submitting a PR.

## References

- Original implementation: [Generals Game Code](https://github.com/TheSuperHackers/GeneralsGameCode)
- W3D Viewer: [VulkanW3DViewer](https://github.com/vitexftw/VulkanW3DViewer)
