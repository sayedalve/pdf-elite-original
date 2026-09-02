# Build System — PDF Elite Native (C++/Win32/PDFium)

> **Status:** Proposed | **Applies to:** Native C++ rebuild | **Last updated:** 2025-01

---

## Table of Contents

1. [Overview](#overview)
2. [Toolchain Requirements](#toolchain-requirements)
3. [CMake Project Structure](#cmake-project-structure)
4. [Root CMakeLists.txt](#root-cmakelistsxt)
5. [Compiler Flags](#compiler-flags)
6. [Linker Flags](#linker-flags)
7. [Build Types](#build-types)
8. [PDFium Integration](#pdfium-integration)
9. [CMake Presets](#cmake-presets)
10. [Dependency Management](#dependency-management)
11. [Build Directory Structure](#build-directory-structure)
12. [Test Integration](#test-integration)
13. [Comparison with Current Build](#comparison-with-current-build)

---

## Overview

| Aspect | Current (Tauri) | Proposed (Native) |
|--------|----------------|-------------------|
| **Build system** | Gradle 8.x + npm + cargo | CMake 3.28+ |
| **Language** | Java 25, TypeScript, Rust | C++20, Win32 API |
| **Compiler** | javac, tsc, rustc | MSVC 19.4x (VS 2022) |
| **Target** | multi-platform (deb/rpm/dmg/nsis) | Windows 10/11 x64 only |
| **Build steps** | 6+ (Gradle → JAR → Vite → Tauri → installer) | 1 (CMake configure + build) |
| **Incremental** | Partial (Gradle + Vite HMR) | Full (Ninja/MSBuild) |
| **Parallelism** | Gradle workers + Vite + Cargo | CMake/MSBuild + Ninja `/maxcpucount` |

> **FACT:** The current build runs `npm ci && npx vite --mode desktop` (beforeDev) and `npx vite build --mode desktop` (beforeBuild) as Tauri hooks, adding Node.js as a build dependency. The native rebuild eliminates all three language runtimes.

---

## Toolchain Requirements

| Tool | Minimum Version | Purpose |
|------|----------------|---------|
| **Visual Studio 2022** | 17.8+ | MSVC compiler, Windows SDK, debugger |
| **CMake** | 3.28+ | Build system generator |
| **Ninja** | 1.11+ (optional) | Faster builds than MSBuild |
| **Git** | 2.40+ | FetchContent, pdfium-binaries download |
| **Python 3** | 3.11+ (optional) | CMake helper scripts, golden PDF generation |
| **vcpkg** | Latest (optional) | Third-party library management |
| **LLVM/Clang** | 18+ (optional) | clang-format, clang-tidy |

> **RECOMMENDATION:** Use Ninja generator for development (2-5x faster than MSBuild), MSBuild for release builds to match VS IDE integration.

---

## CMake Project Structure

```
pdf-elite/
├── CMakeLists.txt                  # Root: project(), options, add_subdirectory
├── cmake/
│   ├── PdfiumSetup.cmake           # PDFium discovery/fetch logic
│   ├── CompilerFlags.cmake         # Shared compiler/linker flags
│   └── CTestCustom.cmake           # Test configuration
├── src/
│   └── main.cpp                    # WinMain / wWinMain entry point
├── app/
│   ├── CMakeLists.txt              # app module target
│   ├── Application.h
│   └── Application.cpp
├── core/
│   ├── CMakeLists.txt              # core module (header-only)
│   └── Types.h
├── pdf_engine/
│   ├── CMakeLists.txt              # pdf_engine static lib
│   ├── include/pdf_engine/
│   └── src/
├── rendering/
│   ├── CMakeLists.txt              # rendering static lib
│   ├── include/rendering/
│   └── src/
├── editing/
│   ├── CMakeLists.txt
│   ├── include/editing/
│   └── src/
├── document_model/
│   ├── CMakeLists.txt
│   ├── include/doc_model/
│   └── src/
├── ui/
│   ├── CMakeLists.txt              # ui static lib
│   ├── include/ui/
│   └── src/
├── platform/
│   └── windows/
│       ├── CMakeLists.txt          # platform static lib
│       ├── FileDialog.h/cpp
│       ├── FileAssociation.h/cpp
│       └── Clipboard.h/cpp
├── infrastructure/
│   ├── CMakeLists.txt              # infra static lib
│   ├── include/infra/
│   └── src/
├── tests/
│   ├── CMakeLists.txt              # test executables
│   ├── unit/
│   └── integration/
└── resources/
    ├── icons/
    ├── fonts/
    └── strings/
```

### Root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.28)
project(PdfElite
    VERSION 3.0.0
    DESCRIPTION "PDF Elite — Native Windows PDF Viewer & Editor"
    LANGUAGES CXX
)

# --- Options ---
option(PDFELITE_BUILD_TESTS "Build unit and integration tests" ON)
option(PDFELITE_BUILD_INSTALLER "Build NSIS installer target" OFF)
option(PDFELITE_USE_LTO "Enable link-time optimization in Release" ON)
option(PDFELITE_PDFIUM_PATH "Path to pre-built PDFium (auto-detected if empty)" "")

# --- Standards ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Shared flags ---
include(cmake/CompilerFlags.cmake)

# --- PDFium ---
include(cmake/PdfiumSetup.cmake)

# --- Modules ---
add_subdirectory(core)
add_subdirectory(infrastructure)
add_subdirectory(pdf_engine)
add_subdirectory(rendering)
add_subdirectory(editing)
add_subdirectory(document_model)
add_subdirectory(ui)
add_subdirectory(platform/windows)
add_subdirectory(app)

if(PDFELITE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

---

## Compiler Flags

| Flag | Purpose | When |
|------|---------|------|
| `/std:c++20` | C++20 language standard | Always |
| `/W4` | High warning level | Always |
| `/WX` | Treat warnings as errors | Always |
| `/utf-8` | Source and execution charset UTF-8 | Always |
| `/permissive-` | Strict conformance mode | Always |
| `/Zc:__cplusplus` | Report correct `__cplusplus` value | Always |
| `/Zc:externC` | Strict `extern "C"` | Always |
| `/Zc:wchar_t` | `wchar_t` is built-in type | Always |
| `/EHsc` | C++ exceptions (internal only) | Always |
| `/GL` | Whole program optimization | Release only |
| `/O2` | Max speed optimization | Release only |
| `/Ob2` | Inline expansion | Release only |
| `/DNDEBUG` | Disable assertions | Release only |
| `/Od` | Disable optimizations | Debug only |
| `/Zi` | Full debug info | Debug / RelWithDebInfo |
| `/RTC1` | Run-time error checks | Debug only |
| `/JMC` | Just My Code debugging | Debug only |

### Preprocessor Definitions

| Define | Purpose |
|--------|---------|
| `NOMINMAX` | Prevent Windows.h min/max macros |
| `WIN32_LEAN_AND_MEAN` | Exclude rarely-used Windows headers |
| `VC_EXTRALEAN` | Further trim Windows headers |
| `UNICODE` / `_UNICODE` | Unicode character set (wchar_t) |
| `PDFELITE_VERSION="3.0.0"` | Version string embedding |
| `WINVER=0x0A00` | Target Windows 10 |
| `_WIN32_WINNT=0x0A00` | Target Windows 10 |
| `NTDDI_VERSION=0x0A000006` | Target Windows 10 1809+ |

---

## Linker Flags

| Flag | Purpose | When |
|------|---------|------|
| `/HIGHENTROPYVA` | High-entropy 64-bit ASLR | Release |
| `/DYNAMICBASE` | ASLR | Always |
| `/NXCOMPAT` | DEP compatible | Always |
| `/guard:cf` | Control Flow Guard | Release |
| `/LTCG` | Link-time code generation | Release (when LTO on) |
| `/OPT:REF` | Remove unreferenced functions | Release |
| `/OPT:ICF` | Enable COMDAT folding | Release |
| `/DEBUG:FULL` | Full PDB debug info | Debug / RelWithDebInfo |
| `/INCREMENTAL` | Incremental linking | Debug only |

### CRT Linkage

| Build Type | CRT Flag | Rationale |
|------------|----------|-----------|
| Release | `/MD` | Dynamic CRT (share with system DLLs) |
| Debug | `/MDd` | Dynamic debug CRT |
| Static alternative | `/MT` or `/MTd` | Only if avoiding VCRedist dependency |

> **RECOMMENDATION:** Use `/MD` (dynamic CRT) to reduce binary size. Ship VCRedist as an installer prerequisite or use the system-installed version (present on Windows 10 1809+ via UCRT).

---

## Build Types

| Type | CMake Flag | Optimizations | Debug Info | Purpose |
|------|-----------|---------------|------------|---------|
| **Release** | `--config Release` | `/O2 /GL /Ob2` | No | Production builds, installer |
| **Debug** | `--config Debug` | `/Od /RTC1` | Full PDB | Development, debugging |
| **RelWithDebInfo** | `--config RelWithDebInfo` | `/O2` | Full PDB | Profiling, crash analysis |
| **MinSizeRel** | `--config MinSizeRel` | `/O1` | No | Not recommended (marginal savings) |

---

## PDFium Integration

Two strategies, configurable via `PDFELITE_PDFIUM_PATH`:

### Strategy A: Pre-built Binary (Recommended)

```cmake
# cmake/PdfiumSetup.cmake
include(FetchContent)

if(NOT PDFELITE_PDFIUM_PATH)
    FetchContent_Declare(pdfium
        URL https://github.com/nicnl31/pdfium-binaries/releases/download/chromium%2F130.0.6723.44%2Br0-rc1/pdfium-windows-x64.tgz
        URL_HASH SHA256=<computed>
    )
    FetchContent_MakeAvailable(pdfium)
    set(PDFIUM_ROOT ${pdfium_SOURCE_DIR})
else()
    set(PDFIUM_ROOT ${PDFELITE_PDFIUM_PATH})
endif()

# Locate PDFium libraries
find_library(PDFIUM_LIB NAMES pdfium
    PATHS ${PDFIUM_ROOT}/lib
    NO_DEFAULT_PATH
)
find_path(PDFIUM_INCLUDE_DIR NAMES public/fpdfview.h
    PATHS ${PDFIUM_ROOT}/include
    NO_DEFAULT_PATH
)
```

### Strategy B: Build from Source

```cmake
include(ExternalProject)
ExternalProject_Add(pdfium_from_source
    SOURCE_DIR   ${CMAKE_SOURCE_DIR}/third_party/pdfium
    INSTALL_DIR  ${CMAKE_BINARY_DIR}/pdfium-install
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DPDFIUM_USE_SYSTEM_ICU=OFF
        -DPDFIUM_ENABLE_V8=OFF
    BUILD_BYPRODUCTS <INSTALL_DIR>/lib/pdfium.lib
)
```

| Aspect | Pre-built | From Source |
|--------|-----------|-------------|
| **Build time** | ~5 seconds (download) | ~30-90 minutes |
| **Control** | Fixed configuration | Full GN args control |
| **Size** | ~30MB (static lib) | ~25-40MB (configurable) |
| **Risk** | Version availability | Build environment complexity |

> **FACT:** The current app uses JPDFium 1.0.2 (a private Maven artifact), which wraps PDFium for JVM. The native build eliminates this indirection.

> **RECOMMENDATION:** Use Strategy A (pre-built) for all builds. Only switch to Strategy B if specific PDFium patches or custom GN flags are needed.

---

## CMake Presets

```jsonc
// CMakePresets.json
{
  "version": 6,
  "cmakeMinimumRequired": { "major": 3, "minor": 28, "patch": 0 },
  "configurePresets": [
    {
      "name": "vs2022-default",
      "displayName": "Visual Studio 2022 (x64)",
      "generator": "Visual Studio 17 2022",
      "architecture": { "value": "x64", "strategy": "set" },
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
      }
    },
    {
      "name": "ninja-debug",
      "displayName": "Ninja (Debug)",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
      }
    },
    {
      "name": "ninja-release",
      "displayName": "Ninja (Release)",
      "inherits": "ninja-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "PDFELITE_USE_LTO": "ON"
      }
    },
    {
      "name": "ci-release",
      "displayName": "CI Release",
      "inherits": "ninja-release",
      "cacheVariables": {
        "PDFELITE_BUILD_TESTS": "ON",
        "PDFELITE_BUILD_INSTALLER": "ON"
      }
    }
  ],
  "buildPresets": [
    { "name": "debug", "configurePreset": "ninja-debug" },
    { "name": "release", "configurePreset": "ninja-release" },
    { "name": "ci", "configurePreset": "ci-release" }
  ],
  "testPresets": [
    {
      "name": "default",
      "configurePreset": "ninja-debug",
      "output": { "outputOnFailure": true }
    }
  ]
}
```

---

## Dependency Management

| Dependency | Source | Size Impact | Notes |
|------------|--------|-------------|-------|
| **PDFium** | FetchContent / vcpkg | ~30MB static lib | Core PDF engine |
| **Windows SDK** | System-installed | 0MB (external) | Win32, GDI+, Direct2D, Shell |
| **MSVC CRT** | System-installed | 0MB (external) | `/MD` linkage |
| **wil** (optional) | FetchContent | ~1MB headers | Win32 helper library |
| **fmt** (optional) | vcpkg | ~200KB | Text formatting (or use std::format) |
| **gtest** | FetchContent | test-only | Unit testing framework |
| **libxml2** (optional) | vcpkg | ~2MB | XFA form support |

> **FACT:** The current Gradle build pulls 50+ Java dependencies, 150+ NPM packages, and 30+ Rust crates. The native build reduces this to PDFium + Windows SDK + a handful of optional vcpkg packages.

### vcpkg Configuration

```cmake
# Only needed for optional heavy dependencies
find_package(unofficial-libxml2 CONFIG QUIET)
if(unofficial-libxml2_FOUND)
    target_link_libraries(pdf_engine PRIVATE unofficial::libxml2::libxml2)
    target_compile_definitions(pdf_engine PRIVATE PDFELITE_HAS_LIBXML2)
endif()
```

---

## Build Directory Structure

```
build/
├── ninja-debug/
│   ├── build.ninja
│   ├── compile_commands.json
│   ├── CMakeCache.txt
│   ├── pdf-elite.exe              # Debug executable
│   ├── pdf-elite.pdb              # Debug symbols
│   ├── pdf_engine/                 # Module build dirs
│   ├── rendering/
│   ├── ui/
│   └── _deps/
│       └── pdfium/                 # Fetched PDFium
├── ninja-release/
│   ├── pdf-elite.exe              # Release executable
│   └── pdf-elite.pdb              # RelWithDebInfo symbols
└── vs2022-default/
    ├── PdfElite.sln               # VS solution
    └── pdf-elite.vcxproj          # VS project
```

---

## Test Integration

```cmake
# tests/CMakeLists.txt
include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Unit tests
add_executable(pdf_engine_tests
    unit/PdfiumDocumentTests.cpp
    unit/PdfiumPageTests.cpp
    unit/TileCacheTests.cpp
)
target_link_libraries(pdf_engine_tests
    PRIVATE pdf_engine infrastructure GTest::gtest_main
)
add_test(NAME PdfEngineTests COMMAND pdf_engine_tests)

# Integration tests
add_executable(rendering_integration_tests
    integration/RenderPipelineTests.cpp
)
target_link_libraries(rendering_integration_tests
    PRIVATE rendering pdf_engine infrastructure GTest::gtest_main
)
add_test(NAME RenderingIntegration COMMAND rendering_integration_tests)
```

```bash
# Run all tests
cmake --build build/ninja-debug --target pdf-elite_tests
cd build/ninja-debug && ctest --output-on-failure

# Run single test suite
ctest --test-filter PdfEngineTests -V
```

---

## Comparison with Current Build

| Metric | Current (Gradle+Tauri) | Proposed (CMake) |
|--------|------------------------|-------------------|
| **Full clean build** | 5-15 min (first time) | 30-90 sec (Ninja, cached PDFium) |
| **Incremental build** | 10-60 sec | 1-5 sec |
| **Build tool count** | 4 (Gradle, npm, cargo, NSIS/WiX) | 2 (CMake, compiler) |
| **Build complexity** | 40+ GitHub Actions workflows | 1 CI pipeline |
| **First-time setup** | JDK 25 + Node.js + Rust + system deps | VS 2022 + CMake + Ninja |
| **Docker build** | 3-stage, ~2GB image | Not needed (Windows-only) |
| **Task runner** | Taskfile.yml (20+ tasks) | CMake targets + Ninja |

> **FACT:** The current project uses Taskfile.yml for orchestration, .pre-commit-config.yaml for hooks, .editorconfig for formatting, and .gitleaksignore for secret scanning. The C++ rebuild should adopt .editorconfig and pre-commit hooks (clang-format, clang-tidy) but replaces Taskfile with native CMake targets.

---

## Quick Reference Commands

```bash
# Configure (first time)
cmake --preset ninja-debug

# Build
cmake --build build/ninja-debug

# Run
cmake --build build/ninja-debug && ./build/ninja-debug/pdf-elite.exe

# Test
cmake --build build/ninja-debug --target pdf-elite_tests
ctest --test-dir build/ninja-debug --output-on-failure

# Release build
cmake --preset ninja-release
cmake --build build/ninja-release --config Release

# Clean rebuild
cmake --build build/ninja-debug --clean-first

# IDE generation
cmake --preset vs2022-default
start build/vs2022-default/PdfElite.sln
```
