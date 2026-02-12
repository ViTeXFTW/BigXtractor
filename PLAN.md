# Plan: Add vcpkg Support to BigXtractor

## Current State Analysis

BigXtractor is a C++20 library with **zero external runtime dependencies** (standard library only). The sole optional dependency is Google Test for testing, managed via a git submodule. The build system is CMake 3.20+.

Current integration methods: git submodule, FetchContent, system install via `find_package()`. No package manager support exists.

---

## What's Missing

vcpkg support has two facets: **(A)** making BigXtractor installable via `vcpkg install bigx` (consumer-side), and **(B)** using vcpkg to manage BigXtractor's own dev dependencies like GTest (developer-side).

### Gap 1: No vcpkg manifest (`vcpkg.json`) in project root

The project root needs a `vcpkg.json` manifest file that declares:
- Package name (`bigx`), version (`1.0.0`), description, license
- Dependencies: none for core library; `gtest` as a test-only `"host"` dependency via a `"test"` feature
- Homepage URL

**File to create:** `/vcpkg.json`

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "name": "bigx",
  "version": "1.0.0",
  "description": "Modern C++20 library for reading/writing BIG archive files (Command & Conquer Generals)",
  "homepage": "https://github.com/ViTeXFTW/BigXtractor",
  "license": "MIT",
  "supports": "!uwp",
  "dependencies": [],
  "features": {
    "tests": {
      "description": "Build tests",
      "dependencies": [
        {
          "name": "gtest",
          "host": true
        }
      ]
    }
  }
}
```

### Gap 2: No vcpkg baseline configuration (`vcpkg-configuration.json`)

Needed to pin the vcpkg registry baseline so builds are reproducible.

**File to create:** `/vcpkg-configuration.json`

```json
{
  "default-registry": {
    "kind": "git",
    "repository": "https://github.com/microsoft/vcpkg",
    "baseline": "<latest-commit-sha>"
  }
}
```

### Gap 3: No vcpkg port files for registry submission

To publish BigXtractor to the vcpkg public registry (or use as an overlay port), two files are needed:

**Files to create (for overlay or registry PR):**

#### `ports/bigx/vcpkg.json`
Port manifest with name, version, description, dependencies, and license.

#### `ports/bigx/portfile.cmake`
Build instructions that:
1. Download source from GitHub (via `vcpkg_from_github`)
2. Configure with CMake, passing `-DINSTALL_STANDALONE=ON -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF`
3. Install via `vcpkg_cmake_install`
4. Fix up the cmake config files via `vcpkg_cmake_config_fixup`
5. Remove debug include directories
6. Install license file

```cmake
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO ViTeXFTW/BigXtractor
    REF "v${VERSION}"
    SHA512 <hash>
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DINSTALL_STANDALONE=ON
        -DBUILD_TESTING=OFF
        -DBUILD_EXAMPLES=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME bigx CONFIG_PATH lib/cmake/bigx)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
```

#### `ports/bigx/usage`
```
bigx provides CMake targets:

    find_package(bigx CONFIG REQUIRED)
    target_link_libraries(main PRIVATE bigx::bigx)
```

### Gap 4: CMake install rules gated behind `INSTALL_STANDALONE` option

The current `CMakeLists.txt` wraps all install rules inside `if(INSTALL_STANDALONE)`. vcpkg will need to pass `-DINSTALL_STANDALONE=ON` in the portfile (which works), but a cleaner pattern is to make install rules unconditional, or at least default to ON when the project is the top-level build. This is a design choice:

**Option A (minimal change):** Keep as-is; the portfile passes `-DINSTALL_STANDALONE=ON`. No code changes needed.

**Option B (cleaner):** Replace `INSTALL_STANDALONE` gating with a check that defaults to ON when bigx is the top-level project and OFF when included via `add_subdirectory()`:

```cmake
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  option(INSTALL_STANDALONE "Install as standalone library" ON)
else()
  option(INSTALL_STANDALONE "Install as standalone library" OFF)
endif()
```

**Recommendation:** Option A for now (no CMakeLists.txt change needed for vcpkg to work). Option B can be a follow-up improvement.

### Gap 5: CMakePresets.json lacks vcpkg-aware presets

Developers who want to use vcpkg for GTest (instead of the git submodule) need presets that set `CMAKE_TOOLCHAIN_FILE` to the vcpkg toolchain.

**File to modify:** `/CMakePresets.json`

Add new presets:
```json
{
  "name": "vcpkg",
  "hidden": true,
  "cacheVariables": {
    "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
  }
},
{
  "name": "vcpkg-debug",
  "displayName": "Debug (vcpkg)",
  "inherits": ["base", "vcpkg"],
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Debug"
  }
},
{
  "name": "vcpkg-test",
  "displayName": "Debug with Tests (vcpkg)",
  "inherits": ["vcpkg-debug"],
  "cacheVariables": {
    "BUILD_TESTING": "ON"
  }
}
```

### Gap 6: Documentation lacks vcpkg instructions

Neither `README.md` nor `INTEGRATION.md` mention vcpkg as an integration option.

**Files to modify:**

- `README.md` — Add an "Option 4: vcpkg" section under "Usage in Other Projects"
- `INTEGRATION.md` — Add an "Option 4: vcpkg" section with instructions for both overlay ports and (future) registry install

### Gap 7: No CI workflow testing vcpkg builds

Currently there are only Claude Code-related CI workflows. There's no GitHub Actions workflow that verifies the library builds correctly under vcpkg.

**File to create:** `.github/workflows/vcpkg-build.yml` (optional, lower priority)

---

## Implementation Steps (Ordered)

### Step 1: Create `vcpkg.json` manifest in project root
- Declare package metadata (name, version, description, homepage, license)
- Declare empty dependencies for core, gtest as a test feature dependency
- This enables `vcpkg install` from the project directory

### Step 2: Create `vcpkg-configuration.json`
- Pin the vcpkg baseline to a recent commit for reproducible builds
- Point to the default Microsoft vcpkg registry

### Step 3: Create overlay port files under `ports/bigx/`
- `ports/bigx/vcpkg.json` — port manifest
- `ports/bigx/portfile.cmake` — build/install instructions using `vcpkg_cmake_configure`, `vcpkg_cmake_install`, `vcpkg_cmake_config_fixup`
- `ports/bigx/usage` — consumer usage instructions

### Step 4: Update `CMakePresets.json`
- Add a hidden `vcpkg` base preset that sets `CMAKE_TOOLCHAIN_FILE`
- Add `vcpkg-debug`, `vcpkg-release`, and `vcpkg-test` presets inheriting from it
- Existing presets remain unchanged (no breaking changes)

### Step 5: Update documentation
- `README.md` — Add vcpkg as integration Option 4
- `INTEGRATION.md` — Add detailed vcpkg integration section (overlay port usage, future registry install)
- `AGENTS.md` — Mention vcpkg as an available dependency management option

### Step 6 (Optional): Add CI workflow for vcpkg
- GitHub Actions workflow that installs vcpkg, runs `vcpkg install` from the manifest, builds with the vcpkg toolchain, and runs tests
- Lower priority; can be done as a follow-up

---

## What Does NOT Need Changing

- **`CMakeLists.txt` core build logic** — The existing `find_package(GTest)` already works with vcpkg-provided packages. The install rules work when `INSTALL_STANDALONE=ON` is passed.
- **`cmake/bigx-config.cmake.in`** — Already correct; no dependencies to find.
- **`tests/CMakeLists.txt`** — Already links `gtest` and `gtest_main` which vcpkg provides correctly.
- **Source code** — No changes needed to any `.cpp` or `.hpp` files.
- **Git submodule** — Keep the googletest submodule as a fallback; vcpkg is an alternative, not a replacement.

---

## Summary of Deliverables

| File | Action | Priority |
|------|--------|----------|
| `vcpkg.json` | Create | High |
| `vcpkg-configuration.json` | Create | High |
| `ports/bigx/vcpkg.json` | Create | High |
| `ports/bigx/portfile.cmake` | Create | High |
| `ports/bigx/usage` | Create | High |
| `CMakePresets.json` | Modify (add vcpkg presets) | Medium |
| `README.md` | Modify (add vcpkg section) | Medium |
| `INTEGRATION.md` | Modify (add vcpkg section) | Medium |
| `AGENTS.md` | Modify (mention vcpkg) | Low |
| `.github/workflows/vcpkg-build.yml` | Create | Low (follow-up) |
