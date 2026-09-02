# PDFium Usage Analysis — Current vs. Native Rewrite

> Analysis of how PDFium is currently used in PDF Elite (two independent layers) and how it maps to the native C++/Win32 rewrite.

---

## 1. Current PDFium Architecture

PDF Elite currently uses PDFium in **two completely independent layers** with zero code sharing:

```
┌──────────────────────────────────────────────────────────────────────┐
│                       PDF Elite v2.14.2                             │
├──────────────────────────┬───────────────────────────────────────────┤
│   LAYER 1: FRONTEND     │   LAYER 2: BACKEND                        │
│   PDFium WASM           │   JPDFium 1.0.2 (private)                │
│                          │                                          │
│   @embedpdf/core ^2.14.4 │   com.stirling:jpdfium:1.0.2            │
│   @embedpdf/engines      │   (Maven artifact, no public source)     │
│   (pdfium-wasm)          │                                          │
│                          │   Java JNI wrapper                       │
│   Runs in WebView        │   Runs in JVM                            │
│   (Chrome/V8 sandbox)    │   (Full native access)                   │
│                          │                                          │
│   Primary use:           │   Primary use:                           │
│   Rendering, annotations,│   Merge, backend render (opaque)         │
│   text, search, nav      │                                          │
└──────────────────────────┴───────────────────────────────────────────┘
         │                                    │
         └──────── Tauri IPC Bridge ──────────┘
```

**FACT:** The two PDFium instances never share state. The frontend renders a WASM copy of the PDF; the backend operates on a JVM-loaded copy. Changes on one side require file transfer to the other.

---

## 2. Layer 1: Frontend — PDFium WASM via EmbedPDF

### 2.1 Package Dependencies

| Package | Version | Purpose |
|---------|---------|---------|
| `@embedpdf/core` | ^2.14.4 | Plugin architecture, document management, viewport math |
| `@embedpdf/engines` | latest | PDFium WASM engine bindings |

### 2.2 EmbedPDF Plugin Architecture

EmbedPDF provides a plugin system that wraps PDFium WASM. Each plugin manages a specific capability:

| Plugin | File Location | PDFium APIs Used | Description |
|--------|--------------|------------------|-------------|
| **DocumentManagerPlugin** | `src/lib/@embedpdf/core/src/DocumentManagerPlugin.ts` | `FPDF_LoadDocument`, `FPDF_CloseDocument` | Document lifecycle, multi-document management |
| **ViewportPlugin** | `src/lib/@embedpdf/core/src/ViewportPlugin.ts` | Indirect (coordinates rendering) | Computes visible area, page layout, scroll position |
| **ScrollPlugin** | `src/lib/@embedpdf/core/src/ScrollPlugin.ts` | Indirect | Virtual scrolling, page-boundary snapping |
| **RenderPlugin** | `src/lib/@embedpdf/core/src/RenderPlugin.ts` | `FPDF_RenderPageBitmap` (WASM) | Dispatches tile render requests to PDFium |
| **TilingPlugin** | `src/lib/@embedpdf/engines/pdfium-wasm/src/TilingPlugin.ts` | `FPDF_RenderPageBitmap` | 768px tile grid computation, overlap handling |
| **ZoomPlugin** | `src/lib/@embedpdf/core/src/ZoomPlugin.ts` | Indirect | Zoom level management, fit-page/width calculations |
| **InteractionManagerPlugin** | `src/lib/@embedpdf/core/src/InteractionManagerPlugin.ts` | Indirect | Routes mouse/touch events to appropriate handlers |
| **SelectionPlugin** | `src/lib/@embedpdf/core/src/SelectionPlugin.ts` | `FPDFText_GetCharBox`, `FPDFText_GetText`, `FPDFText_GetCharIndexAtPoint` | Text selection, multi-page selection, copy |
| **HistoryPlugin** | `src/lib/@embedpdf/core/src/HistoryPlugin.ts` | None | Undo/redo for document modifications |
| **AnnotationPlugin** | `src/lib/@embedpdf/core/src/AnnotationPlugin.ts` | `FPDFPage_GetAnnot`, `FPDFPage_CreateAnnot`, `FPDFAnnot_Set*`, `FPDFAnnot_Get*` | All annotation CRUD for 12 annotation types |
| **RedactionPlugin** | `src/lib/@embedpdf/core/src/RedactionPlugin.ts` | `FPDFAnnot_SetRect` (mark creation) | Redaction mark creation; burn-in unclear |
| **PanPlugin** | `src/lib/@embedpdf/core/src/PanPlugin.ts` | None | Panning gesture handling |
| **SpreadPlugin** | `src/lib/@embedpdf/core/src/SpreadPlugin.ts` | Indirect | Two-page spread layout computation |
| **SearchPlugin** | `src/lib/@embedpdf/core/src/SearchPlugin.ts` | `FPDFText_FindStart`, `FPDFText_FindNext`, `FPDFText_GetCharBox` | Find-in-document, result highlighting |
| **ThumbnailPlugin** | `src/lib/@embedpdf/core/src/ThumbnailPlugin.ts` | NOT PDFium — uses PDF.js | Thumbnail generation via PDF.js Web Worker |
| **BookmarkPlugin** | `src/lib/@embedpdf/core/src/BookmarkPlugin.ts` | `FPDFBookmark_GetFirstChild`, `FPDFBookmark_GetNextSibling`, `FPDFBookmark_GetTitle` | Bookmark tree traversal and navigation |
| **AttachmentPlugin** | `src/lib/@embedpdf/core/src/AttachmentPlugin.ts` | `FPDFDoc_GetAttachmentCount`, `FPDFDoc_GetAttachment`, `FPDFAttachment_GetName` | Embedded file listing |
| **RotatePlugin** | `src/lib/@embedpdf/core/src/RotatePlugin.ts` | Indirect (visual only; backend handles persistent rotation) | Visual rotation preview via CSS transform |
| **ExportPlugin** | `src/lib/@embedpdf/core/src/ExportPlugin.ts` | `FPDF_SaveAsDocument` (WASM) | Save/download document |
| **PrintPlugin** | `src/lib/@embedpdf/core/src/PrintPlugin.ts` | Indirect (renders via PDFium, prints via browser) | Print to system printer |

### 2.3 Tile Rendering Implementation

**FACT:** Frontend uses 768px tiles with 5px overlap and 1 extra ring of pre-rendering.

```
Tile Grid Parameters:
  TILE_SIZE = 768 pixels
  TILE_OVERLAP = 5 pixels
  PREFETCH_RINGS = 1 extra ring beyond visible

Rendering Flow:
  1. ViewportPlugin computes visible page region in device pixels
  2. TilingPlugin decomposes visible region into tile coordinates
  3. Prefetch adds 1 ring of tiles around visible set
  4. requestIdleCallback schedules non-urgent tile renders
  5. RenderPlugin calls FPDF_RenderPageBitmap (WASM) for each tile
  6. Canvas 2D blits tiles into visible area
  7. Tile cache holds rendered bitmaps keyed by (page, tile_col, tile_row, zoom_level)
```

**FACT:** WASM precompilation uses `requestIdleCallback` to render prefetch tiles during browser idle time. This improves perceived performance but means tiles may not be ready on rapid scroll.

**Current Limitations:**
- Fixed 768px tile size — not adaptive to DPI or content complexity
- WASM rendering is CPU-only (no WebGL acceleration for PDFium WASM)
- Tile memory not bounded — can exhaust WASM linear memory (~4GB max)
- No tile eviction under memory pressure

### 2.4 PDF.js as Secondary Engine

**FACT:** PDF.js is used for thumbnails ONLY, NOT for main rendering.

| Component | Engine | Reason |
|-----------|--------|--------|
| Main document rendering | PDFium WASM | Higher fidelity, full annotation support |
| Page thumbnails | PDF.js (`PDFWorkerManager`) | Web Worker separation, sufficient for low-res |
| PDF metadata extraction | PDF.js | Already loaded for thumbnails, convenient |

**File:** `src/lib/@embedpdf/core/src/PDFWorkerManager.ts`

**PROBLEM:** Using two PDF engines (PDFium + PDF.js) means:
1. Two copies of the PDF in memory (WASM heap + JS heap)
2. Potential rendering inconsistencies between thumbnails and main view
3. Double the code complexity for no clear benefit

**RECOMMENDATION:** In the native rewrite, use a SINGLE PDFium engine for both main rendering and thumbnails. PDF.js can be completely eliminated. Thumbnails are simply `FPDF_RenderPageBitmap` at a lower scale factor (e.g., 150 DPI vs 150+ DPI for main view).

---

## 3. Layer 2: Backend — JPDFium 1.0.2 (Opaque)

### 3.1 What We Know

| Attribute | Detail |
|-----------|--------|
| **Artifact** | `com.stirling:jpdfium:1.0.2` |
| **Repository** | Private Maven repository (not public) |
| **Source Code** | NOT available in the public repository |
| **Type** | Java JNI wrapper around native PDFium libraries |
| **Native Libraries** | Platform-specific `.dll` / `.so` / `.dylib` bundled per OS |
| **Licensing** | Unknown — private artifact, likely proprietary or custom-licensed |

**FACT:** No source code for JPDFium exists in the public repository. All Java classes that reference it show import statements but the actual JAR contents and JNI implementation are opaque.

### 3.2 Where JPDFium Is Referenced

JPDFium appears in the build configuration but its actual usage locations in Java code are not visible in the public repo. Based on the class names and controller endpoints, inferred usage:

| Controller | Likely Uses JPDFium? | Evidence |
|-----------|---------------------|----------|
| `MergeController` | POSSIBLY | Merge might use JPDFium for page import (faster than PDFBox) |
| `EditTextController` | NO | Uses PDFBox + Ghostscript via `PdfJsonConversionService` |
| `SplitPDFController` | NO | Uses PDFBox for page extraction |
| `RotationController` | NO | Uses PDFBox for page rotation |
| `AddCommentsController` | NO | Uses PDFBox for annotation writing |
| `GetInfoOnPDF` | NO | Uses PDFBox for metadata |
| Other controllers | NO | All use PDFBox |

**ASSUMPTION:** JPDFium may have been intended for rendering PDF previews on the backend (e.g., for merge preview thumbnails) but most backend operations use PDFBox exclusively. The JPDFium dependency may be legacy or for a proprietary-only feature not in the public codebase.

### 3.3 CustomPDFDocumentFactory — 3-Tier Memory Strategy

**File:** `src/main/java/com/stirling/pdf/service/PDFTools/CustomPDFDocumentFactory.java`

This factory manages PDFBox `PDDocument` instances with a 3-tier memory strategy:

```
Tier 1: In-Memory Cache
  - Recently opened documents held in memory
  - LRU eviction based on memory pressure
  - Fastest access for repeated operations

Tier 2: Memory-Mapped Files
  - Large documents loaded via memory-mapped I/O
  - Avoids full document deserialization
  - Slower than Tier 1 but lower memory footprint

Tier 3: Stream-Based Loading
  - Largest or least-recently-used documents
  - Sequential access via InputStream
  - Slowest but lowest memory usage
```

**RECOMMENDATION for Native Rewrite:** PDFium's native C API doesn't need this complexity:
- `FPDF_LoadDocument` already supports both file-path and memory-buffer loading
- PDFium handles its own internal memory management
- For the native rewrite, a simple `std::unordered_map<DocId, PdfDocument>` with LRU eviction is sufficient
- Use `FPDF_LoadMemDocument` for in-memory loads, `FPDF_LoadDocument` (file path) for direct file access
- No need for memory-mapped file tier — PDFium's internal parser is optimized for both modes

---

## 4. PDFium Capability Mapping for Native Rewrite

### 4.1 What PDFium CAN Realistically Handle

| Category | Operations | Confidence |
|----------|-----------|------------|
| **Rendering** | Page render, tile render, thumbnail render, print to DC | FACT — proven in WASM layer |
| **Text Extraction** | Full text, per-character bounding boxes, font info | FACT — SelectionPlugin uses this |
| **Text Search** | Find next/prev, case-sensitive, whole-word | FACT — SearchPlugin uses this |
| **Text Selection** | Hit testing, multi-page selection, copy | FACT — SelectionPlugin uses this |
| **Annotations (Read)** | All 12 types in EmbedPDF: Highlight, Underline, StrikeOut, Ink, StickyNote, FreeText, Square, Circle, Line, Polygon, Polyline, Link | FACT — AnnotationPlugin reads all these |
| **Annotations (Write)** | Create, modify, delete annotations; update appearance | FACT — AnnotationPlugin writes all these |
| **Page Operations** | Insert blank, delete, rotate, reorder, extract, merge, split | FACT — PDFium C API has all these |
| **Bookmarks** | Read outline tree, navigate to bookmark target | FACT — BookmarkPlugin uses this |
| **Metadata** | Title, Author, Subject, Keywords, Creator, Producer | FACT — standard PDFium API |
| **Attachments** | List, read, add, remove embedded files | FACT — AttachmentPlugin reads; write APIs exist |
| **Links** | Detect links at point, follow link actions | FACT — InteractionManagerPlugin uses this |
| **Forms** | Read/write form field values, widget rendering | FACT — PDFium has full AcroForm support |
| **Image Objects** | Create, replace, remove, extract images from pages | FACT — PDFium page object APIs support this |
| **Text Objects** | Create text objects (for page numbers, free text) | FACT — PDFium has `FPDFPageObj_CreateTextObj` |
| **Save** | Full save, incremental save | FACT — ExportPlugin uses WASM save |

### 4.2 What PDFium CANNOT Handle (Requires External Tools)

| Capability | Why PDFium Can't | Alternative for Native Rewrite |
|-----------|-----------------|------------------------------|
| **OCR** (text recognition from images) | No OCR engine built in | Tesseract C API (`tesseract.dll`) — only if OCR is added later |
| **Format Conversion** (DOCX, XLSX, PPTX → PDF) | No format parsers | LibreOffice CLI or dedicated libraries — not in trimmed set |
| **JavaScript Execution** | V8 integration disabled for size | Not needed — not in feature set |
| **Certificate-Based Digital Signatures** | No PKCS#11/Windows cert store integration | Windows CryptoAPI + custom PDF signature generation — not in trimmed set |
| **Redaction Burn-In** (secure content removal) | `FPDFAnnot_ApplyApplier` exists for redaction but may not guarantee content removal at the object level | Custom pass: scan page objects under redaction rect, remove text/path objects, then flatten. Or use `FPDFAnnot_ApplyApplier` and verify with content stream inspection. |
| **Font Normalization** (subset embedding, name standardization) | PDFium can embed fonts but doesn't normalize font names like Ghostscript | Direct font API usage (Windows GDI font enumeration) — Ghostscript NOT needed for basic text editing |
| **AI Features** (summarization, Q&A, translation) | Not a PDF concern | Direct LLM API calls from C++ (not PDF engine responsibility) |
| **PDF/A Compliance** | No validation/conversion | veraPDF for validation, custom conversion if needed — not in trimmed set |

---

## 5. Migration of Each EmbedPDF Plugin to Native PDFium

| EmbedPDF Plugin | WASM PDFium API | Native PDFium C API | Complexity | Notes |
|----------------|----------------|-------------------|------------|-------|
| DocumentManagerPlugin | `FPDF_LoadDocument` (WASM) | `FPDF_LoadDocument` | LOW | Direct mapping |
| ViewportPlugin | Pure JS math | Pure C++ math | LOW | No PDFium dependency |
| ScrollPlugin | Pure JS math | Win32 scroll messages | LOW | Reimplement with Win32 scroll bars |
| RenderPlugin | `FPDF_RenderPageBitmap` (WASM) | `FPDF_RenderPageBitmap` | MEDIUM | Add worker thread dispatch, D2D blit |
| TilingPlugin | Tile grid math + render | Same math + render to HBITMAP | MEDIUM | Tile cache needs Win32 reimplementation |
| ZoomPlugin | CSS transform + scale factor | D2D transform + render scale | LOW | Simpler without CSS |
| InteractionManagerPlugin | DOM event routing | Win32 message routing | MEDIUM | Different event model |
| SelectionPlugin | `FPDFText_*` (WASM) | `FPDFText_*` | MEDIUM | Hit testing + rendering selection overlay |
| HistoryPlugin | JS state stack | C++ undo/redo stack | LOW | Same concept, different language |
| AnnotationPlugin | `FPDFPage_*Annot*`, `FPDFAnnot_*` | Same C APIs | MEDIUM | 12 annotation types, D2D overlay rendering |
| RedactionPlugin | `FPDFAnnot_SetRect` | `FPDFAnnot_SetRect` + burn-in | HIGH | Burn-in requires custom implementation |
| PanPlugin | Touch/mouse gesture | Win32 mouse messages | LOW | Standard Win32 input handling |
| SpreadPlugin | CSS layout | Custom layout engine | MEDIUM | Page pairing logic with D2D |
| SearchPlugin | `FPDFText_FindStart/Next` | Same C APIs | LOW | Direct mapping, D2D highlight rects |
| ThumbnailPlugin | **PDF.js** (NOT PDFium) | `FPDF_RenderPageBitmap` (low scale) | LOW | Eliminate PDF.js entirely |
| BookmarkPlugin | `FPDFBookmark_*` | Same C APIs | LOW | Direct mapping, Win32 TreeView |
| AttachmentPlugin | `FPDFDoc_GetAttachment*` | Same C APIs | LOW | Direct mapping, Win32 ListView |
| RotatePlugin | CSS transform (visual) | `FPDFPage_SetRotation` (persistent) | LOW | Simpler — single code path |
| ExportPlugin | `FPDF_SaveAsDocument` (WASM) | `FPDF_SaveAsDocument` | LOW | Direct mapping, file dialog |
| PrintPlugin | Browser `window.print()` | `FPDF_RenderPageBitmapInDC` or `ShellExecute` | MEDIUM | Native print is more capable |

---

## 6. Eliminating Dual-Engine Complexity

### 6.1 Current Problems

| Problem | Description | Impact |
|---------|-------------|--------|
| **Dual PDFium instances** | WASM PDFium + JPDFium, no shared state | File must be transferred between layers for any backend operation |
| **PDF.js alongside PDFium** | PDF.js for thumbnails, PDFium for rendering | Two engines, two memory copies, visual inconsistency |
| **WASM memory ceiling** | ~4GB linear memory limit | Large documents or many tabs can OOM |
| **No GPU rendering** | PDFium WASM renders to CPU Canvas 2D | Slower than native GPU-accelerated rendering |
| **File roundtrip for edits** | Frontend edits → serialize → Tauri IPC → backend → serialize → response | Adds latency, complexity, and potential data loss |

### 6.2 Native Rewrite Benefits

| Improvement | Description |
|-------------|-------------|
| **Single PDFium instance** | One `FPDF_DOCUMENT` per open tab, no duplication |
| **No PDF.js** | PDFium handles all rendering including thumbnails |
| **No memory ceiling** | Native 64-bit process, virtual memory limited only by system RAM |
| **GPU rendering path** | PDFium renders to CPU bitmap → D2D composites on GPU |
| **No IPC roundtrip** | All operations in-process, no serialization needed |
| **Direct file system access** | No Tauri file system bridge needed |
| **Smaller binary** | No WASM runtime, no V8/JS engine, no Node.js runtime |

---

## 7. PDFium Version Considerations

### 7.1 Current Version (EmbedPDF)

**FACT:** EmbedPDF bundles a specific PDFium WASM build. The exact PDFium Chromium commit is not easily determinable from the NPM package. EmbedPDF v2.14.4 likely tracks a recent Chromium PDFium (~Chromium 120+).

### 7.2 Recommended Version for Native Rewrite

**RECOMMENDATION:** Build PDFium from the latest stable Chromium branch. This provides:
- Latest security fixes
- Best PDF spec compliance (PDF 2.0 improvements)
- Best performance optimizations
- Active maintenance

**RECOMMENDATION:** Pin to a specific Chromium commit for reproducible builds. Update quarterly.

### 7.3 API Compatibility

**FACT:** PDFium's C API is remarkably stable. New APIs are added but existing APIs are almost never removed or changed. Code written for PDFium from Chromium 100 will work with Chromium 130+ without modification.

---

## 8. Summary: What Changes in the Native Rewrite

| Aspect | Current (WASM + JPDFium) | Native Rewrite |
|--------|--------------------------|---------------|
| PDFium instances | 2 (WASM + JPDFium) | 1 (native DLL) |
| PDF engines total | 3 (PDFium WASM, JPDFium, PDF.js) | 1 (PDFium DLL) |
| Memory model | WASM 4GB ceiling + JVM heap | Native 64-bit, no ceiling |
| Rendering | CPU Canvas 2D (browser) | CPU → D2D GPU compositing |
| Threading | Single-threaded JS + WASM | Multi-threaded with per-doc locks |
| File I/O | Tauri bridge (async IPC) | Direct Win32 file API |
| Thumbnail engine | PDF.js Web Worker | PDFium (same engine, lower scale) |
| Annotation rendering | SVG overlay on Canvas | D2D overlay on D2D surface |
| Save | WASM `FPDF_SaveAsDocument` → download | Direct `FPDF_SaveAsDocument` → file |
| Print | Browser `window.print()` | Native print DC or ShellExecute |
| External dependencies | Tauri, Node.js, Rust, Java, Spring Boot, PDFBox, Ghostscript | PDFium DLL only (for trimmed feature set) |

**FACT:** The native rewrite with a single PDFium DLL eliminates all middleware (Tauri, Rust, Java, Spring Boot, PDFBox) and secondary engines (PDF.js, JPDFium) for the trimmed 17-tool feature set.
