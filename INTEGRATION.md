# Integrating big-lib as a Dependency

## Option 1: Git Submodule (Recommended)

### 1. Add the submodule

```bash
cd /path/to/your/project
git submodule add https://github.com/yourusername/big-lib.git external/big-lib
git submodule update --init --recursive
```

### 2. Update your CMakeLists.txt

```cmake
# Add the big-lib subdirectory
add_subdirectory(external/big-lib)

# Link against the library
target_link_libraries(your_target PRIVATE big::big)
```

### 3. Include headers in your code

```cpp
#include <bigx/big.hpp>

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
  big
  GIT_REPOSITORY https://github.com/yourusername/big-lib.git
  GIT_TAG main  # Or a specific version tag
)
FetchContent_MakeAvailable(big)

# Link against the library
target_link_libraries(your_target PRIVATE big::big)
```

---

## Option 3: Installed System Library

### Install big-lib

```bash
git clone https://github.com/yourusername/big-lib.git
cd big-lib
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local -DINSTALL_STANDALONE=ON
cmake --build build
cmake --install build
```

### Use in your project

```cmake
find_package(big REQUIRED)
target_link_libraries(your_target PRIVATE big::big)
```

---

## Recommended Directory Structure

```
your-project/
├── external/
│   └── big-lib/       # ← Git submodule
├── src/
│   └── main.cpp
└── CMakeLists.txt
```

---

## CMake Options

When using big-lib as a submodule, you can control its behavior:

```cmake
# Before add_subdirectory:
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(INSTALL_STANDALONE OFF CACHE BOOL "" FORCE)

add_subdirectory(external/big-lib)
```

Or via command line:
```bash
cmake -B build -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF
```

---

## Usage Example

```cpp
#include <bigx/big.hpp>

int main() {
    // Open an archive
    auto archive = big::Archive::open("archive.big");
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

- The library exports the `big::big` target for proper dependency propagation
- Include directories are automatically configured
- No manual include path configuration needed
- Compiler flags from the parent project are not affected (uses `PRIVATE` where appropriate)
- `compile_commands.json` symlink is only created when big-lib is the top-level project
