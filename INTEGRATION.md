# Integrating BigXtractor as a Dependency

## Option 1: Git Submodule (Recommended)

### 1. Add the submodule

```bash
cd /path/to/your/project
git submodule add https://github.com/ViTeXFTW/BigXtractor.git external/BigXtractor
git submodule update --init --recursive
```

### 2. Update your CMakeLists.txt

```cmake
# Add the BigXtractor subdirectory
add_subdirectory(external/BigXtractor)

# Link against the library
target_link_libraries(your_target PRIVATE bigx::bigx)
```

### 3. Include headers in your code

```cpp
#include <bigx/bigx.hpp>

// Or specific headers:
// #include <bigx/archive.hpp>
// #include <bigx/reader.hpp>
// #include <bigx/writer.hpp>
```

---

## Option 2: FetchContent (No Git Submodule)

```cmake
include(FetchContent)

FetchContent_Declare(
  bigx
  GIT_REPOSITORY https://github.com/ViTeXFTW/BigXtractor.git
  GIT_TAG main  # Or a specific version tag
)
FetchContent_MakeAvailable(bigx)

# Link against the library
target_link_libraries(your_target PRIVATE bigx::bigx)
```

---

## Option 3: Installed System Library

### Install BigXtractor

```bash
git clone https://github.com/ViTeXFTW/BigXtractor.git
cd BigXtractor
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local -DINSTALL_STANDALONE=ON
cmake --build build
cmake --install build
```

### Use in your project

```cmake
find_package(bigx REQUIRED)
target_link_libraries(your_target PRIVATE bigx::bigx)
```

---

## Option 4: vcpkg

### Using the Public Registry (Future)

Once BigXtractor is published to the official vcpkg registry:

```bash
vcpkg install bigx
```

### Using an Overlay Port

To use BigXtractor as an overlay port before it's in the registry:

```bash
# Clone BigXtractor
git clone https://github.com/ViTeXFTW/BigXtractor.git

# Build and install via overlay port
vcpkg install bigx --overlay-ports=BigXtractor/ports
```

### Using with CMake Toolchain

```bash
# Configure with vcpkg toolchain
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

Or use the provided CMake presets:

```bash
# Debug build with vcpkg
cmake --preset vcpkg-debug
cmake --build --preset vcpkg-debug

# Debug with tests
cmake --preset vcpkg-test
cmake --build --preset vcpkg-test
ctest --preset vcpkg-test
```

---

## Recommended Directory Structure

```
your-project/
├── external/
│   └── BigXtractor/       # ← Git submodule
├── src/
│   └── main.cpp
└── CMakeLists.txt
```

---

## CMake Options

When using BigXtractor as a submodule, you can control its behavior:

```cmake
# Before add_subdirectory:
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(INSTALL_STANDALONE OFF CACHE BOOL "" FORCE)

add_subdirectory(external/BigXtractor)
```

Or via command line:
```bash
cmake -B build -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF
```

---

## Usage Example

```cpp
#include <bigx/bigx.hpp>

int main() {
    // Open an archive
    auto archive = bigx::Archive::open("archive.big");
    if (!archive) {
        std::cerr << "Failed to open archive\n";
        return 1;
    }

    // List all files
    for (const auto& file : archive->files()) {
        std::cout << file.path << " (" << file.size << " bytes)\n";
    }

    // Find and extract a file (case-insensitive)
    const auto* file = archive->findFile("Data/Texture.tga");
    if (file) {
        archive->extract(*file, "output.tga");
    }

    return 0;
}
```

---

## Notes

- The library exports the `bigx::bigx` target for proper dependency propagation
- Include directories are automatically configured
- No manual include path configuration needed
- Compiler flags from the parent project are not affected (uses `PRIVATE` where appropriate)
- `compile_commands.json` symlink is only created when bigx is the top-level project
