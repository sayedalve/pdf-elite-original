# Architecture Decision Records (ADRs)

> **FACT:** These decisions are based on analysis of the current Java/Tauri/React architecture and the requirements for a native C++ PDF editor.
> Each ADR follows the format: **Status**, **Context**, **Options Considered**, **Decision**, **Consequences**.

---

## ADR-001: C++20 with MSVC (not Clang, not MinGW)

- **Status:** ✅ Decided
- **Context:** The application targets Windows 10/11 only. We need a modern C++ standard with excellent Windows API support and debugging tools.

### Options Considered

| Option | Pros | Cons |
|--------|------|------|
| **MSVC (Visual Studio 2022)** | Best Win32 integration, best debugger, C++20 complete, /MT static CRT, AppVerifier support | Windows-only (acceptable) | 
| Clang-cl (LLVM) | Faster compiles, stricter diagnostics | Plugin compatibility issues, some MSVC extensions missing |
| MinGW-w64 | GCC compatibility, open source | Inferior debugging, incomplete C++20, ABI issues with Windows SDK |

### Decision

**Use MSVC (Visual Studio 2022) as the sole compiler.**

> **RECOMMENDATION:** Configure CMake to use the "MSVC" generator. Set `/std:c++20 /W4 /WX /MT`.

### Consequences
- ✅ Best possible debugging experience with Visual Studio
- ✅ Full C++20 feature support (concepts, ranges, coroutines, modules)
- ✅ Static CRT linking (`/MT`) eliminates VC++ redistributable dependency
- ✅ Application Verifier, ASan, TSan all work natively
- ⚠️ Windows-only — no cross-compilation (acceptable per requirements)
- ⚠️ Slower compile times than Clang (mitigated by incremental builds)

---

## ADR-002: PDFium as Sole PDF Engine (not PDFBox, not MuPDF)

- **Status:** ✅ Decided
- **Context:** The current app uses PDFBox (Java) for manipulation and JPDFium (opaque bridge) for rendering. We need a single, native, high-performance PDF engine.

### Options Considered

| Option | License | Pros | Cons |
|--------|---------|------|------|
| **PDFium** | BSD 3-clause | Best rendering quality, Google-maintained, Chrome's engine, C API | Limited documentation, no text editing API |
| MuPDF | AGPL | Fast, small, good text handling | AGPL is problematic for commercial use, smaller community |
| PDFBox (via JNI) | Apache 2.0 | Well-documented, mature | Java dependency defeats the purpose of native rewrite |
| Poppler | GPL | Mature, well-tested | GPL requires source distribution |
| libharu | zlib | PDF creation only | Cannot read/edit existing PDFs |

### Decision

**Use PDFium as the sole PDF engine.**

> **FACT:** PDFium is already used in the current application (via JPDFium and pdfjs-dist). Continuing with PDFium reduces learning curve.
> **FACT:** PDFium's BSD 3-clause license is the most permissive of all options.

### Consequences
- ✅ Chrome-quality rendering out of the box
- ✅ BSD 3-clause license — no copyleft concerns
- ✅ Static linking produces single executable
- ✅ Already familiar from JPDFium usage in current app
- ⚠️ Text editing requires working at the text-object level (PDFium has no high-level text editing API)
- ⚠️ Limited official documentation — must rely on headers, Chromium source, and community
- ⚠️ Tied to Google's release cadence (though we can pin a version)

---

## ADR-003: Win32 API for UI (not Qt, not wxWidgets, not WebView)

- **Status:** ✅ Decided
- **Context:** The current app uses React/Mantine in a WebView2 shell, which introduces 200MB+ of overhead. We need a lightweight, native UI.

### Options Considered

| Option | Binary Size | Dev Speed | Native Feel | Notes |
|--------|------------|-----------|-------------|-------|
| **Win32 API (raw)** | +0 MB | Slow | Perfect | Maximum control, minimal size |
| Qt 6 | +40 MB | Fast | Good | Large dependency, licensing (LGPL/commercial) |
| wxWidgets | +8 MB | Medium | Good | Mature, but less modern |
| WebView2 (keep current) | +150 MB | Fast | Poor | Defeats purpose of native rewrite |
| Dear ImGui | +1 MB | Fast | Poor (gaming-style) | Not suitable for document editors |

### Decision

**Use raw Win32 API for all UI.**

> **RECOMMENDATION:** Create a thin UI toolkit layer (per UI_COMPONENTS.md) to avoid raw Win32 calls everywhere.

### Consequences
- ✅ Zero UI framework dependency — smallest possible binary
- ✅ Perfect native Windows look and feel
- ✅ Full control over rendering, theming, and accessibility
- ✅ No runtime dependency beyond Windows itself
- ⚠️ Slower development than framework-based approaches
- ⚠️ Must manually implement common controls (property grid, ribbon, etc.)
- ⚠️ Theming (dark mode) requires custom drawing for every control
- ⚠️ Accessibility requires manual MSAA/UIA implementation

---

## ADR-004: No Runtime Dependencies Beyond PDFium (not .NET, not JVM)

- **Status:** ✅ Decided
- **Context:** The current app requires Java 17+, Node.js, Rust toolchain, and WebView2. The goal is a single portable executable.

### Options Considered

| Option | Startup Time | Binary Size | Complexity |
|--------|-------------|------------|------------|
| **Single static exe (no runtime deps)** | < 500ms | ~30 MB | High (build complexity) |
| .NET 8 AOT | ~1s | ~30 MB | Medium |
| Python embedded | ~2s | ~50 MB | Low |
| Keep current (JVM+Node+Rust) | ~5s | ~300 MB | Low (already exists) |

### Decision

**Zero runtime dependencies. Single static executable.**

> **FACT:** Static linking PDFium + MSVC CRT (`/MT`) + vcpkg static libs produces a single .exe.
> **RECOMMENDATION:** Ship only the .exe + optional LICENSES.txt and NOTICE file.

### Consequences
- ✅ Fastest possible startup time (< 500ms)
- ✅ Smallest possible installer (~40MB)
- ✅ No version conflicts, no missing runtimes
- ✅ Works on any Windows 10/11 machine without prerequisites
- ✅ USB-drive portable (no registry required for basic operation)
- ⚠️ Must build all dependencies statically
- ⚠️ Cannot use any library that requires dynamic loading
- ⚠️ Updates require full binary replacement (no hot-loading)

---

## ADR-005: RAII Ownership Model (not GC, not manual refcount beyond shared_ptr)

- **Status:** ✅ Decided
- **Context:** C++ does not have garbage collection. We need a clear, consistent ownership model to prevent leaks and use-after-free.

### Options Considered

| Option | Pros | Cons |
|--------|------|------|
| **RAII + smart pointers** | Deterministic, no GC pauses, C++ idiomatic | Requires careful design |
| Reference counting (custom) | Familiar to COM developers | Cyclic references, not type-safe |
| Garbage collection (Boehm GC) | Easy memory management | Non-deterministic, pauses, unusual for C++ |
| Arena allocation | Fast, cache-friendly | Lifetime tied to scope, inflexible |

### Decision

**RAII with `std::unique_ptr` for exclusive ownership, `std::shared_ptr` for shared ownership, and custom deleters for PDFium handles.**

### Consequences
- ✅ Deterministic resource cleanup — no GC pauses
- ✅ Compiler-enforced ownership semantics
- ✅ Easy to reason about object lifetimes
- ✅ `unique_ptr` is zero-overhead
- ⚠️ Must define custom deleters for all PDFium handle types
- ⚠️ `shared_ptr` has atomic refcount overhead (use sparingly)
- ⚠️ Must be careful with circular references (use `weak_ptr`)

### Implementation Pattern

```cpp
// PDFium handle wrapper with custom deleter
struct FpdfPageDeleter {
    void operator()(FPDF_PAGE page) const {
        if (page) FPDF_ClosePage(page);
    }
};
using FpdfPagePtr = std::unique_ptr<FPDF_PAGE__, FpdfPageDeleter>;

// Usage
FpdfPagePtr page(FPDF_LoadPage(doc.get(), 0));
// Automatically closed when `page` goes out of scope
```

---

## ADR-006: Tile-Based Rendering (not full-page rendering)

- **Status:** ✅ Decided
- **Context:** PDF pages at high zoom levels (e.g., 400%) produce bitmaps of 5000×7000+ pixels. Rendering full pages on every scroll is too slow.

### Options Considered

| Option | Memory Usage | Scroll Performance | Complexity |
|--------|-------------|-------------------|------------|
| **Tile-based (256×256 or 512×512)** | Bounded (LRU cache) | 60fps | High |
| Full-page rendering | Unbounded | < 10fps at high zoom | Low |
| Strip-based (horizontal bands) | Medium | 30fps | Medium |

### Decision

**Tile-based rendering with 256×256 pixel tiles and an LRU memory cache.**

> **FACT:** Adobe Acrobat, Foxit, and SumatraPDF all use tile-based rendering.
> **RECOMMENDATION:** Start with 256×256 tiles. Profile and adjust if needed.

### Consequences
- ✅ Constant memory usage regardless of document size or zoom level
- ✅ Smooth 60fps scrolling at any zoom level
- ✅ Only visible tiles rendered — no wasted GPU/CPU
- ✅ Cache hit rate is high for typical reading patterns
- ⚠️ Tile boundary rendering may cause subpixel misalignment (mitigate with 1px overlap)
- ⚠️ Cache eviction strategy must be tuned for different document types
- ⚠️ Tile coordination adds complexity to rendering pipeline

---

## ADR-007: JSON Settings File (not Registry, not INI)

- **Status:** ✅ Decided
- **Context:** Application settings (preferences, recent files, shortcuts) need persistent storage.

### Options Considered

| Option | Pros | Cons |
|--------|------|------|
| **JSON file** | Human-readable, supports nesting, portable | Slightly slower than binary |
| Windows Registry | Native, fast, system-integrated | Not portable, permission issues, fragmentation |
| INI file | Simple, legacy support | No nesting, no types, encoding issues |
| TOML file | Clean syntax, typed | Less familiar to Windows devs |
| SQLite | Structured, queryable | Overkill for settings | 

### Decision

**JSON settings file at `%APPDATA%/PDFElite/settings.json` using nlohmann/json.**

> **RECOMMENDATION:** Support portable mode where settings.json lives next to the executable.

### Consequences
- ✅ Portable — works from USB drives
- ✅ Human-readable and editable
- ✅ Supports nested structures (per-document settings, theme configs)
- ✅ No registry permission issues
- ✅ Easy to backup and restore
- ⚠️ Must handle concurrent writes (use file lock or write-to-temp-then-rename)
- ⚠️ Must handle corrupt JSON gracefully (fallback to defaults)
- ⚠️ Not as fast as registry (negligible for settings access pattern)

---

## ADR-008: std::expected for Error Handling (not exceptions across boundaries)

- **Status:** ✅ Decided
- **Context:** C++ exceptions across DLL/PDFium boundaries are dangerous. We need a safe, explicit error handling strategy.

### Options Considered

| Option | Pros | Cons |
|--------|------|------|
| **`std::expected` (C++23) or custom `Result<T>`** | Explicit, no unwinding across boundaries, composable | Not in C++20 standard (polyfill needed) |
| C++ exceptions everywhere | Idiomatic C++, clean code | Undefined behavior across C boundaries (PDFium) |
| Error codes (HRESULT-style) | COM-compatible, no overhead | Easy to ignore, no context |
| `std::optional<T>` + separate error | Simple | Loses error information |

### Decision

**Use a custom `Result<T, Error>` type (similar to `std::expected`) for all APIs that can fail. Use exceptions only internally where safe.**

> **RECOMMENDATION:** Implement a `Result<T>` class that wraps either a value `T` or an `Error` object. Use it at all API boundaries.

### Consequences
- ✅ Explicit error handling — compiler forces you to check
- ✅ Safe across C boundaries (PDFium)
- ✅ Composable with `?`-like operator (via custom monadic operations)
- ✅ Error information is preserved and typed
- ⚠️ Slightly more verbose than exceptions
- ⚠️ Must be consistent — mixing Result and exceptions causes confusion
- ⚠️ Need C++23 polyfill for `std::expected` until MSVC supports it natively

### Implementation Pattern

```cpp
// Result type
template<typename T>
class Result {
    std::variant<T, Error> value_;
public:
    bool ok() const { return std::holds_alternative<T>(value_); }
    T& value() & { return std::get<T>(value_); }
    Error& error() & { return std::get<Error>(value_); }
};

// Usage
Result<FPDF_DOCUMENT> OpenDocument(const std::wstring& path) {
    auto doc = FPDF_LoadDocument(path.c_str(), nullptr);
    if (!doc) {
        return Error{ErrorCode::FileNotFound, L"Failed to load PDF"};
    }
    return FpdfDocPtr(doc);
}
```

---

## ADR-009: PDFium Static Linking (not DLL)

- **Status:** ✅ Decided
- **Context:** PDFium can be linked as a static library or loaded as a DLL.

### Options Considered

| Option | Binary Size | Deployment | Update Flexibility |
|--------|------------|------------|-------------------|
| **Static linking** | +15 MB in exe | Single file | Must recompile |
| Dynamic linking (DLL) | exe + 15 MB DLL | Two files | Can swap DLL |

### Decision

**Static link PDFium into the main executable.**

> **FACT:** PDFium's BSD 3-clause license explicitly permits static linking.
> **RECOMMENDATION:** Build PDFium from vcpkg with static linking configuration.

### Consequences
- ✅ Single-file deployment
- ✅ No DLL version mismatch issues
- ✅ Slightly faster startup (no DLL load)
- ⚠️ Larger executable (~15MB increase)
- ⚠️ Cannot update PDFium independently of the application
- ⚠️ Slightly longer build times

---

## ADR-010: No Plugin Architecture (keep it simple)

- **Status:** ✅ Decided
- **Context:** Some PDF editors support plugins (e.g., Acrobat has a rich plugin ecosystem). This adds significant complexity.

### Options Considered

| Option | Extensibility | Complexity | Security |
|--------|--------------|------------|----------|
| **No plugins** | None | Minimal | Maximum |
| Internal plugin API | High | High | Medium |
| COM/IDL plugins | Very high | Very high | Low (arbitrary code) |
| Lua/Python scripting | High | High | Low |

### Decision

**No plugin architecture. All features are compiled into the executable.**

> **RECOMMENDATION:** If extensibility is needed in the future, consider a command-line interface for automation rather than in-process plugins.

### Consequences
- ✅ Simpler architecture — no ABI stability concerns
- ✅ No security surface from third-party code
- ✅ Faster builds, easier debugging
- ✅ No versioning issues
- ⚠️ Community cannot extend the application
- ⚠️ OCR and other optional features must be compiled in or not at all
- ⚠️ May limit enterprise adoption (no custom plugins)
