# PDF Engine Architecture — PDFium for Native C++/Win32

> Design for the PDFium-based PDF engine in the native rewrite of PDF Elite.

---

## 1. PDFium C API Overview

PDFium exposes a flat C API. All types are opaque handles. The primary handle types are:

| Handle Type | Description | Lifecycle |
|-------------|-------------|-----------|
| `FPDF_LIBRARY` | Library singleton | Init once per process, destroy at exit |
| `FPDF_DOCUMENT` | Open PDF document | `FPDF_LoadDocument` → `FPDF_CloseDocument` |
| `FPDF_PAGE` | Single page | `FPDF_LoadPage` → `FPDF_ClosePage` |
| `FPDF_TEXTSTREAM` | Text extraction stream | `FPDFText_LoadPage` → `FPDFText_ClosePage` |
| `FPDF_SCHHANDLE` | Search context | `FPDFText_FindStart` → `FPDFText_FindClose` |
| `FPDF_ANNOTATION` | Annotation object | `FPDFPage_GetAnnot` → `FPDFPage_CloseAnnot` |
| `FPDF_FORMHANDLE` | Form fill interface | `FWL_Create` → `FWL_Destroy` |
| `FPDF_BITMAP` | Rendered bitmap | `FPDFBitmap_Create` → `FPDFBitmap_Destroy` |
| `FPDF_PAGEOBJECT` | Page content object | Created/destroyed via page object APIs |
| `FPDF_CLIPPATH` | Clipping path | `FPDFClipPath_CountPaths` / `CountPathSegments` |
| `FPDF_BOOKMARK` | Document outline entry | `FPDFBookmark_GetFirstChild` traversal |
| `FPDF_ATTACHMENT` | Embedded file | `FPDFDoc_GetAttachment` traversal |
| `FPDF_DEST` | Destination (link target) | Referenced by bookmarks and link annotations |
| `FPDF_ACTION` | Action (link behavior) | `FPDFAnnot_GetAction` |
| `FPDF_PAGELINK` | Link on page | `FPDFLink_GetLinkAtPoint` |

### Key Header Files

```
fpdfview.h      — Document/page lifecycle, rendering, bitmap
fpdftext.h      — Text extraction, search, text selection
fpdf_annot.h    — Annotation CRUD, properties
fpdf_formfill.h — Interactive form filling (XFA, AcroForm)
fpdfdoc.h       — Bookmarks, metadata, attachments, destinations, actions
fpdfpage.h      — Page objects, content editing, page manipulation
fpdf_save.h     — Save / incremental save
fpdf_edit.h     — Page creation, object creation (text, path, image)
fpdf_transformpage.h — Page rotation, scaling, cropping
fpdf_thumbnail.h — Thumbnail generation (simplified rendering)
fpdf_dataavail.h — Progressive loading / linearized PDF support
fpdf_ext.h      — SDK extension callbacks, notifications
fpdf_ppo.h      — Page-level operations (import pages)
```

---

## 2. Proposed C++ Wrapper Design

### 2.1 RAII Wrapper Hierarchy

```
PdfiumLibrary      — Singleton, owns FPDF_InitLibrary / FPDF_DestroyLibrary
  └─ PdfDocument    — RAII for FPDF_DOCUMENT
       ├─ PdfPage   — RAII for FPDF_PAGE (loaded/closed on demand)
       │    ├─ PdfTextPage   — RAII for FPDF_TEXTPAGE
       │    ├─ PdfAnnotation — RAII for FPDF_ANNOTATION
       │    └─ PdfPageObject — RAII for FPDF_PAGEOBJECT
       ├─ PdfBookmark   — Traversal helper (not RAII, just iterator)
       ├─ PdfAttachment — RAII for attachment access
       └─ PdfFormFillEnv — RAII for FPDF_FORMHANDLE
```

### 2.2 Core RAII Wrappers

#### PdfiumLibrary (Singleton)

```cpp
class PdfiumLibrary {
public:
    static PdfiumLibrary& Instance();
    void Initialize(const PdfiumConfig& config = {});
    ~PdfiumLibrary();
    // Non-copyable, non-movable
    PdfiumLibrary(const PdfiumLibrary&) = delete;
    PdfiumLibrary& operator=(const PdfiumLibrary&) = delete;
private:
    PdfiumLibrary() = default;
    bool m_initialized = false;
};
```

**FACT:** PDFium requires single `FPDF_InitLibraryWithConfig` call per process. Multiple init calls are undefined behavior.

#### PdfDocument

```cpp
class PdfDocument {
public:
    static std::expected<PdfDocument, PdfError> LoadFromFile(const wchar_t* path);
    static std::expected<PdfDocument, PdfError> LoadFromMemory(const uint8_t* data, size_t len);

    // Move-only (transfers ownership of FPDF_DOCUMENT)
    PdfDocument(PdfDocument&& other) noexcept;
    PdfDocument& operator=(PdfDocument&& other) noexcept;

    int PageCount() const;
    std::expected<PdfPage, PdfError> GetPage(int index);
    std::expected<void, PdfError> ImportPages(const PdfDocument& source, const char* page_range, int dest_index);
    std::expected<void, PdfError> DeletePage(int index);
    std::expected<void, PdfError> SaveToFile(const wchar_t* path);
    std::expected<void, PdfError> SaveIncremental(const wchar_t* path);  // fast, append-only

    // Metadata
    std::string GetTitle() const;
    std::string GetAuthor() const;
    std::string GetSubject() const;
    // ... other metadata

    // Bookmarks
    PdfBookmark GetBookmarks() const;

    // Attachments
    int AttachmentCount() const;
    PdfAttachment GetAttachment(int index) const;

    ~PdfDocument();
private:
    FPDF_DOCUMENT m_doc = nullptr;
    std::recursive_mutex m_mutex;  // Per-document lock for thread safety
};
```

**FACT:** `FPDF_CloseDocument` must be called exactly once per document. RAII guarantees this.

#### PdfPage

```cpp
class PdfPage {
public:
    double GetWidth() const;   // Points
    double GetHeight() const;  // Points
    int GetRotation() const;   // 0, 90, 180, 270
    void SetRotation(int degrees);

    // Rendering
    std::expected<PdfBitmap, PdfError> Render(double scale_x, double scale_y, int flags = 0);
    std::expected<PdfBitmap, PdfError> RenderRegion(double left, double top, double right, double bottom, double scale_x, double scale_y);

    // Text
    std::expected<PdfTextPage, PdfError> LoadTextPage();

    // Annotations
    int AnnotationCount() const;
    std::expected<PdfAnnotation, PdfError> GetAnnotation(int index);
    std::expected<PdfAnnotation, PdfError> CreateAnnotation(FPDF_ANNOTATION_SUBTYPE type);

    // Page objects (content editing)
    int ObjectCount() const;
    PdfPageObject GetObject(int index);
    void InsertObject(const PdfPageObject& obj);
    void RemoveObject(const PdfPageObject& obj);

    // Search
    std::expected<PdfSearchContext, PdfError> CreateSearch(const wchar_t* term, int flags);

    ~PdfPage();
private:
    FPDF_DOCUMENT m_owner_doc = nullptr;  // Prevent doc destruction while page exists
    int m_page_index = -1;
    FPDF_PAGE m_page = nullptr;
};
```

**RECOMMENDATION:** Pages should be lazily loaded and cached. For a 1000-page document, loading all pages upfront wastes memory. Use an LRU page cache.

#### PdfAnnotation

```cpp
class PdfAnnotation {
public:
    FPDF_ANNOTATION_SUBTYPE GetType() const;
    FS_RECTF GetRect() const;
    void SetRect(const FS_RECTF& rect);

    // Color
    void GetColor(FPDFANNOT_COLORTYPE type, float& r, float& g, float& b, float& a);
    void SetColor(FPDFANNOT_COLORTYPE type, float r, float g, float b, float a);

    // Text content (for FreeText, StickyNote)
    std::string GetStringValue(const char* key) const;
    void SetStringValue(const char* key, const char* value);

    // Ink paths
    void SetInkList(const FS_POINTF* points, int count);

    // Line
    void SetLine(const FS_POINTF& start, const FS_POINTF& end);

    // Vertices (polygon, polyline)
    void SetVertices(const FS_POINTF* points, int count);

    // Link action
    void SetAction(FPDF_ACTION action);

    // Persist changes
    std::expected<void, PdfError> UpdateAp();  // Regenerate appearance stream

    ~PdfAnnotation();
private:
    FPDF_PAGE m_page = nullptr;
    FPDF_ANNOTATION m_annot = nullptr;
};
```

### 2.3 Abstraction Layer Interfaces

To allow potential future engine swapping (e.g., for features PDFium doesn't support), define interfaces:

#### IDocument

```cpp
class IDocument {
public:
    virtual ~IDocument() = default;
    virtual int PageCount() const = 0;
    virtual std::unique_ptr<IPage> GetPage(int index) = 0;
    virtual std::string GetMetadata(const std::string& key) const = 0;
    virtual std::vector<BookmarkNode> GetBookmarks() const = 0;
    virtual bool Save(const std::wstring& path) = 0;
    virtual bool SaveIncremental(const std::wstring& path) = 0;
};
```

#### IPage

```cpp
class IPage {
public:
    virtual ~IPage() = default;
    virtual SizeF GetSize() const = 0;
    virtual int GetRotation() const = 0;
    virtual void SetRotation(int degrees) = 0;
    virtual std::unique_ptr<ITextSelection> LoadText() = 0;
    virtual std::unique_ptr<IAnnotationCollection> GetAnnotations() = 0;
    virtual std::unique_ptr<ISearchable> CreateSearch(const std::wstring& term) = 0;
    virtual std::unique_ptr<IRenderedTile> Render(const RenderRequest& req) = 0;
};
```

#### ITextSelection

```cpp
class ITextSelection {
public:
    virtual ~ITextSelection() = default;
    virtual int CharCount() const = 0;
    virtual std::wstring GetText(int start, int count) const = 0;
    virtual RectF GetCharRect(int char_index) const = 0;
    virtual int GetCharIndexAtPoint(double x, double y) const = 0;
    virtual std::vector<RectF> GetSelectionRects(int start, int count) const = 0;
};
```

#### IAnnotation

```cpp
class IAnnotation {
public:
    virtual ~IAnnotation() = default;
    virtual AnnotationType GetType() const = 0;
    virtual RectF GetRect() const = 0;
    virtual void SetRect(const RectF& rect) = 0;
    virtual void SetColor(Color color) = 0;
    virtual Color GetColor() const = 0;
    virtual std::string GetContents() const = 0;
    virtual void SetContents(const std::string& text) = 0;
    virtual bool UpdateAppearance() = 0;
};
```

#### ISearchable

```cpp
class ISearchable {
public:
    virtual ~ISearchable() = default;
    virtual SearchResult FindNext() = 0;
    virtual SearchResult FindPrev() = 0;
    virtual void SetFlags(int flags) = 0;  // case-sensitive, whole-word
};
```

**RECOMMENDATION:** Start with concrete PDFium implementations of these interfaces. The interface layer adds minimal overhead (virtual dispatch) but provides future flexibility.

---

## 3. Why PDFium Is Sufficient for Target Features

### 3.1 What PDFium Provides

| Capability | PDFium C API | PDF Elite Needs It? |
|-----------|--------------|---------------------|
| Page rendering (vector→bitmap) | `FPDF_RenderPageBitmap`, `FPDF_RenderPageBitmapInDC` | YES — core feature |
| Text extraction | `FPDFText_GetText`, `FPDFText_GetCharBox` | YES — search, selection, copy |
| Text search | `FPDFText_FindStart` / `FindNext` | YES — Ctrl+F |
| Text hit-testing | `FPDFText_GetCharIndexAtPoint` | YES — text selection |
| Annotation read | `FPDFPage_GetAnnot`, `FPDFAnnot_Get*` | YES — display annotations |
| Annotation write | `FPDFPage_CreateAnnot`, `FPDFAnnot_Set*` | YES — create/edit annotations |
| Annotation appearance | `FPDFAnnot_UpdateAp` | YES — regenerate annotation visuals |
| Form field access | `FPDFAnnot_GetFormFieldAtPoint`, `FPDF_FFLFillField` | NO in trimmed set (P3+ future) |
| Page insertion/deletion | `FPDF_ImportPages`, `FPDFPage_Delete` | YES — split, merge, reorganize |
| Page reordering | `FPDF_ImportPagesByIndex` | YES — reorganize |
| Page rotation | `FPDFPage_SetRotation` | YES — rotate, auto-rotate |
| Page creation | `FPDFPage_New` | YES — insert blank pages |
| Merge documents | `FPDF_ImportPages` | YES — merge tool |
| Split documents | `FPDF_ImportPagesByIndex` + save | YES — split tool |
| Bookmarks/Outlines | `FPDFBookmark_*` family | YES — bookmark sidebar |
| Metadata | `FPDF_GetMetaText` | YES — PDF info |
| Attachments | `FPDFDoc_GetAttachment*` | YES — attachment sidebar |
| Links/Actions | `FPDFLink_GetLinkAtPoint` | YES — link annotations |
| Incremental save | `FPDF_SaveAsDocument` with flags | YES — fast save |
| Full save | `FPDF_SaveAsDocument` | YES — save-as |
| Image objects | `FPDFPageObj_NewImageObj`, `FPDFImageObj_SetBitmap` | YES — add/replace/remove images |
| Text objects | `FPDFPageObj_CreateTextObj` | YES — page numbers, free-text annotations |
| Path objects | `FPDFPageObj_NewPathObj` | YES — shape annotations (square, circle, line) |
| Page object manipulation | `FPDFPage_InsertObject`, `FPDFPage_RemoveObject` | YES — content editing |
| Thumbnail rendering | `FPDF_RenderPageBitmap` at low scale | YES — thumbnail sidebar (single engine) |
| Progressive loading | `FPDF_DATAAVAIL_*` | NICE — for large file UX |
| Print to DC | `FPDF_RenderPageBitmapInDC` | YES — native print |

**FACT:** PDFium covers 100% of the P0 and P1 feature requirements for the trimmed 17-tool feature set.

### 3.2 What PDFium Does NOT Provide

| Capability | Needed? | Alternative |
|-----------|---------|------------|
| OCR (Tesseract integration) | NO (not in trimmed set) | Tesseract C API if needed later |
| Format conversion (DOCX, XLSX, PPTX) | NO (not in trimmed set) | LibreOffice CLI if needed later |
| JavaScript execution | NO | N/A — not in feature set |
| Digital signatures (certificate-based) | NO (not in trimmed set) | Windows CryptoAPI + custom implementation if needed |
| Redaction burn-in (secure content removal) | PARTIAL — PDFium can create redaction annotations and has `FPDFAnnot_ApplyApplier` for redaction, but comprehensive content verification may need additional passes | Custom text-scanning pass before PDFium redaction |
| AI features (summarization, Q&A) | NO (not in trimmed set) | LLM API integration (not PDF engine concern) |
| PDF/A compliance | NO (not in trimmed set) | veraPDF or Ghostscript if needed later |
| Font subsetting/embedding advanced | RARELY | Direct PDF object manipulation if needed |

**FACT:** For the trimmed 17-tool feature set, PDFium alone is sufficient. No additional PDF library is needed.

**RECOMMENDATION:** Design the architecture so that non-PDFium capabilities (if added later) plug in as separate modules, not as competing PDF engines.

---

## 4. PDFium Build Configuration for Windows x64

### 4.1 Obtaining PDFium

| Method | Description | Recommended? |
|--------|-------------|-------------|
| Google's prebuilt binaries | `pdfium.appspot.com` or Chromium CI artifacts | YES for initial development |
| Build from Chromium source | `depot_tools` + `gn` + `ninja` | YES for production (custom flags) |
| pdfium-win-docker-build | Community Docker builds | NO (not controlled) |
| bblanchon/pdfium-binaries | GitHub releases, multiple targets | MAYBE — convenient but may lag |

### 4.2 Recommended Build Configuration

```
is_debug = false
target_cpu = "x64"
target_os = "win"
pdf_use_win32 = true
pdf_enable_xfa = false          # XFA not needed (legacy forms)
pdf_enable_v8 = false          # No JS execution needed
pdf_enable_skia = false        # Use GDI/D2D instead of Skia
pdf_use_starnames = false      # Not needed
pdf_enable_callout = false     # Not needed
```

**RECOMMENDATION:** Build with `is_debug=false` and `pdf_enable_v8=false` for minimal binary size (~30MB) and no V8 dependency. V8 adds ~100MB and is unnecessary for PDF Elite's feature set.

**RECOMMENDATION:** Consider `pdf_enable_skia=true` only if D2D rendering proves insufficient for quality. Skia adds ~20MB but provides better text rendering at extreme zoom levels.

### 4.3 Build Dependencies

| Dependency | Version | Purpose |
|-----------|---------|---------|
| Visual Studio 2022 | 17.x | C++ compiler, MSVC toolchain |
| Windows SDK | 10/11 | Win32 headers, D2D, GDI |
| depot_tools | latest | Chromium build toolchain |
| ninja | latest | Build system |

### 4.4 Output Artifacts

```
pdfium.dll          — Core PDFium library (~30MB release)
pdfium.lib          — Import library
public/             — C API headers (fpdfview.h, etc.)
resources/          — Optional CJK font data
```

---

## 5. Memory Management Model

### 5.1 Ownership Rules

| Object | Owner | Lifetime |
|--------|-------|----------|
| `FPDF_LIBRARY` | Process (singleton) | Application lifetime |
| `FPDF_DOCUMENT` | `PdfDocument` RAII wrapper | Until wrapper destroyed or moved |
| `FPDF_PAGE` | `PdfPage` RAII wrapper | Until page evicted from cache or wrapper destroyed |
| `FPDF_TEXTPAGE` | `PdfTextPage` RAII wrapper | Transient — created per-operation |
| `FPDF_ANNOTATION` | `PdfAnnotation` RAII wrapper | While page is loaded (page owns annotations) |
| `FPDF_BITMAP` | `PdfBitmap` RAII wrapper | Transient — created per-render, destroyed after blit to D2D |
| `FPDF_PAGEOBJECT` | Shared ownership | Created by user, owned by page after `InsertObject` |
| `FPDF_SCHHANDLE` | `PdfSearchContext` RAII wrapper | While search is active |

### 5.2 Page Cache Strategy

```
LRU Page Cache (max N pages, configurable, default 10)
┌──────────────────────────────────────────────┐
│ Page 3  [loaded]  ──┐                        │
│ Page 7  [loaded]  ──┤  Recently accessed     │
│ Page 12 [loaded]  ──┘                        │
│ Page 1  [loaded]  ──┐  Less recently accessed │
│ Page 5  [loaded]  ──┘                        │
│ Page 9  [evicted]   (FPDF_ClosePage called)   │
└──────────────────────────────────────────────┘
```

**RECOMMENDATION:** Default cache size of 10 pages. At 768px tile rendering and ~4MB per loaded page state, this uses ~40MB for page objects. Tile bitmaps are managed separately in a GPU-memory-aware cache.

### 5.3 Bitmap Memory

```
Render Pipeline Memory:
  FPDF_RenderPageBitmap → FPDF_BITMAP (CPU memory)
  → Copy to ID2D1Bitmap (GPU memory)
  → FPDFBitmap_Destroy (free CPU memory)
  → D2D renders from GPU bitmap
```

**RECOMMENDATION:** Use `FPDFBitmap_CreateEx` with externally-allocated memory to avoid double-copy. Allocate a D2D-compatible buffer, render PDFium into it, then wrap as `ID2D1Bitmap1` without intermediate copy.

---

## 6. Threading Model

### 6.1 PDFium Threading Rules

**FACT:** PDFium is NOT thread-safe for concurrent access to the SAME document. Multiple documents CAN be accessed concurrently from different threads.

| Operation | Thread Safety | Notes |
|-----------|--------------|-------|
| `FPDF_InitLibrary` | Single-threaded (call once) | Must complete before any other calls |
| `FPDF_LoadDocument` | Thread-safe (different docs) | Each thread can load its own document |
| Rendering on Document A | NOT thread-safe | Only one thread per document at a time |
| Rendering on Document A + Document B | Thread-safe | Different documents, different threads |
| `FPDF_SaveAsDocument` | NOT thread-safe | Must not overlap with other doc operations |

### 6.2 Proposed Threading Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ UI Thread (Win32 Message Loop)                              │
│  ├─ Handles all user input, window messages, D2D rendering  │
│  ├─ Sends operations to Document Worker threads            │
│  └─ Receives results via PostMessage / callbacks            │
├─────────────────────────────────────────────────────────────┤
│ Document Worker Pool (N threads, default = CPU count - 1)   │
│  ├─ Each document assigned to a specific worker thread      │
│  ├─ Worker thread owns document mutex for all operations    │
│  ├─ Renders tiles, extracts text, modifies pages            │
│  └─ Posts completed bitmaps back to UI thread               │
├─────────────────────────────────────────────────────────────┤
│ File I/O Thread                                             │
│  ├─ Handles all file load/save operations                   │
│  ├─ Memory-mapped file loading for large PDFs               │
│  └─ Background save (incremental)                           │
└─────────────────────────────────────────────────────────────┘
```

### 6.3 Per-Document Locking

```cpp
class PdfDocument {
    std::recursive_mutex m_mutex;  // Allows re-entrant locking within same thread

public:
    // All public methods lock the mutex:
    std::expected<PdfPage, PdfError> GetPage(int index) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        // ... FPDF_LoadPage ...
    }
};
```

**RECOMMENDATION:** Use `std::recursive_mutex` per document. The UI thread may call into the document from multiple code paths (e.g., rendering + text extraction on the same page). Recursive mutex prevents deadlock.

**RECOMMENDATION:** Tile rendering should be dispatched to the document's assigned worker thread. The UI thread should NEVER call `FPDF_RenderPageBitmap` directly — it blocks the message loop.

---

## 7. Rendering Pipeline

### 7.1 PDFium → HBITMAP → D2D Pipeline

```
Step 1: UI thread determines visible tile set from viewport
Step 2: Dispatch tile render requests to document worker thread
Step 3: Worker thread calls FPDF_RenderPageBitmap (CPU rendering)
Step 4: Worker thread copies bitmap to shared memory / D2D staging texture
Step 5: Worker thread posts WM_RENDER_TILE_COMPLETE to UI thread
Step 6: UI thread composites tile bitmaps via Direct2D
```

### 7.2 Tile Rendering Details

```
Current (WASM):   768px tiles, 5px overlap, 1 extra ring
Native Proposal:   512px or 768px tiles (configurable), 4px overlap, 1 extra ring

For a 1920x1080 display at 100% zoom viewing a letter-size page:
  Page at 72 DPI = 612x792 points
  At 150% Windows scaling = 918x1188 device pixels
  Tiles needed: 2 columns × 2 rows = 4 tiles (with overlap ring: ~12 tiles)

At 400% zoom:
  2448x3168 device pixels
  Tiles needed: 5 columns × 5 rows = 25 tiles (with overlap ring: ~48 tiles)
```

### 7.3 D2D Integration

```cpp
// Render PDFium tile into D2D-compatible bitmap
void RenderTileToD2D(FPDF_PAGE page, const TileRect& rect, double scale,
                     ID2D1RenderTarget* d2dTarget) {
    int width = static_cast<int>((rect.right - rect.left) * scale);
    int height = static_cast<int>((rect.bottom - rect.top) * scale);

    // Create PDFium bitmap (BGRx, 4 bytes/pixel, pre-multiplied alpha)
    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, true);
    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);  // White background

    // Render to bitmap
    FPDF_RenderPageBitmap(bitmap, page,
        rect.left * scale, rect.top * scale,
        width, height, 0, FPDF_ANNOT | FPDF_LCD_TEXT);

    // Get pixel data
    void* buffer = FPDFBitmap_GetBuffer(bitmap);
    int stride = FPDFBitmap_GetStride(bitmap);

    // Create D2D bitmap from PDFium buffer (zero-copy if formats match)
    D2D1_BITMAP_PROPERTIES props = {};
    props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;

    ID2D1Bitmap1* d2dBitmap = nullptr;
    d2dTarget->CreateBitmap(
        D2D1::SizeU(width, height),
        buffer, stride, &props, &d2dBitmap);

    // Draw to render target
    d2dTarget->DrawBitmap(d2dBitmap,
        D2D1::RectF(rect.left * scale, rect.top * scale,
                    rect.right * scale, rect.bottom * scale));

    d2dBitmap->Release();
    FPDFBitmap_Destroy(bitmap);
}
```

**RECOMMENDATION:** Use `DXGI_FORMAT_B8G8R8A8_UNORM` which matches PDFium's native bitmap format (when using `FPDFBitmap_Create` with `alpha=1`), enabling zero-copy D2D bitmap creation.

---

## 8. Text Extraction Pipeline

### 8.1 Extraction Flow

```
FPDFText_LoadPage(page) → FPDF_TEXTPAGE
  │
  ├─ FPDFText_CountChars(texpage) → total character count
  ├─ FPDFText_GetText(texpage, start, count) → UTF-16LE text
  ├─ FPDFText_GetCharBox(texpage, index) → {left, right, bottom, top} in page coords
  ├─ FPDFText_GetCharIndexAtPoint(texpage, x, y) → character index
  └─ FPDFText_GetFontSize(texpage, index) → font size
  │
FPDFText_ClosePage(texpage)
```

### 8.2 Text Selection Algorithm

```
1. On mouse-down: Get char index at point via FPDFText_GetCharIndexAtPoint
2. On mouse-move: Get char index at current point
3. Compute selection range [min, max]
4. For each char in range: FPDFText_GetCharBox → union into highlight rectangles
5. For display: render highlight rectangles as semi-transparent overlay
6. On copy: FPDFText_GetText for the selected range → SetClipboardData
```

### 8.3 Search Pipeline

```
FPDFText_FindStart(texpage, term, flags, start_index) → FPDF_SCHHANDLE
  │
  ├─ FPDFText_FindNext(handle) → FPDF_SCHRESULT (found/not found)
  │    ├─ If found: FPDFText_GetCharBox for match range → highlight
  │    └─ If not found: search complete
  ├─ FPDFText_FindPrev(handle) → reverse search
  └─ FPDFText_FindClose(handle)

Flags: FPDF_MATCHCASE, FPDF_MATCHWHOLEWORD, FPDF_CONSECUTIVE
```

---

## 9. Annotation Read/Write Pipeline

### 9.1 Reading Annotations

```
FPDFPage_GetAnnotCount(page) → count
  FOR each index:
    FPDFPage_GetAnnot(page, index) → FPDF_ANNOTATION
    FPDFAnnot_GetSubtype(annot) → type (Highlight, Ink, etc.)
    FPDFAnnot_GetRect(annot) → bounding rectangle
    FPDFAnnot_GetColor(annot, FPDFANNOT_COLORTYPE_Color) → RGBA
    FPDFAnnot_GetStringValue(annot, "Contents") → annotation text
    FPDFAnnot_GetInkListPath(annot) → ink points (for Ink type)
    FPDFAnnot_GetLine(annot) → start/end points (for Line type)
    FPDFAnnot_GetVertices(annot) → vertex array (for Polygon/Polyline)
    FPDFPage_CloseAnnot(annot)
```

### 9.2 Writing Annotations

```
FPDFPage_CreateAnnot(page, FPDF_ANNOT_HIGHLIGHT) → FPDF_ANNOTATION
  │
  ├─ FPDFAnnot_SetRect(annot, &rect)
  ├─ FPDFAnnot_SetColor(annot, FPDFANNOT_COLORTYPE_Color, r, g, b, a)
  ├─ FPDFAnnot_SetStringValue(annot, "Contents", "Highlighted text")
  ├─ FPDFAnnot_SetStringValue(annot, "Title", "User")
  ├─ FPDFAnnot_AppendAttachmentPoints(annot, quad_points)  // for highlight/underline
  │
  ├─ FPDFAnnot_UpdateAp(annot)  // Regenerate appearance stream
  └─ (annotation is persisted when document is saved)
```

### 9.3 Annotation Overlay Rendering

```
For display (not baked into page):
  1. Read all annotations from page
  2. For each annotation, compute display rectangle/paths
  3. Render as D2D overlay on top of PDF tile bitmap:
     - Highlight: semi-transparent yellow rectangle
     - Ink: D2D path stroke with annotation color
     - StickyNote: icon + popup
     - FreeText: text layout at annotation position
     - Shapes: D2D geometry (rectangle, ellipse, line, polygon)
  4. On save, annotations are written to PDF via PDFium (not baked into content)
```

---

## 10. Form Field Handling

**FACT:** Form field support is NOT in the trimmed 17-tool feature set. This section is for future reference.

```
FPDFDOC_InitFormFillEnvironment(document, callbacks) → FPDF_FORMHANDLE
  │
  ├─ FPDF_ANNOT_WIDGET type annotations are form fields
  ├─ FORM_ForceToKillFocus(form_handle)  // Commit current field
  ├─ FPDF_FFLFillField(form_handle, page, annot, value)  // Set field value
  ├─ FPDF_FFLGetFieldValue(form_handle, page, annot, buffer, buflen)  // Get value
  └─ FPDF_ANNOT_XFA_WIDGET for XFA forms (requires pdf_enable_xfa=true)
```

---

## 11. Page Manipulation

### 11.1 Insert Page

```cpp
// Insert blank page
FPDF_PAGE new_page = FPDFPage_New(doc, page_index, width_pts, height_pts);
FPDF_ClosePage(new_page);
```

### 11.2 Delete Page

```cpp
FPDFPage_Delete(doc, page_index);
// Note: Invalidates all loaded page handles — must reload after delete
```

### 11.3 Rotate Page

```cpp
FPDFPage_SetRotation(page, rotation);
// rotation: 0 (normal), 1 (90° CW), 2 (180°), 3 (270° CW)
```

### 11.4 Reorder Pages

```cpp
// Create new document, import pages in desired order
FPDF_DOCUMENT new_doc = FPDF_CreateNewDocument();
int order[] = {3, 1, 4, 0, 2};  // New page order
for (int i = 0; i < 5; i++) {
    FPDF_ImportPagesByIndex(new_doc, old_doc, &order[i], 1, i);
}
```

### 11.5 Merge Documents

```cpp
FPDF_ImportPages(dest_doc, source_doc, "1-5,8,10-12", dest_page_index);
// Page range syntax: "1-5" = pages 1-5, "1,3,5" = specific pages
```

### 11.6 Extract Pages

```cpp
// Same as merge but with new empty destination document
FPDF_DOCUMENT new_doc = FPDF_CreateNewDocument();
FPDF_ImportPagesByIndex(new_doc, old_doc, indices, count, 0);
```

---

## 12. Save / Incremental Save Strategy

### 12.1 Full Save

```cpp
// Writes complete document (rebuilds cross-reference table)
FPDF_SaveAsDocument(doc, filepath, FPDF_SAVE_NO_LINEARIZATION);
// Result: Complete, compact PDF
// Use case: Save As, after major structural changes
```

### 12.2 Incremental Save

```cpp
// Appends changes only (fast, preserves original structure)
FPDF_SaveAsDocument(doc, filepath, FPDF_SAVE_FLAG_INCREMENTAL);
// Result: Original + appended change set
// Use case: Auto-save, annotation changes, minor edits
```

**RECOMMENDATION:** Use incremental save for auto-save and minor changes (annotations, rotation). Use full save for structural changes (merge, split, delete pages) and "Save As". Consider running full save in background after N incremental saves to prevent unbounded file growth.

### 12.3 Save Strategy Matrix

| Trigger | Save Type | Rationale |
|---------|-----------|-----------|
| User Ctrl+S | Full save | User expects clean file |
| Auto-save (every 30s) | Incremental | Fast, non-disruptive |
| Annotation added/modified | Incremental | Preserve annotation history |
| Page deleted/inserted | Full save | Structural change, incremental can be large |
| Merge operation | Full save | New document anyway |
| Save As | Full save | New file, clean output |