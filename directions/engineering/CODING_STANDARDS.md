# Coding Standards — PDF Elite Native (C++/Win32/PDFium)

> **Status:** Proposed | **Applies to:** Native C++ rebuild | **Last updated:** 2025-01

---

## Table of Contents

1. [C++20 Rules](#c20-rules)
2. [Naming Conventions](#naming-conventions)
3. [File Organization](#file-organization)
4. [Include Order](#include-order)
5. [Win32 API Usage](#win32-api-usage)
6. [Error Handling](#error-handling)
7. [Threading Rules](#threading-rules)
8. [PDFium Encapsulation](#pdfium-encapsulation)
9. [Memory Management](#memory-management)
10. [Comments & Documentation](#comments--documentation)
11. [Formatting](#formatting)
12. [Code Review Checklist](#code-review-checklist)

---

## C++20 Rules

### Required Modern C++ Features

| Feature | Usage | Example |
|---------|-------|---------|
| `std::unique_ptr` | Sole ownership | `auto doc = std::make_unique<Document>(path);` |
| `std::shared_ptr` | Shared ownership (rendered tiles) | `auto bitmap = std::make_shared<Gdiplus::Bitmap>(...);` |
| `std::string_view` | Non-owning string references | `void SetTitle(std::string_view title)` |
| `std::span` | Non-owning array references | `void ProcessPages(std::span<PageIndex> pages)` |
| `std::optional` | May-be-absent values | `std::optional<Annotation> FindAt(Point pos)` |
| `std::variant` | Type-safe unions | `using Value = std::variant<bool, int, double, std::string>;` |
| `constexpr` | Compile-time constants | `constexpr int kMaxRecentFiles = 20;` |
| `std::format` | String formatting (C++23) or `fmt` | `std::format("Page {} of {}", current, total)` |
| `enum class` | Strongly typed enums | `enum class PageRotation { None, Clockwise90, ... };` |
| `std::filesystem` | Path manipulation | `std::filesystem::path p(L"C:\\temp\\file.pdf");` |
| `std::chrono` | Time/duration handling | `auto timeout = std::chrono::seconds(30);` |
| `concepts` (C++20) | Template constraints | `template<std::integral T> void Set(T value)` |
| `ranges` (C++20) | Algorithm pipelines | `std::ranges::find_if(pages, is_visible);` |
| `[[nodiscard]]` | Prevent ignoring return values | `[[nodiscard]] Result<Document> Open(...)` |
| `[[nodiscard("reason")]]` | Explain why return must not be ignored | `[[nodiscard("may be nullptr")]] Page* GetPage(int i)` |

### Prohibited Practices

| Practice | Reason | Alternative |
|----------|--------|-------------|
| `new` / `delete` | Manual memory management | `std::make_unique`, `std::make_shared` |
| Raw `T*` ownership | No clear ownership semantics | Smart pointers |
| C-style arrays | No bounds checking | `std::vector`, `std::array`, `std::span` |
| `malloc` / `free` | Not type-safe, no constructors | C++ allocation operators |
| C-style casts | Unsafe, bypasses type system | `static_cast`, `reinterpret_cast` (rarely) |
| `#define` for constants | No type safety, no scoping | `constexpr`, `inline constexpr` |
| `NULL` | C legacy, not type-safe | `nullptr` |
| `using namespace std;` | Name collision risk | Explicit `std::` qualification |
| Multiple inheritance | Complexity, diamond problem | Composition, interfaces with `std::variant` |
| Exceptions across module boundaries | ABI instability, caller language mismatch | `Result<T>` / `std::expected` |
| `volatile` | Almost always wrong in application code | `std::atomic` for concurrency |
| `goto` | Unstructured control flow | `break`, `continue`, early returns |
| `auto` for non-obvious types | Reduces readability | Explicit type when intent unclear |

### RAII Is Mandatory

Every resource must be wrapped in an RAII type:

```cpp
// GOOD: RAII wrapper ensures cleanup
HandleGuard file(CreateFileW(path, GENERIC_READ, 0, nullptr,
    OPEN_EXISTING, 0, nullptr));
if (!file.IsValid()) {
    return Result<FileData>::Error(ErrorCode::FileNotFound);
}
// file closes automatically when HandleGuard goes out of scope

// BAD: Raw HANDLE, must remember to close
HANDLE hFile = CreateFileW(path, ...);
if (hFile == INVALID_HANDLE_VALUE) return error;
// ... must not forget CloseHandle(hFile) on every exit path
```

> **RECOMMENDATION:** Use the `wil` (Windows Implementation Library) header-only helpers from Microsoft when available: `wil::unique_hfile`, `wil::unique_handle`, `wil::com_ptr`. They provide production-tested RAII wrappers for all common Win32 resource types.

---

## Naming Conventions

| Category | Style | Example |
|----------|-------|---------|
| **Classes / Structs** | PascalCase | `Document`, `TileCache`, `RenderWorker`, `RenderSettings` |
| **Interfaces (abstract)** | PascalCase (no prefix) | `Renderer`, `Search` (not `IRenderer`) |
| **Member functions** | camelCase | `openFile()`, `renderPage()`, `getCurrentPage()` |
| **Free functions** | camelCase | `utf16ToUtf8()`, `clampZoom()` |
| **Local variables** | camelCase | `pageIndex`, `zoomLevel`, `filePath` |
| **Member variables** | `m_` prefix + camelCase | `m_document`, `m_currentPage`, `m_zoomLevel` |
| **Static member variables** | `s_` prefix + camelCase | `s_instance`, `s_maxCacheSize` |
| **Constants** | `k` prefix + PascalCase | `kDefaultZoom`, `kMaxRecentFiles`, `kAppVersion` |
| **Enums** | PascalCase (enum class) | `enum class PageRotation { None, Clockwise90, ... };` |
| **Enum values** | PascalCase | `PageRotation::Clockwise90` |
| **Namespaces** | lowercase | `pdf_engine`, `rendering`, `ui`, `detail` |
| **Macros** | UPPER_SNAKE_CASE | `PDFELITE_VERSION_MAJOR`, `NOMINMAX` |
| **Template parameters** | Single uppercase or PascalCase | `template<typename T>`, `template<typename Allocator>` |
| **Type aliases** | PascalCase | `using PageIndex = int;`, `using RenderCallback = std::function<void()>;` |
| **Win32 callbacks** | Prefix with module | `PdfEliteWndProc`, `PdfEliteDialogProc` |
| **CMake targets** | lowercase with underscores | `pdf_engine`, `rendering_lib`, `pdf_elite_exe` |

### File Naming

| Type | Convention | Example |
|------|-----------|---------|
| Header files | PascalCase.h | `Document.h`, `TileCache.h` |
| Source files | PascalCase.cpp | `Document.cpp`, `TileCache.cpp` |
| CMake files | CMakeLists.txt, PascalCase.cmake | `CMakeLists.txt`, `PdfiumSetup.cmake` |
| Test files | PascalCase + Tests.cpp | `PdfEngineTests.cpp`, `TileCacheTests.cpp` |
| Resource files | lowercase | `app.rc`, `app.manifest`, `strings.en-US.json` |

---

## File Organization

### Header/Source Split

Every module except `core/` follows the header/source split:

```
module/
├── include/module/        # Public headers (consumed by other modules)
│   └── ClassName.h
├── src/                   # Implementation files
│   └── ClassName.cpp
└── CMakeLists.txt
```

### Single Class Per File

| Rule | Detail |
|------|--------|
| One class per .h/.cpp pair | `Document.h` + `Document.cpp` = only `class Document` |
| Exceptions allowed | Small tightly-coupled types (e.g., `Rect.h` with `Rect<T>`, `Point<T>`, `Size<T>`) |
| Internal types | Place in `detail` namespace or `internal/` subdirectory |
| Forward declarations | Use `class Document;` in headers when possible to reduce includes |

### Header Guard

```cpp
#pragma once
```

> **RECOMMENDATION:** Use `#pragma once` instead of traditional include guards. It is supported by all modern compilers (MSVC, GCC, Clang), is less error-prone, and is the de facto standard for Windows C++ development.

### Class Declaration Order

```cpp
// Document.h
#pragma once

#include "core/Types.h"          // Project types first

#include <memory>                 // Then standard library
#include <string>
#include <vector>

#include <windows.h>              // Then system headers (when needed in public API)

namespace pdf_engine {

// Forward declarations
class Page;

// Public types first
class Document {
public:
    // Types & constants
    using PageRange = std::pair<PageIndex, PageIndex>;
    static constexpr int kInvalidPage = -1;
    
    // Constructors / destructors
    explicit Document(std::wstring_view path);
    ~Document();
    
    // Non-copyable, movable
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;
    Document(Document&&) noexcept;
    Document& operator=(Document&&) noexcept;

    // Public interface
    [[nodiscard]] int pageCount() const;
    [[nodiscard]] std::shared_ptr<Page> getPage(PageIndex index);
    [[nodiscard]] Metadata metadata() const;
    
    Result<void> save(std::wstring_view path);
    void close();

private:
    // Private data members (always at bottom)
    struct Impl;
    std::unique_ptr<Impl> m_impl;  // Pimpl idiom for PDFium types
};

}  // namespace pdf_engine
```

---

## Include Order

Strict ordering in every `.cpp` file:

```cpp
// 1. Own header (must be first)
#include "module/ClassName.h"

// 2. Project headers (alphabetical within groups)
#include "core/Types.h"
#include "core/Result.h"
#include "infra/Logger.h"

// 3. Third-party headers (alphabetical)
#include <fmt/format.h>
#include <wil/resource.h>

// 4. System / platform headers
#include <windows.h>
#include <d2d1.h>
#include <shlwapi.h>

// 5. C standard library (last)
#include <cmath>
#include <cstring>
```

| Priority | Category | Rationale |
|----------|----------|-----------|
| 1 | Own header | Verifies header is self-contained (has all its own includes) |
| 2 | Project headers | Knows about project types before using them |
| 3 | Third-party | Well-defined, minimal dependencies |
| 4 | System headers | Largest, most polluting (defines macros, types) |
| 5 | C stdlib | Rarely needed in C++20, include last if needed |

### Include What You Use

Every `.cpp` file must include every header it directly depends on. Never rely on transitive includes.

```cpp
// BAD: relies on Document.h transitively including Page.h
#include "pdf_engine/Document.h"
void UsePage(std::shared_ptr<Page> p);  // Error: Page is not defined here

// GOOD: includes what it uses
#include "pdf_engine/Document.h"
#include "pdf_engine/Page.h"            // Added
void UsePage(std::shared_ptr<Page> p);  // OK: Page is fully defined
```

---

## Win32 API Usage

### RAII Wrappers Are Mandatory

| Win32 Resource | RAII Wrapper | Cleanup Action |
|---------------|--------------|---------------|
| `HANDLE` (generic) | `HandleGuard` / `wil::unique_handle` | `CloseHandle()` |
| `HANDLE` (file) | `wil::unique_hfile` | `CloseHandle()` |
| `HWND` | Managed by Window class | `DestroyWindow()` |
| `HBITMAP` | `wil::unique_hbitmap` | `DeleteObject()` |
| `HDC` | `wil::unique_hdc` | `ReleaseDC()` or `DeleteDC()` |
| `HFONT` | `wil::unique_hfont` | `DeleteObject()` |
| `HBRUSH` | `wil::unique_hbrush` | `DeleteObject()` |
| `HICON` | `wil::unique_hicon` | `DestroyIcon()` |
| `HMENU` | `wil::unique_hmenu` | `DestroyMenu()` |
| `HGLOBAL` | `wil::unique_hglobal` | `GlobalFree()` |
| `COM interface*` | `wil::com_ptr<T>` | `Release()` |
| `BSTR` | `wil::unique_bstr` | `SysFreeString()` |
| `CRITICAL_SECTION` | `wil::unique_critical_section` | `DeleteCriticalSection()` |

### Never Use Naked Win32 Resource Types

```cpp
// BAD: Naked HANDLE, must close manually on every path
HANDLE hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
if (hEvent == nullptr) return Error;
// ... many lines of code ...
CloseHandle(hEvent);  // Easy to miss on error paths

// GOOD: RAII wrapper, automatic cleanup
wil::unique_handle hEvent{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
if (!hEvent) return Error;
// ... code ... hEvent closes automatically
```

### Unicode Is Mandatory

| Rule | Detail |
|------|--------|
| `UNICODE` / `_UNICODE` defined | All Win32 calls use W variants |
| `std::wstring` for Win32 paths | No `std::string` for file paths on Windows |
| `L""` literals | Or use string conversion utilities |
| `CreateFileW`, `MessageBoxW`, etc. | Never use A variants |
| `wWinMain` entry point | Not `WinMain` or `main` |

### Window Message Handling

```cpp
// GOOD: Early return pattern for WndProc
LRESULT CALLBACK PdfEliteWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE:
            return OnCreate(hwnd, reinterpret_cast<CREATESTRUCT*>(lp));
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            OnResize(hwnd, LOWORD(lp), HIWORD(lp));
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wp, lp);
    }
}
```

| Rule | Detail |
|------|--------|
| Use WndProc dispatch pattern | Switch on message, delegate to handler functions |
| Never block in WndProc | All heavy work deferred to thread pool |
| Return 0 for handled messages | Let DefWindowProcW handle unhandled messages |
| Validate HWND before use | Never assume HWND is valid in callbacks |

---

## Error Handling

### Result Type

All fallible operations return `Result<T>`, never throw exceptions across module boundaries.

```cpp
// core/Result.h
template<typename T>
class Result {
public:
    static Result Ok(T value);
    static Result Error(ErrorCode code, std::string message = "");
    
    [[nodiscard]] bool isOk() const;
    [[nodiscard]] bool isError() const;
    [[nodiscard]] explicit operator bool() const;
    
    [[nodiscard]] T& value();
    [[nodiscard]] const T& value() const;
    [[nodiscard]] T& operator*();
    [[nodiscard]] const T& operator*() const;
    [[nodiscard]] T* operator->();
    
    [[nodiscard]] ErrorCode error() const;
    [[nodiscard]] const std::string& errorMessage() const;

private:
    std::variant<T, ErrorInfo> m_data;
};

// Specialization for void
[[noreturn]] Result<void> Result<void>::Error(ErrorCode code, std::string message);
```

### Usage Pattern

```cpp
// GOOD: Propagate errors
Result<Document> PdfEngine::openFile(const std::wstring& path) {
    if (!std::filesystem::exists(path)) {
        return Result<Document>::Error(ErrorCode::FileNotFound,
            fmt::format("File not found: {}", utf16ToUtf8(path)));
    }
    
    auto doc = tryLoadPdfium(path);
    if (!doc) {
        return Result<Document>::Error(ErrorCode::InvalidPdf,
            "Failed to parse PDF file");
    }
    
    return Result<Document>::Ok(Document(std::move(doc)));
}

// GOOD: Caller checks result
auto result = engine.openFile(L"test.pdf");
if (!result) {
    logger.error("Failed to open: {}", result.errorMessage());
    showErrorDialog(result.errorMessage());
    return;
}
auto& document = *result;
```

### Error Rules

| Rule | Detail |
|------|--------|
| Never ignore `Result<T>` | Code review must catch this |
| Log errors at the point of handling, not at the point of occurrence | Let callers decide log level |
| Use specific `ErrorCode` values | Not just `ErrorCode::Unknown` |
| Preserve error chain | Include `errorMessage()` from inner calls |
| Exceptions allowed ONLY within a module | May use internally, must catch at module boundary |
| Win32 errors use `GetLastError()` + `FormatMessageW` | Convert to `ErrorCode` + message string |

### Error Code Categories

| Category | Examples |
|----------|---------|
| File I/O | `FileNotFound`, `PermissionDenied`, `FileLocked`
| PDF parsing | `InvalidPdf`, `CorruptPdf`, `UnsupportedFeature`
| PDF security | `PasswordRequired`, `PasswordIncorrect`, `PermissionDenied`
| Rendering | `RenderFailed`, `OutOfMemory`, `InvalidPage`
| System | `OutOfMemory`, `NotInitialized`, `AlreadyInitialized` |

---

## Threading Rules

### Core Principle

> **Never block the UI thread.** The Win32 message loop must remain responsive at all times.

### Thread Responsibilities

| Thread | Role | Allowed Operations |
|--------|------|-------------------|
| **UI thread** (main) | Message pump, painting, user input | Win32 calls, lightweight state reads, PostMessage/SendMessage to own windows |
| **Render workers** (pool) | PDF page rendering | PDFium rendering, bitmap creation (all PDFium calls) |
| **File I/O thread** (pool) | File open, save, search | File system operations, PDFium document loading |
| **Update thread** (single) | Auto-update checks | WinHTTP requests, file downloads |

### Rules

| Rule | Detail |
|------|--------|
| No PDFium calls on UI thread | All `FPDF_*` calls happen on render/I/O threads |
| No file I/O on UI thread | Use thread pool for file open/save |
| No blocking waits on UI thread | No `WaitForSingleObject`, no `std::future::get()` without timeout |
| Cross-thread communication via `PostMessage` | Send results to UI via window messages, not direct calls |
| Shared data protected by `SRWLOCK` | Prefer reader-writer locks over `std::mutex` for read-heavy data |
| Use `std::atomic` for simple flags | Page dirty state, cancel flags, shutdown flags |
| No `std::async` with `wait()` | Use thread pool with callback pattern |

### Thread-Safe Pattern

```cpp
// GOOD: Post result to UI thread from worker
void RenderWorker::onRenderComplete(RenderResult result, HWND targetHwnd) {
    // Allocate on heap, ownership transfers to message handler
    auto* data = new RenderResult(std::move(result));
    PostMessageW(targetHwnd, WM_RENDER_COMPLETE, 0, reinterpret_cast<LPARAM>(data));
}

// In WndProc:
case WM_RENDER_COMPLETE: {
    std::unique_ptr<RenderResult> result(reinterpret_cast<RenderResult*>(lp));
    onRenderComplete(*result);  // Update UI safely on UI thread
    return 0;
}
```

---

## PDFium Encapsulation

### The Golden Rule

> **Never expose `FPDF_*` types outside the `pdf_engine` module.**

The entire PDFium C API is an implementation detail of the `pdf_engine` module. External modules interact only through the public C++ classes: `Document`, `Page`, `Annotation`, etc.

### Enforcement

| Rule | Detail |
|------|--------|
| No `#include <fpdfview.h>` outside `pdf_engine/` | Public headers use only project types |
| No `FPDF_DOCUMENT`, `FPDF_PAGE`, etc. in public headers | Replaced by `Document*`, `Page*` |
| No `FPDF_*` function calls outside `pdf_engine/src/` | All calls wrapped in `Pdfium*.cpp` files |
| Pimpl idiom for Document/Page | Private impl struct holds FPDF handles |
| `detail` namespace for internal types | `PdfiumDocumentHandle` lives in `detail::` |

### Example: Pimpl for PDFium Isolation

```cpp
// include/pdf_engine/Document.h — PUBLIC (no PDFium types)
namespace pdf_engine {
class Document {
public:
    explicit Document(std::wstring_view path);
    ~Document();
    [[nodiscard]] int pageCount() const;
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
}

// src/PdfiumDocument.cpp — PRIVATE (has PDFium types)
#include <fpdfview.h>
#include <fpdfdoc.h>

namespace pdf_engine {
struct Document::Impl {
    FPDF_DOCUMENT document = nullptr;
    std::wstring path;
    Metadata metadata;
};

Document::Document(std::wstring_view path) : m_impl(std::make_unique<Impl>()) {
    m_impl->path = path;
    m_impl->document = FPDF_LoadDocument(...);
}

int Document::pageCount() const {
    return FPDF_GetPageCount(m_impl->document);
}
}
```

> **FACT:** The current app accesses PDFium through JPDFium 1.0.2, a private Maven artifact that wraps PDFium via JNI. This adds a Java→JNI→C++ call chain per operation. The native rebuild eliminates both the Java layer and the JNI bridge, calling PDFium's C API directly.

---

## Memory Management

### Ownership Model

| Pattern | When to Use | Example |
|---------|-------------|--------|
| `std::unique_ptr<T>` | Exclusive ownership, most common | `std::unique_ptr<Document> m_doc;` |
| `std::shared_ptr<T>` | Shared ownership (rendered bitmaps) | `std::shared_ptr<Gdiplus::Bitmap> m_bitmap;` |
| `std::weak_ptr<T>` | Observe shared without extending lifetime | Cache observer pattern |
| Stack allocation | Small, scope-bound objects | `RectD bounds;`, `RenderRequest request;` |
| Raw `T*` (non-owning) | Observers, parameters, C API interop | Must document non-owning |

### Specific Rules

| Rule | Detail |
|------|--------|
| `std::make_unique` / `std::make_shared` | Never call `new` directly |
| Pimpl for large private state | Reduces compile-time coupling |
| `std::vector` over raw arrays | Always, no exceptions |
| `std::string` / `std::wstring` | Never `char*` / `wchar_t*` for owned strings |
| Reserve vector capacity | When size is known in advance |
| `emplace_back` over `push_back` | Avoids temporary construction |
| Move semantics for large objects | `std::move()` when transferring ownership |
| No cyclic `shared_ptr` | Use `weak_ptr` to break cycles |

### Bitmap Memory

Rendered PDF bitmaps are the largest memory consumer:

| Aspect | Strategy |
|--------|----------|
| Storage | `std::shared_ptr<Gdiplus::Bitmap>` in `TileCache` |
| Eviction | LRU cache with max memory budget (~256MB default) |
| Sharing | Multiple viewer controls can share the same tile bitmap |
| Background | Downsampled thumbnails (~150 DPI) for sidebar |
| Foreground | Full-resolution tiles at current zoom level |

---

## Comments & Documentation

### Principle: Explain WHY, Not WHAT

```cpp
// BAD: Stating the obvious
// Set the zoom level to 1.5
m_zoomLevel = 1.5;

// GOOD: Explaining the reasoning
// PDFium renders best at integer scale factors; 150% is the closest
// to user's requested 160% that avoids sub-pixel blurring.
m_zoomLevel = 1.5;
```

### When to Comment

| Type | When | Example |
|------|------|--------|
| **Why** | Non-obvious design decisions | `// Using SRWLOCK instead of mutex: 95% reads, 5% writes on page cache` |
| **Workaround** | Bug workarounds, platform quirks | `// Windows 10 1903 bug: DefWindowProcW crashes on WM_NCCREATE with nullptr` |
| **Performance** | Why a specific approach was chosen | `// Pre-allocate 1024 entries: profiling shows 98% of docs have < 1000 pages` |
| **Safety** | Thread safety invariants | `// Must be called under m_renderLock; releases lock during wait` |
| **TODO** | Known limitations | `// TODO: Handle XFA forms (requires libxml2 dependency)` |

### When NOT to Comment

| Anti-pattern | Why |
|-------------|------|
| `// Increment counter` | The code says this |
| `// Get page count` | `pageCount()` says this |
| `// Constructor` | Above a constructor |
| Commented-out code | Delete it. Git remembers. |
| Change log in comments | Use git log. |

### File Header Comment

```cpp
// pdf_engine/src/PdfiumDocument.cpp
// PDF Elite — Native Windows PDF Viewer & Editor
// Copyright 2025 PDF Elite Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Implementation of Document class using PDFium C API.
// All FPDF_* calls are isolated to this translation unit.
```

---

## Formatting

### clang-format Configuration

```yaml
# .clang-format
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
AllowShortFunctionsOnASingleLine: Inline
AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
BreakBeforeBraces: Allman
NamespaceIndentation: None
SortIncludes: CaseInsensitive
IncludeBlocks: Regroup
PointerAlignment: Left
ReferenceAlignment: Left
SpacesBeforeTrailingComments: 2
AlignAfterOpenBracket: Align
BinPackArguments: false
BinPackParameters: false
AllowAllParametersOfDeclarationOnNextLine: true
```

### Key Style Rules

| Aspect | Rule | Rationale |
|--------|------|-----------|
| **Indent** | 4 spaces | Windows convention, readable |
| **Braces** | Allman (next line) | Windows/MSVC convention |
| **Line length** | 100 characters | Reasonable for wide screens |
| **Pointer alignment** | Left (`int* p`) | Clear type vs. variable distinction |
| **Trailing return** | Not used | `auto foo() -> int` is unusual in Win32 C++ |
| **Tabs** | Never, spaces only | Consistency across editors |
| **Trailing whitespace** | Never | `.editorconfig` enforced |
| **File ending** | Single newline | POSIX standard |

### Example

```cpp
namespace rendering {

Result<std::shared_ptr<RenderedTile>> Renderer::renderTile(
    const RenderRequest& request)
{
    if (!m_document) {
        return Result<...>::Error(ErrorCode::NotInitialized);
    }

    auto page = m_document->getPage(request.pageIndex);
    if (!page) {
        return Result<...>::Error(ErrorCode::InvalidPage);
    }

    const auto dpi = request.zoom * kBaseDpi;
    auto bitmap = page->render(dpi, request.colorMode);
    if (!bitmap) {
        return Result<...>::Error(ErrorCode::RenderFailed);
    }

    auto tile = std::make_shared<RenderedTile>(
        std::move(bitmap), request.bounds);
    m_cache->insert(request.key(), tile);

    return Result<...>::Ok(tile);
}

}  // namespace rendering
```

### Editor Integration

> **FACT:** The current project uses `.editorconfig` for cross-editor consistency. This should be carried forward to the C++ project.

```ini
# .editorconfig (additions for C++)
[*.h]
indent_style = space
indent_size = 4

[*.cpp]
indent_style = space
indent_size = 4

[*.rc]
indent_style = space
indent_size = 4

[CMakeLists.txt]
indent_style = space
indent_size = 2
```

---

## Code Review Checklist

Every pull request must pass these checks:

### Correctness

- [ ] Code compiles with `/W4 /WX` (zero warnings)
- [ ] All tests pass (`ctest --output-on-failure`)
- [ ] `clang-tidy` passes with zero warnings
- [ ] New code is covered by unit tests
- [ ] Edge cases handled: empty input, null pointers, out-of-range indices
- [ ] Error paths tested (not just happy path)

### Safety

- [ ] No raw `new` / `delete` — all ownership via smart pointers
- [ ] No naked Win32 HANDLE / HWND / HBITMAP — all in RAII wrappers
- [ ] No `FPDF_*` types or functions outside `pdf_engine/` module
- [ ] No blocking calls on UI thread (file I/O, waits, sleeps)
- [ ] Shared data protected by locks or atomics
- [ ] No exceptions thrown across module boundaries

### Style

- [ ] `clang-format` passes (run `git diff --check` for whitespace)
- [ ] Include order: own → project → third-party → system → C stdlib
- [ ] Header is self-contained (includes its own dependencies)
- [ ] Naming follows conventions (PascalCase classes, camelCase functions, m_ members)
- [ ] `[[nodiscard]]` on all fallible return types
- [ ] No `using namespace std;` anywhere
- [ ] No commented-out code

### Documentation

- [ ] Public API has clear intent (function/parameter names are self-documenting)
- [ ] Non-obvious design decisions are commented (WHY, not WHAT)
- [ ] TODO comments have issue tracker references
- [ ] No stale comments that contradict the code

### Performance

- [ ] No unnecessary copies (use `const&` or `std::move`)
- [ ] No repeated allocation in loops (pre-allocate or move outside)
- [ ] No O(n²) algorithms on large collections without justification
- [ ] Vector `.reserve()` used when final size is known

### Security

- [ ] No buffer overflows (use `std::vector`, `std::string`, bounds-checked access)
- [ ] No integer overflow in size calculations
- [ ] No unvalidated user input passed to Win32 APIs
- [ ] File paths validated before use
- [ ] No hardcoded secrets or credentials

---

## Summary

| Category | Key Rule |
|----------|----------|
| Language | C++20, MSVC, `/W4 /WX /utf-8 /permissive-` |
| Memory | RAII mandatory, smart pointers only, no raw `new`/`delete` |
| Naming | PascalCase classes, camelCase functions, `m_` members, `k` constants |
| Error | `Result<T>` for all fallible operations, never ignore errors |
| Threading | Never block UI thread, PostMessage for cross-thread communication |
| PDFium | Encapsulated in `pdf_engine` module, Pimpl idiom, no `FPDF_*` leaks |
| Win32 | RAII wrappers for all HANDLE/HWND/HBITMAP, Unicode only |
| Comments | Explain WHY, not WHAT; delete commented-out code |
| Formatting | clang-format (LLVM-based, Allman braces, 4-space indent) |
| Review | Checklist above must pass before merge |