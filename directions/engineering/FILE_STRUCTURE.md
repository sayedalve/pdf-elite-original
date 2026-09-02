# File Structure — PDF Elite Native (C++/Win32/PDFium)

> **Status:** Proposed | **Applies to:** Native C++ rebuild | **Last updated:** 2025-01

---

## Table of Contents

1. [Directory Tree](#directory-tree)
2. [Top-Level Files](#top-level-files)
3. [CMake Configuration](#cmake-configuration)
4. [Source Entry Point](#source-entry-point)
5. [Module: core](#module-core)
6. [Module: infrastructure](#module-infrastructure)
7. [Module: pdf_engine](#module-pdf_engine)
8. [Module: rendering](#module-rendering)
9. [Module: editing](#module-editing)
10. [Module: document_model](#module-document_model)
11. [Module: ui](#module-ui)
12. [Module: platform/windows](#module-platformwindows)
13. [Module: app](#module-app)
14. [Tests Directory](#tests-directory)
15. [Resources Directory](#resources-directory)
16. [Tools & Installer](#tools--installer)
17. [File Naming Conventions](#file-naming-conventions)
18. [Comparison with Current Structure](#comparison-with-current-structure)

---

## Directory Tree

```
pdf-elite/
├── CMakeLists.txt                          # Root build configuration
├── CMakePresets.json                       # Build presets (VS2022, Ninja)
├── .clang-format                           # Code formatting rules
├── .clang-tidy                             # Static analysis checks
├── .editorconfig                           # Editor settings (carried forward)
├── .gitignore                              # Git ignore rules
├── LICENSE                                 # Apache 2.0 (carried forward)
├── NOTICES.txt                             # Third-party license notices
├── README.md                               # Project readme
├── CHANGELOG.md                            # Version history
│
├── cmake/
│   ├── PdfiumSetup.cmake                   # PDFium discovery/download logic
│   ├── CompilerFlags.cmake                 # Shared compiler & linker flags
│   └── CTestCustom.cmake                   # CTest timeouts and configuration
│
├── src/
│   └── main.cpp                            # wWinMain entry point
│
├── app/
│   ├── CMakeLists.txt                      # Application module target
│   ├── Application.h                       # Application lifecycle
│   ├── Application.cpp
│   ├── CommandLine.h                       # Command-line argument parsing
│   ├── CommandLine.cpp
│   ├── CrashHandler.h                      # Crash reporting setup
│   ├── CrashHandler.cpp
│   ├── Updater.h                           # Auto-update logic
│   └── Updater.cpp
│
├── core/
│   ├── CMakeLists.txt                      # Core types (header-only library)
│   ├── Common.h                            # Shared macros, forward declarations
│   ├── Types.h                             # Fundamental type aliases
│   ├── Result.h                            # Result<T> / error handling types
│   ├── Rect.h                              # Geometry: Rect, Point, Size
│   └── Constants.h                         # Application-wide constants
│
├── pdf_engine/
│   ├── CMakeLists.txt                      # PDF engine static library
│   ├── include/pdf_engine/
│   │   ├── Document.h                      # PDF document interface
│   │   ├── Page.h                          # PDF page interface
│   │   ├── Annotation.h                    # Annotation read/write
│   │   ├── TextSelection.h                 # Text selection & extraction
│   │   ├── Search.h                        # Full-text search
│   │   ├── FormField.h                     # Form field interaction
│   │   ├── Bookmark.h                      # Outline/bookmark navigation
│   │   ├── Metadata.h                      # Document metadata
│   │   ├── Export.h                        # PDF save/export interface
│   │   └── PdfEngine.h                     # Engine factory / entry point
│   ├── src/
│   │   ├── PdfiumWrapper.h                 # PRIVATE: PDFium RAII wrappers
│   │   ├── PdfiumWrapper.cpp
│   │   ├── PdfiumDocument.cpp              # PRIVATE: FPDF_DOCUMENT wrapper
│   │   ├── PdfiumPage.cpp                  # PRIVATE: FPDF_PAGE wrapper
│   │   ├── PdfiumAnnotation.cpp            # PRIVATE: FPDF_ANNOTATION wrapper
│   │   ├── PdfiumTextSelection.cpp         # PRIVATE: FPDF_TEXTPAGE wrapper
│   │   ├── PdfiumSearch.cpp                # PRIVATE: Search implementation
│   │   ├── PdfiumFormField.cpp             # PRIVATE: FPDF_FORMFIELD wrapper
│   │   ├── PdfiumBookmark.cpp              # PRIVATE: FPDF_BOOKMARK wrapper
│   │   ├── PdfiumMetadata.cpp              # PRIVATE: Document metadata
│   │   ├── PdfiumExport.cpp                # PRIVATE: PDF save implementation
│   │   └── PdfEngine.cpp                   # PRIVATE: Engine initialization
│   └── pdfium_test_helper.h               # Test-only: PDFium lifecycle for tests
│
├── rendering/
│   ├── CMakeLists.txt                      # Rendering static library
│   ├── include/rendering/
│   │   ├── Renderer.h                      # Rendering orchestrator
│   │   ├── TileCache.h                     # Tile-based render cache
│   │   ├── RenderWorker.h                  # Background render thread pool
│   │   ├── RenderRequest.h                 # Render job description
│   │   ├── RenderResult.h                  # Rendered tile data
│   │   ├── RenderSettings.h                # DPI, color mode, etc.
│   │   └── GpuResources.h                  # Direct2D/DirectWrite resources
│   ├── src/
│   │   ├── Renderer.cpp
│   │   ├── TileCache.cpp
│   │   ├── RenderWorker.cpp
│   │   ├── RenderRequest.cpp
│   │   ├── RenderResult.cpp
│   │   └── GpuResources.cpp
│   └── include/rendering/
│       └── internal/                       # Implementation details
│           ├── D2DRenderer.h               # Direct2D rendering backend
│           └── D2DRenderer.cpp
│
├── editing/
│   ├── CMakeLists.txt                      # Editing operations static library
│   ├── include/editing/
│   │   ├── EditOperation.h                 # Base class for edits
│   │   ├── UndoManager.h                   # Undo/redo stack
│   │   ├── TextEditor.h                    # Text annotation editing
│   │   ├── DrawingTool.h                   # Freehand drawing
│   │   ├── HighlightTool.h                 # Highlight/strikeout/underline
│   │   ├── StampTool.h                     # Stamp/watermark
│   │   ├── PageManipulation.h             # Rotate, delete, reorder, insert
│   │   └── CropTool.h                      # Page cropping
│   └── src/
│       ├── EditOperation.cpp
│       ├── UndoManager.cpp
│       ├── TextEditor.cpp
│       ├── DrawingTool.cpp
│       ├── HighlightTool.cpp
│       ├── StampTool.cpp
│       ├── PageManipulation.cpp
│       └── CropTool.cpp
│
├── document_model/
│   ├── CMakeLists.txt                      # Document model static library
│   ├── include/doc_model/
│   │   ├── DocumentModel.h                 # In-memory document representation
│   │   ├── PageModel.h                     # Page state and metadata
│   │   ├── AnnotationModel.h               # Annotation data model
│   │   ├── SelectionModel.h                # Text selection state
│   │   ├── SearchModel.h                   # Search results state
│   │   └── RecentFiles.h                   # Recent files list
│   └── src/
│       ├── DocumentModel.cpp
│       ├── PageModel.cpp
│       ├── AnnotationModel.cpp
│       ├── SelectionModel.cpp
│       ├── SearchModel.cpp
│       └── RecentFiles.cpp
│
├── ui/
│   ├── CMakeLists.txt                      # UI static library
│   ├── include/ui/
│   │   ├── Window.h                        # Main window (Win32)
│   │   ├── TabBar.h                        # Tab control for multiple documents
│   │   ├── Toolbar.h                       # Main toolbar
│   │   ├── Sidebar.h                       # Page thumbnails / bookmarks panel
│   │   ├── ViewerControl.h                 # PDF viewport control
│   │   ├── StatusBar.h                     # Bottom status bar
│   │   ├── FindBar.h                       # In-document search bar
│   │   ├── Dialogs.h                       # All dialog declarations
│   │   ├── Theme.h                         # Visual theme management
│   │   ├── Accessibility.h                 # UI Automation support
│   │   └── ClipboardIntegration.h          # Copy/paste coordination
│   ├── src/
│   │   ├── Window.cpp
│   │   ├── WindowProc.cpp                  # Main WndProc handler
│   │   ├── TabBar.cpp
│   │   ├── Toolbar.cpp
│   │   ├── Sidebar.cpp
│   │   ├── ViewerControl.cpp
│   │   ├── ViewerScroll.cpp                # Scroll/zoom handling
│   │   ├── StatusBar.cpp
│   │   ├── FindBar.cpp
│   │   ├── Dialogs.cpp
│   │   ├── Dialogs/
│   │   │   ├── AboutDialog.cpp
│   │   │   ├── PreferencesDialog.cpp
│   │   │   ├── GoToPageDialog.cpp
│   │   │   └── PrintDialog.cpp
│   │   ├── Theme.cpp
│   │   ├── Accessibility.cpp
│   │   └── ClipboardIntegration.cpp
│   ├── resources/
│   │   ├── app.rc                          # Windows resource file
│   │   ├── app.manifest                    # Application manifest (UAC, DPI)
│   │   ├── strings.en-US.rc               # English string table
│   │   ├── accelators.rc                   # Keyboard accelerators
│   │   ├── toolbar.bmp                     # Toolbar bitmap (legacy)
│   │   └── icons/
│   │       ├── app.ico                     # Application icon
│   │       └── toolbar-icons/              # Individual toolbar icons
│   └── include/ui/
│       └── internal/
│           ├── Win32Helpers.h              # Window class registration, etc.
│           └── Win32Helpers.cpp
│
├── platform/
│   └── windows/
│       ├── CMakeLists.txt                  # Windows platform static library
│       ├── FileDialog.h                    # Open/Save file dialogs
│       ├── FileDialog.cpp
│       ├── FileAssociation.h               # .pdf file type registration
│       ├── FileAssociation.cpp
│       ├── Clipboard.h                     # System clipboard operations
│       ├── Clipboard.cpp
│       ├── ShellIntegration.h              # Jump lists, recent files
│       ├── ShellIntegration.cpp
│       ├── PowerManagement.h               # Sleep/wake handling
│       ├── PowerManagement.cpp
│       ├── HighDpi.h                       # DPI awareness setup
│       ├── HighDpi.cpp
│       ├── SingleInstance.h                # Single-instance mutex
│       └── SingleInstance.cpp
│
├── infrastructure/
│   ├── CMakeLists.txt                      # Infrastructure static library
│   ├── include/infra/
│   │   ├── Logger.h                        # Structured logging
│   │   ├── Settings.h                      # Persistent settings (registry/JSON)
│   │   ├── Error.h                         # Error codes and categories
│   │   ├── ThreadPool.h                    # General-purpose thread pool
│   │   ├── Timer.h                         # High-resolution timer
│   │   ├── StringConvert.h                 # UTF-8 ↔ UTF-16 conversion
│   │   ├── ScopeGuard.h                    # RAII scope exit
│   │   ├── ComPtr.h                        # COM smart pointer (or use wil::com_ptr)
│   │   └── HandleGuard.h                   # Win32 HANDLE RAII wrapper
│   └── src/
│       ├── Logger.cpp
│       ├── Settings.cpp
│       ├── Error.cpp
│       ├── ThreadPool.cpp
│       ├── Timer.cpp
│       ├── StringConvert.cpp
│       ├── ScopeGuard.cpp
│       ├── HandleGuard.cpp
│       └── ComPtr.cpp
│
├── resources/
│   ├── icons/
│   │   ├── app.ico                         # 256x256, 48x48, 32x32, 16x16
│   │   ├── pdf-file.ico                    # File association icon
│   │   └── app.svg                         # Source vector (build tool generates .ico)
│   ├── fonts/
│   │   ├── Inter-Regular.ttf               # UI font (optional, fallback to Segoe UI)
│   │   └── Inter-Bold.ttf                  # UI bold font
│   ├── strings/
│   │   ├── en-US.json                      # Localized strings
│   │   └── strings.schema.json             # String key schema
│   └── theme/
│       ├── light.json                      # Light theme colors
│       └── dark.json                       # Dark theme colors
│
├── tests/
│   ├── CMakeLists.txt                      # Test configuration
│   ├── unit/
│   │   ├── CoreTests.cpp                   # Types.h, Rect.h, Result.h
│   │   ├── PdfEngineTests.cpp              # Document open, page count, metadata
│   │   ├── PdfiumDocumentTests.cpp         # Internal PDFium wrapper tests
│   │   ├── PdfiumPageTests.cpp             # Page rendering, text extraction
│   │   ├── TileCacheTests.cpp              # Cache eviction, capacity
│   │   ├── UndoManagerTests.cpp            # Undo/redo operations
│   │   ├── DocumentModelTests.cpp          # Model state management
│   │   ├── SettingsTests.cpp               # Settings read/write
│   │   └── ThreadPoolTests.cpp             # Thread pool lifecycle
│   ├── integration/
│   │   ├── RenderPipelineTests.cpp         # Full render-from-file pipeline
│   │   ├── FileOpenTests.cpp               # Open various PDF files
│   │   ├── AnnotationRoundTrip.cpp         # Add, save, reload annotations
│   │   ├── FormFillRoundTrip.cpp           # Fill and save form fields
│   │   ├── SearchIntegrationTests.cpp      # Search across multi-page docs
│   │   └── SettingsPersistenceTests.cpp    # Settings survive app restart
│   ├── golden_pdfs/
│   │   ├── README.md                       # Golden file documentation
│   │   ├── simple.pdf                      # 1-page text PDF
│   │   ├── multi-page.pdf                  # 10-page mixed content
│   │   ├── forms.pdf                       # Interactive form fields
│   │   ├── annotations.pdf                 # Pre-annotated document
│   │   ├── embedded-fonts.pdf              # Various embedded fonts
│   │   ├── large.pdf                       # 100+ pages (performance testing)
│   │   ├── scanned.pdf                     # Image-only (OCR test input)
│   │   ├── password-protected.pdf          # Encrypted PDF
│   │   └── malformed/                      # Edge cases
│   │       ├── empty.pdf
│   │       ├── corrupted-header.pdf
│   │       └── zero-byte.pdf
│   └── test_helpers/
│       ├── TestEnvironment.h               # PDFium init/cleanup for tests
│       ├── TempFile.h                      # Temp file creation/cleanup
│       └── AssertHelpers.h                # Custom test assertions
│
└── tools/
    ├── installer/
    │   ├── pdf-elite.nsi                   # NSIS installer script
    │   ├── pdf-elite.wxs                   # WiX XML (alternative)
    │   └── sign.bat                        # Code signing script
    ├── scripts/
    │   ├── generate_icons.py               # SVG → ICO converter
    │   ├── generate_version.py             # Version file from git tag
    │   └── update_pdfium.py                # PDFium version update helper
    └── contrib/
        └── pdfium_patches/                 # Patches for building PDFium from source
            └── README.md
```

---

## Top-Level Files

| File | Purpose | Notes |
|------|---------|-------|
| `CMakeLists.txt` | Root build configuration | Project definition, options, module inclusion |
| `CMakePresets.json` | Build presets | VS2022, Ninja debug/release, CI |
| `.clang-format` | Formatting rules | LLVM or Microsoft base style |
| `.clang-tidy` | Static analysis config | Enabled checks, suppressions |
| `.editorconfig` | Editor settings | **Carried forward** from current project |
| `.gitignore` | Git ignore | Build dirs, IDE files, PDBs |
| `LICENSE` | Apache 2.0 | **Carried forward** from current project |
| `NOTICES.txt` | Third-party notices | PDFium BSD-3-Clause notice |
| `README.md` | Project overview | Build instructions, features |
| `CHANGELOG.md` | Version history | Semver changelog entries |

---

## CMake Configuration

| File | Purpose |
|------|---------|
| `cmake/PdfiumSetup.cmake` | FetchContent or find_package for PDFium; sets `PDFIUM_LIB`, `PDFIUM_INCLUDE_DIR` |
| `cmake/CompilerFlags.cmake` | Applies `/W4 /WX /utf-8 /permissive-` and all linker flags per build type |
| `cmake/CTestCustom.cmake` | Test timeouts (60s default, 300s for integration), memory check config |

---

## Source Entry Point

| File | Purpose |
|------|---------|
| `src/main.cpp` | `int wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)` — initializes logging, parses command line, creates `Application`, enters message loop |

---

## Module: core

> **Purpose:** Header-only library of fundamental types shared across all modules. No external dependencies. No implementation files.

| File | Purpose |
|------|---------|
| `Common.h` | `#pragma once`, forward declarations, `NOMINMAX`, platform detection macros |
| `Types.h` | `using PageIndex = int;`, `using ZoomLevel = double;`, enum classes for `PageRotation`, `RenderMode` |
| `Result.h` | `template<typename T> class Result` — value-or-error type inspired by `std::expected` (C++23 preview) |
| `Rect.h` | `template<typename T> struct Rect { T x, y, width, height; }`, `using RectD = Rect<double>;`, `using RectI = Rect<int>;` |
| `Constants.h` | `constexpr double kDefaultZoom = 1.0;`, `constexpr int kMaxRecentFiles = 20;`, version string |

> **RECOMMENDATION:** Keep `core/` strictly header-only with zero dependencies (not even Windows.h). This allows it to be included from test files and any module without circular dependency risk.

---

## Module: infrastructure

> **Purpose:** Cross-cutting concerns — logging, settings, error handling, threading, Win32 RAII wrappers. No PDF-specific logic.

| File | Purpose |
|------|---------|
| `Logger.h/cpp` | `enum class Level { Trace, Debug, Info, Warn, Error };` Output to debug console + optional file |
| `Settings.h/cpp` | Read/write to `HKCU\Software\PDF Elite` or `settings.json` in portable mode |
| `Error.h/cpp` | `enum class ErrorCode { FileNotFound, InvalidPdf, PasswordRequired, ... };` with human-readable messages |
| `ThreadPool.h/cpp` | `class ThreadPool` — fixed-size pool using Windows Thread Pool API or `std::jthread` |
| `Timer.h/cpp` | `class ScopedTimer` — RAII performance measurement for logging |
| `StringConvert.h/cpp` | `std::string Utf16ToUtf8(std::wstring_view)`, `std::wstring Utf8ToUtf16(std::string_view)` |
| `ScopeGuard.h/cpp` | `template<typename F> class ScopeGuard` — execute-on-exit |
| `ComPtr.h/cpp` | COM smart pointer or thin wrapper around `wil::com_ptr_t` |
| `HandleGuard.h/cpp` | RAII wrapper for `HANDLE` — calls `CloseHandle` in destructor |

---

## Module: pdf_engine

> **Purpose:** PDF document abstraction layer. Wraps PDFium C API behind a clean C++ interface. **Never exposes `FPDF_*` types outside this module.**

### Public API (include/pdf_engine/)

| File | Public Types |
|------|-------------|
| `Document.h` | `class Document` — open, save, close, page count, metadata, permissions |
| `Page.h` | `class Page` — dimensions, rotation, render to bitmap, text content |
| `Annotation.h` | `class Annotation` — type, bounds, content, color, add/remove/modify |
| `TextSelection.h` | `class TextSelection` — select text by rect/point, get selected text |
| `Search.h` | `class Search` — find text, highlight matches, navigate results |
| `FormField.h` | `class FormField` — type, value, options, is read-only, set value |
| `Bookmark.h` | `class Bookmark` — outline tree navigation, title, destination |
| `Metadata.h` | `struct Metadata` — title, author, subject, keywords, creation date |
| `Export.h` | `enum class ExportFormat { Pdf, ... };` — save with options |
| `PdfEngine.h` | `class PdfEngine` — factory: `Result<Document> OpenFile(const std::wstring& path)` |

### Private Implementation (src/)

| File | Purpose |
|------|---------|
| `PdfiumWrapper.h/cpp` | RAII wrappers: `PdfiumDocumentHandle`, `PdfiumPageHandle`, `PdfiumTextPageHandle` |
| `PdfiumDocument.cpp` | `Document` method implementations using FPDF_* calls |
| `PdfiumPage.cpp` | `Page` method implementations |
| `PdfiumAnnotation.cpp` | `Annotation` method implementations |
| `PdfiumTextSelection.cpp` | `TextSelection` using `FPDFText_GetText` |
| `PdfiumSearch.cpp` | `Search` using `FPDFText_FindStart`/`FindNext` |
| `PdfiumFormField.cpp` | `FormField` using `FORM_ForceToBe*` APIs |
| `PdfiumBookmark.cpp` | `Bookmark` using `FPDFBookmark_*` APIs |
| `PdfiumMetadata.cpp` | `Metadata` using `FPDF_GetMetaData` |
| `PdfiumExport.cpp` | Save using `FPDF_SaveAsCopy` / `FPDF_ExportPages` |
| `PdfEngine.cpp` | `FPDF_InitLibrary`, `FPDF_DestroyLibrary`, document creation |

> **FACT:** The current app accesses PDFium indirectly through JPDFium 1.0.2 (private Maven artifact), which adds JNI overhead. The native C++ module calls PDFium's C API directly — zero abstraction layers between application code and PDFium.

> **RECOMMENDATION:** The `PdfiumWrapper.h` file defines RAII types like `PdfiumDocumentHandle` that call `FPDF_CloseDocument` in their destructor. These types are in a `detail` namespace and never appear in public headers.

---

## Module: rendering

> **Purpose:** Tile-based PDF rendering with caching and background worker threads. Bridges `pdf_engine::Page` to Win32 GDI+/Direct2D output.

| File | Purpose |
|------|---------|
| `Renderer.h/cpp` | `class Renderer` — orchestrates rendering pipeline, manages zoom/scroll state |
| `TileCache.h/cpp` | `class TileCache` — LRU cache of rendered bitmap tiles (GDI+ `Bitmap*` or D2D bitmaps) |
| `RenderWorker.h/cpp` | `class RenderWorker` — background thread(s) that execute render requests |
| `RenderRequest.h/cpp` | `struct RenderRequest { PageIndex page; RectD viewport; ZoomLevel zoom; }` |
| `RenderResult.h/cpp` | `struct RenderResult { std::shared_ptr<Gdiplus::Bitmap> bitmap; RectD bounds; }` |
| `RenderSettings.h/cpp` | `struct RenderSettings { double dpi; bool antiAlias; ColorMode colorMode; }` |
| `GpuResources.h/cpp` | `class GpuResources` — Direct2D factory, render target (lazy initialized) |
| `internal/D2DRenderer.h/cpp` | Direct2D rendering backend (paint tiles to HWND) |

---

## Module: editing

> **Purpose:** PDF editing operations with undo/redo support. Modifies documents through `pdf_engine` public API.

| File | Purpose |
|------|---------|
| `EditOperation.h/cpp` | `class EditOperation` — abstract base for all edits, `execute()` / `undo()` |
| `UndoManager.h/cpp` | `class UndoManager` — stack-based undo/redo, configurable max depth |
| `TextEditor.h/cpp` | `class TextEditor` — free text annotation creation/editing |
| `DrawingTool.h/cpp` | `class DrawingTool` — ink/line/rectangle/ellipse freehand drawing |
| `HighlightTool.h/cpp` | `class HighlightTool` — highlight, strikeout, underline text markup |
| `StampTool.h/cpp` | `class StampTool` — image stamps, text watermarks |
| `PageManipulation.h/cpp` | Functions: rotate, delete, insert, reorder, extract pages |
| `CropTool.h/cpp` | `class CropTool` — page boundary adjustment |

---

## Module: document_model

> **Purpose:** In-memory representation of open documents, their state, and UI-adjacent data. Separates model from view.

| File | Purpose |
|------|---------|
| `DocumentModel.h/cpp` | `class DocumentModel` — holds open Document, current page, zoom, scroll position |
| `PageModel.h/cpp` | `class PageModel` — per-page state: rotation, label, cached thumbnail |
| `AnnotationModel.h/cpp` | `class AnnotationModel` — annotation list, selection state, edit mode |
| `SelectionModel.h/cpp` | `class SelectionModel` — active text selection, copy state |
| `SearchModel.h/cpp` | `class SearchModel` — search query, results list, current match index |
| `RecentFiles.h/cpp` | `class RecentFiles` — MRU list persisted to settings |

---

## Module: ui

> **Purpose:** Win32 window management, controls, and user interaction. The largest module by line count.

| File | Purpose |
|------|---------|
| `Window.h/cpp` | `class Window` — main window creation, sizing, centering, state save/restore |
| `WindowProc.cpp` | `LRESULT CALLBACK WndProc(...)` — message dispatch (delegates to controls) |
| `TabBar.h/cpp` | `class TabBar` — WC_TABCTRL wrapper with drag-reorder |
| `Toolbar.h/cpp` | `class Toolbar` — buttons: Open, Save, Print, Zoom, Search, Annotate |
| `Sidebar.h/cpp` | `class Sidebar` — page thumbnails, bookmarks, optional annotations panel |
| `ViewerControl.h/cpp` | `class ViewerControl` — scrollable PDF viewport, zoom, pan |
| `ViewerScroll.cpp` | Scroll handling: wheel, keyboard, scrollbar, touch/gesture |
| `StatusBar.h/cpp` | `class StatusBar` — page X of Y, zoom %, file size |
| `FindBar.h/cpp` | `class FindBar` — search input, next/prev, match count, highlight toggle |
| `Dialogs.h/cpp` | All modal dialogs: About, Preferences, GoToPage, Print |
| `Theme.h/cpp` | `class Theme` — light/dark mode, colors, metrics loaded from JSON |
| `Accessibility.h/cpp` | UIAutomation providers for screen readers |
| `ClipboardIntegration.h/cpp` | Copy selected text, paste images as stamps |

---

## Module: platform/windows

> **Purpose:** Windows-specific platform services. Isolated so a future `platform/linux/` or `platform/macos/` could theoretically be added.

| File | Purpose |
|------|---------|
| `FileDialog.h/cpp` | `GetOpenFileName`/`IFileOpenDialog` wrapper for .pdf filter |
| `FileAssociation.h/cpp` | Register/unregister `.pdf` → PDF Elite in registry |
| `Clipboard.h/cpp` | `OpenClipboard`/`SetClipboardData` for text and bitmap |
| `ShellIntegration.h/cpp` | Jump lists, recent files (SHAddToRecentDocs), taskbar progress |
| `PowerManagement.h/cpp` | `SetThreadExecutionState` to prevent sleep during rendering |
| `HighDpi.h/cpp` | `SetProcessDpiAwarenessContext`, DPI scaling helpers |
| `SingleInstance.h/cpp` | Named mutex + `FindWindow` to enforce single instance |

---

## Module: app

> **Purpose:** Application lifecycle — startup, shutdown, command-line handling, crash reporting, auto-updates. Glues all modules together.

| File | Purpose |
|------|---------|
| `Application.h/cpp` | `class Application` — `Initialize()`, `Run()`, `Shutdown()`, owns all subsystems |
| `CommandLine.h/cpp` | Parse args: `pdf-elite.exe [--portable] [--debug] [--version] [file.pdf]` |
| `CrashHandler.h/cpp` | `SetUnhandledExceptionFilter`, `MiniDumpWriteDump`, WER registration |
| `Updater.h/cpp` | GitHub Releases version check, download, verify, launch updater process |

---

## Tests Directory

| Subdirectory | Purpose | PDFium Required? |
|-------------|---------|-------------------|
| `unit/` | Fast tests, no file I/O | No (except PdfEngine tests) |
| `integration/` | Full pipeline tests with real PDF files | Yes |
| `golden_pdfs/` | Test fixture PDF files | N/A (data) |
| `test_helpers/` | Shared test utilities | Yes |

> **RECOMMENDATION:** Golden PDF files should be minimal (1-10 pages, < 1MB each) and checked into the repository. Large test files (> 5MB) should be generated by test setup code or downloaded via CMake `FetchContent` on first test run.

---

## Resources Directory

| Subdirectory | Contents | Format |
|-------------|----------|--------|
| `icons/` | App icon, file type icon, toolbar icons | `.ico`, `.svg` (source) |
| `fonts/` | UI fonts (optional) | `.ttf` |
| `strings/` | Localized strings | `.json` |
| `theme/` | Color schemes | `.json` |

---

## Tools & Installer

| File/Directory | Purpose |
|---------------|---------|
| `installer/pdf-elite.nsi` | NSIS installer script |
| `installer/pdf-elite.wxs` | WiX XML alternative |
| `installer/sign.bat` | Code signing batch script |
| `scripts/generate_icons.py` | Convert SVG source to multi-resolution ICO |
| `scripts/generate_version.py` | Extract version from git tag, write `version.rc.in` |
| `scripts/update_pdfium.py` | Update PDFium version in `PdfiumSetup.cmake` |
| `contrib/pdfium_patches/` | Patches for building PDFium from source (Strategy B) |

---

## File Naming Conventions

| Convention | Example | Applies To |
|-----------|---------|-----------|
| PascalCase.h / .cpp | `Document.h`, `TileCache.cpp` | Class declarations and definitions |
| camelCase for functions | `openFile()`, `renderPage()` | Member and free functions |
| camelCase for variables | `currentPage`, `zoomLevel` | Local and member variables |
| `m_` prefix for members | `m_document`, `m_zoomLevel` | Class member variables |
| UPPER_CASE for constants | `kMaxCacheSize`, `kDefaultDpi` | `constexpr` and `#define` constants |
| `_` suffix for private (alternative) | `renderImpl_()` | If not using `m_` prefix style |
| One class per file | `Document.h` contains only `class Document` | All source files |
| Header/source split | `Window.h` + `Window.cpp` | All modules (except `core/`) |
| `internal/` for private | `internal/D2DRenderer.h` | Implementation details not in public API |
| Test suffix | `PdfEngineTests.cpp`, `TileCacheTests.cpp` | All test files |

---

## Comparison with Current Structure

| Aspect | Current (Tauri/Gradle) | Proposed (Native C++) |
|--------|------------------------|----------------------|
| **Root config** | `build.gradle.kts`, `settings.gradle.kts`, `package.json`, `Cargo.toml`, `tauri.conf.json` | `CMakeLists.txt`, `CMakePresets.json` |
| **Source languages** | Java, TypeScript, Rust | C++20 only |
| **Module system** | Gradle subprojects (common, core, proprietary, saas) | CMake `add_subdirectory()` with static libraries |
| **Frontend dir** | `src/main/frontend/` (Vite + React) | None (native Win32 UI) |
| **Backend dir** | `src/main/kotlin/` or `src/main/java/` | None (no server) |
| **Rust dir** | `src-tauri/` | None (no Tauri) |
| **Python dir** | `engine/` (pyproject.toml) | None (PDFium C API directly) |
| **Test config** | JUnit (Java), Vitest (JS), cargo test (Rust), pytest (Python) | Google Test (C++ only) |
| **Resource format** | `.properties`, `.json`, `.yaml`, `static/` assets | `.rc`, `.manifest`, `.json`, `.ico`, `.ttf` |
| **CI config** | `.github/workflows/` (40+ files) | `.github/workflows/` (5-8 files) |
| **Docker** | `Dockerfile` (3-stage), `docker-compose.yml` | None |
| **Linting** | `.pre-commit-config.yaml` | `.clang-format`, `.clang-tidy` |
| **Code quality** | `.gitleaksignore`, `.imgbotconfig` | `.clang-tidy` (replaces most) |
| **Total source files (est.)** | ~500+ across 4 languages | ~100-120 C++ files |
| **Total directories (est.)** | ~200+ | ~40 |