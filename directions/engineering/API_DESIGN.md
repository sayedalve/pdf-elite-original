# API Design — Internal Module Interfaces

> Engineering doc for PDF Elite native C++/Win32/PDFium rebuild
> Target: Files 26 of 31 in engineering documentation set

---

## 1. Overview

This document defines the **internal C++ interfaces** that mediate between all modules in the native PDF Elite application. These interfaces form the **module boundary contracts**: each module consumes only the interfaces it needs and never reaches into another module's internals.

**FACT:** The current web app has no formal interface boundaries. The `@app/*` module system resolves through a per-flavor cascade (core → proprietary → desktop → saas → cloud), creating implicit dependencies rather than explicit contracts.

**RECOMMENDATION:** The native app must define explicit C++ abstract interfaces (pure virtual classes) at module boundaries. This enables independent compilation, testability, and future refactoring.

---

## 2. Design Principles

| Principle | Rule | Rationale |
|-----------|------|-----------|
| **Opaque PDFium** | No `FPDF_*` types leak beyond `pdf_engine` module | Decouples rendering/UI from PDFium API changes |
| **Interface-only boundaries** | Modules interact via abstract classes only | Enables mocking in tests, swap implementations |
| **RAII ownership** | All document/page handles use `std::unique_ptr` with custom deleters | Prevents resource leaks without GC |
| **Error via Result<T>** | Operations return `Result<T>` instead of exceptions | Consistent error handling, no exception overhead |
| **String views** | Input strings are `std::string_view`; outputs are `std::string` | Zero-copy inputs, clear ownership on outputs |
| **Async via callbacks** | Long operations accept `std::function<void(Result<T>)>` | Fits Win32 message-pump model without threads |

---

## 3. Core Interfaces

### 3.1 IDocument

The central interface representing an open PDF document. Created by `pdf_engine`, consumed by rendering, editing, and UI modules.

```cpp
// pdf_engine/public/idocument.h
#pragma once
#include "types.h"
#include "result.h"
#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <chrono>

class IDocument {
public:
    virtual ~IDocument() = default;

    // Lifecycle
    virtual void Close() = 0;
    virtual Result<void> Save(std::string_view path) = 0;
    virtual Result<void> SaveAs(std::string_view path) = 0;

    // Document info
    virtual int GetPageCount() const = 0;
    virtual Result<DocumentMetadata> GetMetadata() const = 0;
    virtual Result<void> SetMetadata(const DocumentMetadata& meta) = 0;

    // Page access
    virtual Result<std::unique_ptr<IPage>> GetPage(int page_index) = 0;

    // Modification state
    virtual bool IsModified() const = 0;
    virtual void MarkClean() = 0;

    // Security
    virtual bool IsEncrypted() const = 0;
    virtual Result<void> SetPassword(std::string_view password) = 0;

    // Identifier
    virtual std::string GetDocumentId() const = 0;
};
```

### 3.2 IPage

Represents a single page within a document. Returned by `IDocument::GetPage()`.

```cpp
// pdf_engine/public/ipage.h
#pragma once
#include "types.h"
#include "result.h"
#include <memory>
#include <vector>
#include <optional>

class IPage {
public:
    virtual ~IPage() = default;

    // Rendering (delegated to render module)
    virtual Result<PageBitmap> Render(
        const RenderParams& params
    ) = 0;

    // Text extraction
    virtual Result<std::string> GetText() const = 0;
    virtual Result<TextBounds> GetTextBounds(
        int start_char, int end_char
    ) const = 0;

    // Annotations
    virtual Result<std::vector<AnnotationInfo>> GetAnnotations() const = 0;
    virtual Result<std::unique_ptr<IAnnotation>> CreateAnnotation(
        AnnotationType type, const RectF& bounds
    ) = 0;

    // Form fields
    virtual Result<std::vector<FormFieldInfo>> GetFormFields() const = 0;

    // Page properties
    virtual SizeF GetSize() const = 0;
    virtual int GetRotation() const = 0;  // 0, 90, 180, 270
    virtual Result<void> SetRotation(int degrees) = 0;

    // Links
    virtual Result<std::vector<LinkInfo>> GetLinks() const = 0;

    // Label
    virtual std::string GetLabel() const = 0;
};
```

### 3.3 IAnnotation

Manipulates a single annotation on a page. Mirrors the 12 annotation types from the current EmbedPDF `AnnotationPlugin`.

```cpp
// pdf_engine/public/iannotation.h
#pragma once
#include "types.h"
#include "result.h"
#include <optional>

class IAnnotation {
public:
    virtual ~IAnnotation() = default;

    virtual AnnotationType GetType() const = 0;
    virtual std::string GetId() const = 0;

    // Properties
    virtual AnnotationProperties GetProperties() const = 0;
    virtual Result<void> SetProperties(const AnnotationProperties& props) = 0;

    // Content (for text annotations, etc.)
    virtual Result<std::string> GetContent() const = 0;
    virtual Result<void> SetContent(std::string_view text) = 0;

    // Appearance
    virtual Result<void> SetAppearanceColor(Color color) = 0;
    virtual Result<void> SetAppearanceStroke(float width) = 0;

    // Lifecycle
    virtual Result<void> Delete() = 0;
    virtual bool IsDeleted() const = 0;

    // History
    virtual Result<void> Undo() = 0;
    virtual Result<void> Redo() = 0;
    virtual bool CanUndo() const = 0;
    virtual bool CanRedo() const = 0;
};
```

### 3.4 ITextSelection

Manages text selection on the PDF canvas. Replaces the current `TextSelectionMenu` with clipboard interaction.

```cpp
// pdf_engine/public/itextselection.h
#pragma once
#include "types.h"
#include "result.h"
#include <string>

class ITextSelection {
public:
    virtual ~ITextSelection() = default;

    virtual Result<void> Select(
        int page_index,
        PointF start,
        PointF end
    ) = 0;

    virtual Result<void> SelectAll(int page_index) = 0;
    virtual Result<void> ClearSelection() = 0;

    virtual Result<std::string> GetSelectedText() const = 0;
    virtual Result<void> CopyToClipboard(HWND hwnd) const = 0;

    virtual bool HasSelection() const = 0;
    virtual SelectionBounds GetSelectionBounds() const = 0;
};
```

### 3.5 ISearch

Replaces the current `SearchPlugin` (Ctrl+F, 300ms debounce, next/prev, match count).

```cpp
// pdf_engine/public/isearch.h
#pragma once
#include "types.h"
#include "result.h"
#include <vector>
#include <functional>

class ISearch {
public:
    virtual ~ISearch() = default;

    virtual Result<SearchResults> Search(
        std::string_view query,
        SearchOptions options = {}
    ) = 0;

    virtual Result<void> Next() = 0;
    virtual Result<void> Previous() = 0;

    virtual const SearchResults& GetResults() const = 0;
    virtual int GetCurrentIndex() const = 0;

    virtual void Clear() = 0;
    virtual bool IsActive() const = 0;

    // Async search for large documents
    virtual void SearchAsync(
        std::string_view query,
        std::function<void(Result<SearchResults>)> callback,
        SearchOptions options = {}
    ) = 0;
};
```

### 3.6 IRenderCache

Tile-based render cache used by the rendering module. Not exposed to UI directly.

```cpp
// render/public/irendercache.h
#pragma once
#include "types.h"
#include "result.h"
#include <memory>

class IRenderCache {
public:
    virtual ~IRenderCache() = default;

    virtual Result<std::shared_ptr<PageBitmap>> RequestTile(
        const TileKey& key,
        std::function<void(std::shared_ptr<PageBitmap>)> on_ready = nullptr
    ) = 0;

    virtual void Evict(const TileKey& key) = 0;
    virtual void EvictPage(int page_index) = 0;
    virtual void Clear() = 0;

    virtual size_t GetMemoryUsage() const = 0;
    virtual size_t GetTileCount() const = 0;

    virtual void SetMaxMemory(size_t bytes) = 0;
};
```

### 3.7 IThumbnailGenerator

Generates page thumbnails for the sidebar/page navigator.

```cpp
// render/public/ithumbnailgenerator.h
#pragma once
#include "types.h"
#include "result.h"
#include <memory>
#include <functional>

class IThumbnailGenerator {
public:
    virtual ~IThumbnailGenerator() = default;

    virtual void Generate(
        int page_index,
        float scale,
        std::function<void(std::shared_ptr<PageBitmap>)> callback
    ) = 0;

    virtual void Cancel(int page_index) = 0;
    virtual void CancelAll() = 0;

    virtual void SetMaxCacheSize(size_t bytes) = 0;
    virtual std::shared_ptr<PageBitmap> GetCached(int page_index) const = 0;
};
```

---

## 4. Shared Types

```cpp
// common/public/types.h
#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Geometry
struct PointF { float x, y; };
struct SizeF  { float width, height; };
struct RectF  { float left, top, right, bottom; };
struct RectI  { int left, top, right, bottom; };

// Color (ARGB)
struct Color { uint8_t a, r, g, b; };

// Render parameters
class RenderParams {
public:
    float scale = 1.0f;
    int   dpi = 72;
    bool  antialias = true;
    bool  render_annotations = true;
    bool  render_form_fields = true;
    RectF clip_rect;  // empty = full page
};

// Tile key for cache
class TileKey {
public:
    int   page_index;
    float scale;
    int   tile_x, tile_y;  // tile grid coordinates
    bool operator==(const TileKey&) const = default;
};

// Annotation types (mirrors EmbedPDF AnnotationPlugin 12 types)
enum class AnnotationType {
    Text, Highlight, Underline, StrikeOut, Squiggly,
    FreeText, Ink, Stamp, Link, Popup,
    Square, Circle, Line, Polygon, PolyLine
};

// Document metadata
class DocumentMetadata {
public:
    std::string title, author, subject, keywords;
    std::string creator, producer;
    std::string creation_date, modification_date;
};

// Page bitmap (HBITMAP wrapper or DIB section)
struct PageBitmap {
    void*  pixels = nullptr;   // BGRA pixel data
    int    width = 0;
    int    height = 0;
    int    stride = 0;         // bytes per row
    float  scale = 1.0f;      // render scale used
};

// Search result
class SearchResult {
public:
    int    page_index;
    RectF  bounds;
    int    char_start, char_end;
};

class SearchResults {
public:
    std::vector<SearchResult> items;
    int total_count = 0;
};

struct SearchOptions {
    bool  case_sensitive = false;
    bool  whole_word = false;
    int   start_page = 0;
    int   max_results = 0;  // 0 = unlimited
};
```

---

## 5. Module Dependency Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                         UI MODULE                           │
│  (Win32 windows, dialogs, toolbar, sidebar, status bar)      │
│                                                              │
│  Consumes: IRenderCache, IThumbnailGenerator,                │
│            ITextSelection, ISearch, IAnnotation              │
│  NEVER touches: PDFium, IDocument internals                  │
└──────────┬─────────────────────────────────┬────────────────┘
           │                                 │
           ▼                                 ▼
┌─────────────────────┐          ┌──────────────────────────┐
│   RENDERING MODULE  │          │    EDITING MODULE        │
│                     │          │                          │
│  Receives: IDocument│          │  Receives: IDocument    │
│  Returns: bitmaps   │          │  Modifies via:           │
│                     │          │    IAnnotation           │
│  Exposes:           │          │    ITextSelection        │
│    IRenderCache     │          │    ISearch               │
│    IThumbnailGen    │          │    Form field interfaces  │
└──────────┬──────────┘          └────────────┬─────────────┘
           │                                  │
           ▼                                  ▼
┌──────────────────────────────────────────────────────────────┐
│                     PDF ENGINE MODULE                        │
│                                                              │
│  Exposes: IDocument, IPage, IAnnotation, ITextSelection,     │
│           ISearch (all as pure virtual interfaces)           │
│  Internals: PDFium FPDF_* types (NEVER exported)             │
│  Also provides: IFormFiller, IBookmarkManager                │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
                    ┌──────────────┐
                    │   PDFium     │
                    │  (static lib)│
                    └──────────────┘

┌──────────────────────────────────────────────────────────────┐
│                    COMMON MODULE                             │
│  types.h, result.h, error_codes.h (shared across all)       │
└──────────────────────────────────────────────────────────────┘
```

---

## 6. Module Boundary Rules

### 6.1 PDF Engine → All Others

| Rule | Detail |
|------|--------|
| Exported types | Only interface classes and value types from `types.h` |
| Forbidden exports | `FPDF_DOCUMENT`, `FPDF_PAGE`, `FPDF_ANNOTATION`, any `fpdf_` function |
| Header structure | `pdf_engine/public/*.h` — these are the only headers other modules include |
| Implementation | `pdf_engine/src/*.cpp` — links against PDFium static library |

**FACT:** In the current web app, the `OperationRouter` dynamically routes tool requests to local/SaaS/self-hosted backends. The native app eliminates this entirely — all PDF operations are local via PDFium.

### 6.2 Rendering Module

| Input | Output |
|-------|--------|
| `IDocument`, `IPage` | `PageBitmap` (raw pixel data) |
| Render parameters | Tile cache entries |
| Never receives | HWND, HDC, or any Win32 UI types |
| Never produces | Window handles, UI commands |

### 6.3 UI Module

| Input | Output |
|-------|--------|
| `PageBitmap` from cache | GDI/ Direct2D draw calls |
| User input events | Tool/annotation commands via interfaces |
| Never receives | PDFium types, raw document handles |
| Never produces | PDF modifications directly |

### 6.4 Editing Module

| Input | Output |
|-------|--------|
| `IDocument`, `IPage` | Modified annotation/form state |
| `IAnnotation` pointers | History entries for undo/redo |
| Never receives | Render cache, bitmap data, HWND |
| Never produces | Pixel data, draw calls |

---

## 7. Interface Factory / Document Manager

A single `IDocumentManager` creates and tracks open documents:

```cpp
// pdf_engine/public/idocument_manager.h
class IDocumentManager {
public:
    virtual ~IDocumentManager() = default;

    virtual Result<std::unique_ptr<IDocument>> OpenDocument(
        std::string_view path,
        std::string_view password = ""
    ) = 0;

    virtual Result<std::unique_ptr<IDocument>> OpenDocumentFromBuffer(
        const std::vector<uint8_t>& data,
        std::string_view password = ""
    ) = 0;

    virtual Result<std::unique_ptr<IDocument>> CreateNewDocument(
        const SizeF& page_size = {612.0f, 792.0f}  // US Letter
    ) = 0;

    virtual int GetOpenDocumentCount() const = 0;
    virtual bool CloseDocument(std::string_view doc_id) = 0;
};
```

---

## 8. Result Type

All fallible operations return `Result<T>` instead of throwing exceptions:

```cpp
// common/public/result.h
template<typename T>
class Result {
public:
    // Success
    static Result Ok(T value);

    // Failure
    static Result Err(ErrorCode code, std::string message);

    bool IsOk() const;
    bool IsErr() const;
    const T& Value() const;
    const T& ValueOr(const T& fallback) const;
    ErrorCode Error() const;
    const std::string& ErrorMessage() const;
};

// Specialization for void
enum class ErrorCode {
    None = 0,
    FileNotFound,
    PermissionDenied,
    InvalidPassword,
    CorruptPdf,
    OutOfMemory,
    InvalidPage,
    InvalidAnnotation,
    EngineError,
    OperationCancelled,
};
```

---

## 9. Comparison: Current vs. Proposed

| Aspect | Current (Web/Tauri) | Proposed (Native C++) |
|--------|---------------------|----------------------|
| Document handle | JavaScript object in EmbedPDF | `std::unique_ptr<IDocument>` |
| Page access | `pdfDoc.getPage(n)` (JS) | `doc->GetPage(n)` → `IPage` |
| Annotations | `AnnotationPlugin` (internal) | `IAnnotation` (public interface) |
| Text selection | `TextSelectionMenu` + clipboard API | `ITextSelection::CopyToClipboard()` via Win32 |
| Search | `SearchPlugin` with debounced input | `ISearch` with sync + async modes |
| Rendering | Canvas 2D in browser | `IRenderCache` → `PageBitmap` → GDI/D2D |
| Tool routing | `OperationRouter` → local/SaaS/self-hosted | Direct `IDocument` calls (all local) |
| Error handling | try/catch + error boundaries | `Result<T>` monadic errors |

---

## 10. Header Dependency Graph

```
types.h          ← no dependencies (leaf)
result.h         ← types.h (ErrorCode enum)

idocument.h      ← types.h, result.h, ipage.h
ipage.h          ← types.h, result.h, iannotation.h, itextselection.h
iannotation.h    ← types.h, result.h
itextselection.h ← types.h, result.h
isearch.h        ← types.h, result.h

irendercache.h   ← types.h, result.h
ithumbnailgen.h  ← types.h, result.h

idocument_manager.h ← idocument.h, result.h
```

**RECOMMENDATION:** Keep the header dependency graph acyclic. The `types.h` leaf ensures no circular includes. Use forward declarations where needed to minimize compile-time dependencies.

---

*Document 26 of 31 — API Design*
