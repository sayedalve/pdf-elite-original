# Feature Inventory — PDF Elite v2.14.2

> Complete inventory of every existing feature, its implementation, and migration guidance for the native C++/Win32/PDFium rewrite.

**Status Legend:**
| Status | Meaning |
|--------|---------|
| **REAL** | Fully functional, tested, production-ready |
| **PARTIAL** | Functional but incomplete, edge cases fail, or degraded |
| **VISUAL** | UI exists but the feature is a stub or non-functional |
| **MISSING** | Not implemented at all |

**Migration Difficulty Legend:**
| Level | Meaning |
|-------|---------|
| **LOW** | Direct PDFium C API mapping, straightforward |
| **MEDIUM** | Requires custom logic but well-understood |
| **HIGH** | Complex logic, external dependencies, or significant re-architecture |
| **CRITICAL** | Fundamental architectural challenge, may need alternative approach |

---

## 1. Document Viewing & Navigation

### 1.1 PDF Rendering

| Attribute | Detail |
|-----------|--------|
| **Name** | Core PDF Rendering |
| **Status** | REAL |
| **Frontend Location** | `src/lib/@embedpdf/core/`, `src/lib/@embedpdf/engines/pdfium-wasm/` |
| **Key Classes** | `PdfiumEngine`, `DocumentManagerPlugin`, `RenderPlugin`, `ViewportPlugin` |
| **Architecture** | PDFium WASM → Canvas 2D tile rendering → DOM display |
| **PDFium Involvement** | PRIMARY — all rendering via PDFium WASM |
| **Stirling PDF Involvement** | None |
| **Java Involvement** | None |
| **WebView/Tauri Involvement** | Tauri provides window; rendering is pure WebView Canvas |
| **Current Limitations** | WASM memory ceiling (~4GB), no GPU acceleration, tile seam artifacts at extreme zoom |
| **Bugs/Problems** | Occasional rendering glitches with complex gradients; text layer misalignment on some CJK fonts |
| **C++ Implementation** | Native PDFium → `FPDF_RenderPageBitmap` → HBITMAP/D2D surface |
| **Reuse Potential** | HIGH — tile logic, viewport math, and render scheduling are reusable concepts |
| **Migration Difficulty** | MEDIUM |
| **Priority** | P0 — Core |

### 1.2 Tile Rendering System

| Attribute | Detail |
|-----------|--------|
| **Name** | Tile-Based Rendering |
| **Status** | REAL |
| **Frontend Location** | `src/lib/@embedpdf/engines/pdfium-wasm/src/TilingPlugin.ts` |
| **Key Classes** | `TilingPlugin`, `ViewportPlugin` |
| **Architecture** | 768px tiles with 5px overlap, 1 extra ring pre-rendering, Canvas 2D blitting |
| **Dependencies** | Pdfium WASM engine, requestIdleCallback for precompilation |
| **PDFium Involvement** | PRIMARY — `FPDF_RenderPageBitmap` called per tile |
| **Current Limitations** | Fixed tile size not adaptive to DPI; overlap can cause seam artifacts with certain blend modes |
| **Bugs/Problems** | Memory pressure with many tiles at high zoom; no tile eviction under memory stress |
| **C++ Implementation** | PDFium → `HBITMAP` per tile → D2D/GDI cache → Direct2D composition |
| **Reuse Potential** | MEDIUM — viewport math and ring strategy are reusable; tile cache needs Win32 reimplementation |
| **Migration Difficulty** | MEDIUM |
| **Priority** | P0 — Core |

### 1.3 Thumbnail Sidebar

| Attribute | Detail |
|-----------|--------|
| **Name** | Page Thumbnail Panel |
| **Status** | REAL |
| **Frontend Location** | `src/components/Sidebar/Thumbnails.tsx`, `src/lib/@embedpdf/core/src/ThumbnailPlugin.ts` |
| **Key Classes** | `ThumbnailPlugin`, `PDFWorkerManager` (PDF.js) |
| **Architecture** | PDF.js renders low-res thumbnails (NOT PDFium) in Web Worker; lazy-loaded on scroll |
| **PDFium Involvement** | None — thumbnails use PDF.js exclusively |
| **Stirling PDF Involvement** | None |
| **Java Involvement** | None |
| **Current Limitations** | PDF.js rendering diverges slightly from PDFium main view; dual engine complexity |
| **Bugs/Problems** | Thumbnails can be stale after edits; no explicit thumbnail invalidation pipeline |
| **C++ Implementation** | Single PDFium engine renders both main view and thumbnails at different scale factors |
| **Reuse Potential** | LOW — PDF.js thumbnail rendering replaced entirely by PDFium |
| **Migration Difficulty** | LOW |
| **Priority** | P1 |

### 1.4 Zoom

| Attribute | Detail |
|-----------|--------|
| **Name** | Zoom In/Out/Fit Page/Fit Width |
| **Status** | REAL |
| **Frontend Location** | `src/lib/@embedpdf/core/src/ZoomPlugin.ts`, `src/components/Toolbar/ZoomControls.tsx` |
| **Key Classes** | `ZoomPlugin` |
| **Architecture** | CSS transform-based zoom with tile re-rendering on threshold changes |
| **Dependencies** | ViewportPlugin, TilingPlugin |
| **PDFium Involvement** | Indirect — triggers re-render via RenderPlugin |
| **Current Limitations** | Zoom levels snap to tile-aligned thresholds; smooth pinch-zoom on trackpad has jank |
| **Bugs/Problems** | "Fit page" can mis-calculate with non-standard page sizes |
| **C++ Implementation** | `FPDF_RenderPageBitmap` with computed scale; D2D transform for display |
| **Reuse Potential** | HIGH — zoom math, fit-page calculations, and preset levels map directly |
| **Migration Difficulty** | LOW |
| **Priority** | P0 |

### 1.5 Scroll

| Attribute | Detail |
|-----------|--------|
| **Name** | Scroll / Panning |
| **Status** | REAL |
| **Frontend Location** | `src/lib/@embedpdf/core/src/ScrollPlugin.ts` |
| **Key Classes** | `ScrollPlugin` |
| **Architecture** | Virtual scroll container with page boundary snapping |
| **PDFium Involvement** | Indirect — scroll position drives viewport calculations |
| **Current Limitations** | Large documents (>1000 pages) can have scroll position drift |
| **Bugs/Problems** | Minor jank on rapid scroll with heavy tile re-rendering |
| **C++ Implementation** | Win32 scroll bars + Direct2D viewport transform; tile prefetch based on scroll velocity |
| **Reuse Potential** | HIGH — scroll math and page-boundary logic are reusable |
| **Migration Difficulty** | LOW |
| **Priority** | P0 |

### 1.6 Page Navigation

| Attribute | Detail |
|-----------|--------|
| **Name** | Go to Page / Page Indicator |
| **Status** | REAL |
| **Frontend Location** | `src/components/Toolbar/PageNavigation.tsx` |
| **Key Classes** | PageNavigation component, ViewportPlugin |
| **Architecture** | Text input + stepper for page number; ViewportPlugin scrolls to page |
| **PDFium Involvement** | None — pure UI/navigation |
| **C++ Implementation** | Win32 edit control + scrollbar positioning |
| **Reuse Potential** | HIGH |
| **Migration Difficulty** | LOW |
| **Priority** | P1 |

### 1.7 Spread View

| Attribute | Detail |
|-----------|--------|
| **Name** | Two-Page Spread View |
| **Status** | PARTIAL |
| **Frontend Location** | `src/lib/@embedpdf/core/src/SpreadPlugin.ts` |
| **Key Classes** | `SpreadPlugin` |
| **Architecture** | CSS flex layout grouping pages in pairs; odd-first-page offset support |
| **PDFium Involvement** | Indirect — affects page layout, not rendering |
| **Current Limitations** | Cover page handling inconsistent; doesn't respect PDF page layout hints |
| **Bugs/Problems** | Can display wrong page pairing for documents with different page sizes |
| **C++ Implementation** | Layout engine groups pages in pairs; D2D renders side-by-side |
| **Reuse Potential** | MEDIUM |
| **Migration Difficulty** | MEDIUM |
| **Priority** | P2 |

---

## 2. Text Interaction

### 2.1 Text Selection

| Attribute | Detail |
|-----------|--------|
| **Name** | Text Selection (Click & Drag) |
| **Status** | REAL |
| **Frontend Location** | `src/lib/@embedpdf/core/src/SelectionPlugin.ts` |
| **Key Classes** | `SelectionPlugin` |
| **Architecture** | Text layer overlay (invisible text spans) captures mouse events; PDFium WASM extracts text positions |
| **Dependencies** | PDFium WASM `FPDFText_GetCharBox`, `FPDFText_GetText` |
| **PDFium Involvement** | PRIMARY — text position and content extraction |
| **Current Limitations** | Text layer can desync from render layer after zoom/scroll; CJK selection imprecise |
| **Bugs/Problems** | Multi-page selection has edge cases; selection across rotated pages broken |
| **C++ Implementation** | `FPDFText_GetCharBox` → hit testing → `FPDFText_GetText` for extraction → clipboard |
| **Reuse Potential** | HIGH — text hit-testing logic directly applicable |
| **Migration Difficulty** | MEDIUM |
| **Priority** | P0 |

### 2.2 Copy/Paste

| Attribute | Detail |
|-----------|--------|
| **Name** | Copy Selected Text to Clipboard |
| **Status** | REAL |
| **Frontend Location** | `src/lib/@embedpdf/core/src/SelectionPlugin.ts` (clipboard integration) |
| **Key Classes** | `SelectionPlugin` + browser Clipboard API |
| **Architecture** | Selection → text extraction → `navigator.clipboard.writeText()` |
| **PDFium Involvement** | PRIMARY — provides the text content |
| **C++ Implementation** | `FPDFText_GetText` → `SetClipboardData(CF_UNICODETEXT, ...)` via Win32 |
| **Reuse Potential** | HIGH |
| **Migration Difficulty** | LOW |
| **Priority** | P0 |

### 2.3 Search

| Attribute | Detail |
|-----------|--------|
| **Name** | Find in Document (Ctrl+F) |
| **Status** | REAL |
| **Frontend Location** | `src/lib/@embedpdf/core/src/SearchPlugin.ts` |
| **Key Classes** | `SearchPlugin` |
| **Architecture** | `FPDFText_FindStart` / `FPDFText_FindNext` via WASM; highlights rendered on text layer overlay |
| **Dependencies** | PDFium WASM text search APIs |
| **PDFium Involvement** | PRIMARY — all text search via PDFium |
| **Current Limitations** | No regex support; whole-word/case-sensitive toggles present but slow on large docs |
| **Bugs/Problems** | Search results don't persist when switching between tabs |
| **C++ Implementation** | `FPDFText_FindStart` / `FPDFText_FindNext` → highlight rectangles via `FPDFText_GetCharBox` |
| **Reuse Potential** | HIGH — search logic and highlight rendering map directly |
| **Migration Difficulty** | LOW |
| **Priority** | P0 |

### 2.4 Text Editing (Find & Replace)

| Attribute | Detail |
|-----------|--------|
| **Name** | PDF Text Editor (Find/Replace via JSON roundtrip) |
| **Status** | REAL |
| **Frontend Location** | `src/components/Tools/PDFTextEditor/` |
| **Backend Location** | `src/main/java/com/stirling/pdf/controller/EditTextController.java` |
| **Key Classes** | `EditTextController`, `PdfJsonConversionService` (~2000 lines) |
| **Architecture** | Frontend selects text → sends to Java backend → PDFBox extracts full doc as structured JSON (with Ghostscript font normalization) → modifies JSON → PDFBox rebuilds PDF → returns to frontend |
| **Dependencies** | PDFBox 3.0.7, Ghostscript (font normalization), Spring Boot HTTP API |
| **PDFium Involvement** | None — purely PDFBox + Ghostscript pipeline |
| **Stirling PDF Involvement** | HIGH — core Stirling PDF feature, heavily modified |
| **Java Involvement** | CRITICAL — `PdfJsonConversionService` is ~2000 lines of complex JSON↔PDF conversion |
| **Current Limitations** | Slow roundtrip (full doc serialization); loses some PDF features (complex layouts, embedded fonts can corrupt); doesn't handle form fields or annotations embedded in text |
| **Bugs/Problems** | CJK text replacement frequently corrupts font mappings; large documents (>50 pages) can timeout |
| **C++ Implementation** | RECOMMENDATION: Implement direct text editing via PDFium's `FPDF_PAGEOBJ_TEXT_SetText`, `FPDFText_SetText` for simple cases. For complex layout preservation, use a custom text editing layer that reads via `FPDFText_GetText` and writes via PDFium page object manipulation. Ghostscript dependency can be eliminated with proper font handling. |
| **Reuse Potential** | LOW — the JSON roundtrip architecture should not be replicated; concept of find/replace is reusable |
| **Migration Difficulty** | HIGH — current approach is architecturally wrong for native; needs full reimplementation |
| **Priority** | P1 |

---

## 3. Annotation System

### 3.1 Annotation Types Overview

| Annotation Type | Status | Frontend Location | PDFium API | C++ Mapping |
|----------------|--------|-------------------|------------|-------------|
| Highlight | REAL | `native/src/pdf_engine/` | `FPDFAnnot_SetColor`, `FPDFAnnot_GetRect` | `FPDF_ANNOT_HIGHLIGHT` |
| Underline | REAL | `native/src/pdf_engine/` | Same as Highlight | `FPDF_ANNOT_UNDERLINE` |
| StrikeOut | REAL | `native/src/pdf_engine/` | Same as Highlight | `FPDF_ANNOT_STRIKEOUT` |
| Ink (Freehand Draw) | REAL | `native/src/pdf_engine/` | `FPDFAnnot_SetInkListPath` | `FPDF_ANNOT_INK` |
| StickyNote (Text) | REAL | `native/src/pdf_engine/` | `FPDFAnnot_SetStringValue` | `FPDF_ANNOT_TEXT` |
| FreeText (Typewriter) | REAL | `native/src/pdf_engine/` | `FPDFPageObj_CreateTextObj` | `FPDF_ANNOT_FREETEXT` |
| Square | REAL | `native/src/pdf_engine/` | `FPDFAnnot_SetRect` | `FPDF_ANNOT_SQUARE` |
| Circle | REAL | `native/src/pdf_engine/` | `FPDFAnnot_SetRect` | `FPDF_ANNOT_CIRCLE` |
| Line | REAL | `native/src/pdf_engine/` | `FPDFAnnot_SetLine` | `FPDF_ANNOT_LINE` |
| Polygon | MISSING | `N/A` | `FPDFAnnot_SetVertices` | `FPDF_ANNOT_POLYGON` |
| Polyline | MISSING | `N/A` | `FPDFAnnot_SetVertices` | `FPDF_ANNOT_POLYLINE` |
| Link | REAL | `AnnotationPlugin.ts` | `FPDFAnnot_SetAction` | `FPDF_ANNOT_LINK` |

### 3.2 Annotation Rendering & Editing

| Attribute | Detail |
|-----------|--------|
| **Name** | Annotation CRUD Operations |
| **Status** | REAL |
| **Frontend Location** | `src/lib/@embedpdf/core/src/AnnotationPlugin.ts`, `src/components/Tools/Annotate/` |
| **Key Classes** | `AnnotationPlugin`, `InteractionManagerPlugin` |
| **Architecture** | Annotations rendered as overlay SVGs on top of PDF canvas; edits stored in EmbedPDF state, synced to PDFium WASM |
| **PDFium Involvement** | PRIMARY — all annotation read/write via PDFium WASM APIs |
| **Stirling PDF Involvement** | Minimal — `AddCommentsController` exists but frontend does annotations locally |
| **Java Involvement** | Minimal |
| **Current Limitations** | Annotation pop-up appearance varies from Acrobat; some annotation properties not editable (border style, dash pattern) |
| **Bugs/Problems** | Ink annotation pressure sensitivity not preserved; annotation z-order issues with overlapping types |
| **C++ Implementation** | Direct PDFium C API: `FPDFPage_CreateAnnot`, `FPDFAnnot_Set*`, `FPDFAnnot_Get*`; render as overlay on D2D surface |
| **Reuse Potential** | HIGH — annotation types and UI paradigms map 1:1 to PDFium C API |
| **Migration Difficulty** | MEDIUM — SVG overlay approach changes to D2D overlay; data model similar |
| **Priority** | P0 |

### 3.3 Redaction

| Attribute | Detail |
|-----------|--------|
| **Name** | Redaction (Mark & Apply) |
| **Status** | PARTIAL |
| **Frontend Location** | `src/lib/@embedpdf/core/src/RedactionPlugin.ts` |
| **Key Classes** | `RedactionPlugin` |
| **Architecture** | Redaction marks rendered as overlay; "Apply" sends to backend for burn-in |
| **PDFium Involvement** | PARTIAL — PDFium can create redaction annotations but burn-in requires content removal + re-render |
| **Stirling PDF Involvement** | None visible in trimmed controllers |
| **Current Limitations** | Redaction marks are visual only — actual content removal (burn-in) may not be fully functional |
| **Bugs/Problems** | Burn-in may leave recoverable text under redaction marks if not implemented correctly |
| **C++ Implementation** | `FPDFAnnot_SetRect` for marks; burn-in requires: remove text objects under rect via `FPDFPageObj_GetText`, then `FPDF_RenderPage` to flatten, then `FPDF_ANNOT_REDACT` with `FPDFAnnot_AppendApplier`. If PDFium's redaction applier is insufficient, use a custom content-removal pass before flattening. |
| **Reuse Potential** | MEDIUM — UI concepts reusable; burn-in needs careful reimplementation for security |
| **Migration Difficulty** | HIGH — burn-in must be cryptographically complete, no text leakage |
| **Priority** | P2 |

---

## 4. Page Manipulation

### 4.1 Merge PDFs

| Attribute | Detail |
|-----------|--------|
| **Name** | Merge Multiple PDFs |
| **Status** | REAL |
| **Frontend Location** | `src/components/Tools/Merge/` |
| **Backend Location** | `src/main/java/com/stirling/pdf/controller/MergeController.java` |
| **Key Classes** | `MergeController`, PDFBox `PDFMergerUtility` |
| **Architecture** | Frontend collects files → uploads to Spring Boot → PDFBox merges → returns result |
| **PDFium Involvement** | None currently — PDFBox handles merge on backend |
| **Stirling PDF Involvement** | HIGH — core Stirling PDF feature |
| **Java Involvement** | CRITICAL — PDFBox merge utility |
| **Current Limitations** | Large file uploads over HTTP; no streaming merge |
| **Bugs/Problems** | Bookmark conflicts on merge; form field name collisions |
| **C++ Implementation** | `FPDF_ImportPages` / `FPDF_ImportPagesByIndex` — native PDFium merge, no HTTP roundtrip |
| **Reuse Potential** | MEDIUM — UI for file reordering reusable; merge logic replaced by PDFium |
| **Migration Difficulty** | LOW — PDFium merge is straightforward |
| **Priority** | P0 |

### 4.2 Split PDF

| Attribute | Detail |
|-----------|--------|
| **Name** | Split PDF (4 modes) |
| **Status** | REAL |
| **Frontend Location** | `src/components/Tools/Split/` |
| **Backend Location** | `src/main/java/com/stirling/pdf/controller/SplitPDFController.java` |
| **Key Classes** | `SplitPDFController` (4 endpoints: by ranges, by fixed count, by size, by every N pages) |
| **Architecture** | Frontend selects mode → uploads to Spring Boot → PDFBox splits → returns ZIP of results |
| **PDFium Involvement** | None currently — PDFBox handles split on backend |
| **Stirling PDF Involvement** | HIGH — core Stirling PDF feature |
| **Java Involvement** | CRITICAL |
| **C++ Implementation** | `FPDF_ImportPagesByIndex` to create sub-documents; `FPDF_SaveAsDocument` for each split |
| **Reuse Potential** | MEDIUM — UI for split mode selection reusable |
| **Migration Difficulty** | LOW |
| **Priority** | P0 |

### 4.3 Rotate Pages

| Attribute | Detail |
|-----------|--------|
| **Name** | Rotate Pages (Selected/All) |
| **Status** | REAL |
| **Frontend Location** | `src/lib/@embedpdf/core/src/RotatePlugin.ts`, `src/components/Tools/Rotate/` |
| **Backend Location** | `src/main/java/com/stirling/pdf/controller/RotationController.java` |
| **Key Classes** | `RotatePlugin` (visual), `RotationController` (persistent) |
| **Architecture** | Frontend rotates visually via CSS transform; backend persists rotation via PDFBox page rotation |
| **PDFium Involvement** | PARTIAL — visual rotation on frontend; backend uses PDFBox |
| **Stirling PDF Involvement** | MEDIUM |
| **C++ Implementation** | `FPDFPage_SetRotation` for both visual and persistent — single code path |
| **Reuse Potential** | HIGH |
| **Migration Difficulty** | LOW |
| **Priority** | P0 |

### 4.4 Auto-Rotate

| Attribute | Detail |
|-----------|--------|
| **Name** | Auto-Rotate Based on Text Orientation |
| **Status** | REAL |
| **Frontend Location** | `src/components/Tools/AutoRotate/` |
| **Backend Location** | `src/main/java/com/stirling/pdf/controller/AutoRotateController.java` |
| **Key Classes** | `AutoRotateController` |
| **Architecture** | Backend analyzes text orientation via PDFBox text extraction → applies rotation |
| **PDFium Involvement** | None currently — PDFBox text analysis |
| **C++ Implementation** | `FPDFText_GetCharBox` to analyze text angle per page → `FPDFPage_SetRotation` |
| **Reuse Potential** | MEDIUM — orientation analysis algorithm reusable |
| **Migration Difficulty** | MEDIUM |
| **Priority** | P2 |

### 4.5 Reorganize Pages

| Attribute | Detail |
|-----------|--------|
| **Name** | Drag-and-Drop Page Reordering |
| **Status** | REAL |
| **Frontend Location** | `src/components/Tools/ReorganizePages/` |
| **Backend Location** | `src/main/java/com/stirling/pdf/controller/RearrangePagesPDFController.java` |
| **Key Classes** | `RearrangePagesPDFController` |
| **Architecture** | Frontend shows thumbnail grid with drag-drop → sends new order to backend → PDFBox rebuilds |
| **PDFium Involvement** | None currently — PDFBox page reordering on backend |
| **C++ Implementation** | `FPDF_ImportPagesByIndex` in new order → new document → save |
| **Reuse Potential** | HIGH — drag-drop UI concept fully reusable |
| **Migration Difficulty** | LOW |
| **Priority** | P0 |

### 4.6 Extract Pages

| Attribute | Detail |
|-----------|--------|
| **Name** | Extract Selected Pages to New PDF |
| **Status** | REAL |
| **Frontend Location** | `src/components/Tools/ExtractPages/` |
| **Backend Location** | Handled via `RearrangePagesPDFController` or split logic |
| **Key Classes** | `SplitPDFController` (page range extraction) |
| **C++ Implementation** | `FPDF_ImportPagesByIndex` for selected pages → new document |
| **Migration Difficulty** | LOW |
| **Priority** | P1 |

### 4.7 Remove Pages

| Attribute | Detail |
|-----------|--------|
| **Name** | Delete Pages from PDF |
| **Status** | REAL |
| **Frontend Location** | `src/components/Tools/RemovePages/` |
| **Backend Location** | Handled via `RearrangePagesPDFController` |
| **C++ Implementation** | `FPDFPage_Delete` for each removed page |
| **Migration Difficulty** | LOW |
| **Priority** | P1 |

### 4.8 Insert Blank Pages

| Attribute | Detail |
|-----------|--------|
| **Name** | Insert Blank Pages at Positions |
| **Status** | REAL |
| **Frontend Location** | `src/components/Tools/InsertBlankPages/` |
| **Backend Location** | `src/main/java/com/stirling/pdf/controller/BlankPageController.java` |
| **Key Classes** | `BlankPageController` |
| **Architecture** | Backend creates blank PDFBox pages with specified size → inserts into document |
| **C++ Implementation** | `FPDFPage_New` creates blank page → insert at position |
| **Migration Difficulty** | LOW |
| **Priority** | P1 |

### 4.9 Page Numbers

| Attribute | Detail |
|-----------|--------|
| **Name** | Add Page Numbers |
| **Status** | REAL |
| **Frontend Location** | `src/components/Tools/AddPageNumbers/` |
| **Backend Location** | `src/main/java/com/stirling/pdf/controller/PageNumbersController.java` |
| **Key Classes** | `PageNumbersController` |
| **Architecture** | Backend uses PDFBox to add text objects at page positions |
| **C++ Implementation** | `FPDFPageObj_CreateTextObj` → position at page bottom → `FPDFPage_InsertObject` |
| **Migration Difficulty** | MEDIUM — font loading and positioning logic needs care |
| **Priority** | P1 |

---

## 5. Image Operations

### 5.1 Extract Images

| Attribute | Detail |
|-----------|--------|
| **Name** | Extract All Images from PDF |
| **Status** | REAL |
| **Frontend Location** | `src/components/Tools/ExtractImages/` |
| **Backend Location** | `src/main/java/com/stirling/pdf/controller/ExtractImagesController.java` |
| **Key Classes** | `ExtractImagesController` |
| **Architecture** | Backend uses PDFBox to enumerate `PDImageXObject` → extracts to individual files → ZIP |
| **C++ Implementation** | `FPDFPageObj_GetType` to find image objects → `FPDFImageObj_GetBitmap` → save as PNG/JPEG |
| **Migration Difficulty** | MEDIUM |
| **Priority** | P2 |

### 5.2 Remove Images

| Attribute | Detail |
|-----------|--------|
| **Name** | Remove Images from PDF |
| **Status** | REAL |
| **Frontend Location** | `src/components/Tools/RemoveImages/` |
| **Backend Location** | `src/main/java/com/stirling/pdf/controller/RemoveImagesController.java` |
| **Key Classes** | `RemoveImagesController` |
| **C++ Implementation** | Enumerate page objects → filter `FPDF_PAGEOBJ_IMAGE` → `FPDFPage_RemoveObject` |
| **Migration Difficulty** | LOW |
| **Priority** | P2 |

### 5.3 Replace Image

| Attribute | Detail |
|-----------|--------|
| **Name** | Replace Image in PDF |
| **Status** | REAL |
| **Frontend Location** | `src/components/Tools/ReplaceImage/` |
| **Backend Location** | `src/main/java/com/stirling/pdf/controller/ReplaceImageController.java` |
| **Key Classes** | `ReplaceImageController` |
| **C++ Implementation** | `FPDFPageObj_GetBitmap` for existing → replace with new `FPDFPageObj_NewImageObj` → `FPDFPage_InsertObject` |
| **Migration Difficulty** | MEDIUM |
| **Priority** | P2 |

### 5.4 Add/Overlay Image

| Attribute | Detail |
|-----------|--------|
| **Name** | Add Image to PDF Page(s) |
| **Status** | REAL |
| **Frontend Location** | `src/components/Tools/AddImage/`, `src/components/Tools/OverlayImage/` |
| **Backend Location** | `src/main/java/com/stirling/pdf/controller/OverlayImageController.java` |
| **Key Classes** | `OverlayImageController` |
| **C++ Implementation** | `FPDFPageObj_NewImageObj` → `FPDFImageObj_SetBitmap` → position → `FPDFPage_InsertObject` |
| **Migration Difficulty** | MEDIUM |
| **Priority** | P1 |

---

## 6. UI & Application Features

### 6.1 Tab Management

| Attribute | Detail |
|-----------|--------|
| **Name** | Multi-Tab Document Interface |
| **Status** | REAL |
| **Frontend Location** | `src/components/TabBar/`, `src/stores/` (Zustand store) |
| **Key Classes** | Tab management via Zustand state; `DocumentManagerPlugin` per tab |
| **Architecture** | React state manages array of open documents; each tab has its own EmbedPDF instance |
| **PDFium Involvement** | Per-instance — each tab creates its own PDFium WASM module |
| **Current Limitations** | Memory usage grows linearly with tabs; no tab unloading |
| **Bugs/Problems** | WASM memory per tab not reclaimed on close; can OOM with many large PDFs |
| **C++ Implementation** | Win32 tab control (or custom D2D tabs) with `FPDF_DOCUMENT` per tab; lazy unload on tab switch |
| **Reuse Potential** | HIGH — tab UX patterns map to Win32 |
| **Migration Difficulty** | MEDIUM |
| **Priority** | P1 |

### 6.2 Bookmark Sidebar

| Attribute | Detail |
|-----------|--------|
| **Name** | PDF Bookmarks / Outlines Sidebar |
| **Status** | REAL |
| **Frontend Location** | `src/lib/@embedpdf/core/src/BookmarkPlugin.ts`, `src/components/Sidebar/Bookmarks.tsx` |
| **Key Classes** | `BookmarkPlugin` |
| **PDFium Involvement** | PRIMARY — `FPDFBookmark_GetFirstChild`, `FPDFBookmark_GetNextSibling`, etc. |
| **C++ Implementation** | Direct PDFium bookmark traversal → Win32 TreeView control |
| **Reuse Potential** | HIGH |
| **Migration Difficulty** | LOW |
| **Priority** | P1 |

### 6.3 Attachment Sidebar

| Attribute | Detail |
|-----------|--------|
| **Name** | PDF Attachments Panel |
| **Status** | PARTIAL |
| **Frontend Location** | `src/lib/@embedpdf/core/src/AttachmentPlugin.ts` |
| **Key Classes** | `AttachmentPlugin` |
| **PDFium Involvement** | PRIMARY — `FPDFDoc_GetAttachmentCount`, `FPDFDoc_GetAttachment` |
| **Current Limitations** | Read-only view; no add/remove attachment functionality |
| **C++ Implementation** | PDFium attachment APIs + Win32 ListView; add `FPDFDoc_AddAttachment` for write support |
| **Migration Difficulty** | LOW |
| **Priority** | P3 |

### 6.4 Layer Sidebar

| Attribute | Detail |
|-----------|--------|
| **Name** | Optional Content (Layers) Panel |
| **Status** | VISUAL |
| **Frontend Location** | `src/components/Sidebar/Layers.tsx` (if exists) |
| **Current Limitations** | UI may exist but layer toggle functionality is likely a stub |
| **C++ Implementation** | `FPDFDoc_GetOCCount`, `FPDF_OCG_SetState` for layer visibility |
| **Migration Difficulty** | MEDIUM |
| **Priority** | P3 |

### 6.5 Dark Mode

| Attribute | Detail |
|-----------|--------|
| **Name** | Dark Theme |
| **Status** | REAL |
| **Frontend Location** | Mantine UI theme provider, `src/styles/` |
| **Architecture** | CSS custom properties + Mantine color scheme toggle |
| **PDFium Involvement** | None — pure CSS theming |
| **C++ Implementation** | Win32 dark mode via `SetWindowCompositionAttribute` + D2D color scheme switch; invert PDF background for dark viewing |
| **Reuse Potential** | LOW — CSS theme system replaced by Win32 theming entirely |
| **Migration Difficulty** | MEDIUM — Win32 dark mode has edge cases across Windows versions |
| **Priority** | P1 |

### 6.6 Sepia Mode

| Attribute | Detail |
|-----------|--------|
| **Name** | Sepia/Reading Theme |
| **Status** | PARTIAL |
| **Frontend Location** | Mantine theme variant |
| **Current Limitations** | May not apply sepia to PDF content itself, only to chrome |
| **C++ Implementation** | D2D color matrix transform on rendered PDF tiles (sepia filter) |
| **Migration Difficulty** | LOW |
| **Priority** | P3 |

### 6.7 Reading Mode

| Attribute | Detail |
|-----------|--------|
| **Name** | Distraction-Free Reading View |
| **Status** | VISUAL |
| **Frontend Location** | Likely a toolbar toggle hiding sidebars |
| **Current Limitations** | May only hide UI, no actual reading optimizations |
| **C++ Implementation** | Full-screen mode + hide all chrome + auto-scroll + comfortable column width |
| **Migration Difficulty** | LOW |
| **Priority** | P3 |

### 6.8 Keyboard Shortcuts

| Attribute | Detail |
|-----------|--------|
| **Name** | Keyboard Shortcut System |
| **Status** | REAL |
| **Frontend Location** | `src/hooks/useKeyboardShortcuts.ts` (or similar) |
| **Architecture** | React keydown event handlers mapped to actions |
| **C++ Implementation** | `WM_KEYDOWN` / ` accelerators / `RegisterHotKey` |
| **Reuse Potential** | HIGH — shortcut mapping table directly reusable |
| **Migration Difficulty** | LOW |
| **Priority** | P0 |

### 6.9 Toolbar

| Attribute | Detail |
|-----------|--------|
| **Name** | Main Toolbar |
| **Status** | REAL |
| **Frontend Location** | `src/components/Toolbar/` |
| **Architecture** | Mantine UI components (ActionIcon, Group, Tooltip) + TailwindCSS |
| **C++ Implementation** | Win32 toolbar control or custom D2D toolbar |
| **Reuse Potential** | MEDIUM — toolbar layout and action set reusable; UI framework completely different |
| **Migration Difficulty** | MEDIUM |
| **Priority** | P0 |

### 6.10 Context Menus

| Attribute | Detail |
|-----------|--------|
| **Name** | Right-Click Context Menu |
| **Status** | REAL |
| **Frontend Location** | `src/components/ContextMenu/` (or inline handlers) |
| **C++ Implementation** | `TrackPopupMenu` / `CreatePopupMenu` with `WM_CONTEXTMENU` |
| **Reuse Potential** | HIGH — menu structure and actions directly reusable |
| **Migration Difficulty** | LOW |
| **Priority** | P1 |

### 6.11 Drag and Drop

| Attribute | Detail |
|-----------|--------|
| **Name** | File Drag & Drop to Open |
| **Status** | REAL |
| **Frontend Location** | Tauri drag-drop events + React drop handlers |
| **C++ Implementation** | `WM_DROPFILES` / `IDropTarget` COM interface |
| **Migration Difficulty** | LOW |
| **Priority** | P1 |

### 6.12 File Associations

| Attribute | Detail |
|-----------|--------|
| **Name** | Windows File Association (.pdf) |
| **Status** | PARTIAL |
| **Frontend Location** | Tauri configuration (`tauri.conf.json`) |
| **Current Limitations** | Tauri handles association but may not support all shell verbs |
| **C++ Implementation** | Registry entries: `HKEY_CLASSES_ROOT\.pdf` + `shell\open\command`; app manifest |
| **Migration Difficulty** | LOW |
| **Priority** | P1 |

### 6.13 Auto-Update

| Attribute | Detail |
|-----------|--------|
| **Name** | Automatic Application Updates |
| **Status** | PARTIAL |
| **Frontend Location** | Tauri updater plugin |
| **Current Limitations** | Dependent on Tauri's update server infrastructure |
| **C++ Implementation** | Win32 update service: version check HTTP API → download MSI/EXE → `ShellExecute` installer |
| **Migration Difficulty** | MEDIUM |
| **Priority** | P2 |

### 6.14 MDM Provisioning

| Attribute | Detail |
|-----------|--------|
| **Name** | Mobile Device Management / Enterprise Deployment |
| **Status** | VISUAL |
| **Current Limitations** | Likely not implemented; enterprise deployment not a priority for consumer app |
| **C++ Implementation** | GPO/Intune support via registry policies, ADMX templates |
| **Migration Difficulty** | HIGH |
| **Priority** | P4 |

### 6.15 Connection Modes

| Attribute | Detail |
|-----------|--------|
| **Name** | Offline/Online Mode Switching |
| **Status** | VISUAL |
| **Current Limitations** | Tauri is inherently offline-capable; "connection modes" may refer to AI features |
| **C++ Implementation** | Native app is always offline-capable; online features optional |
| **Migration Difficulty** | N/A |
| **Priority** | P3 |

### 6.16 Print

| Attribute | Detail |
|-----------|--------|
| **Name** | Print PDF |
| **Status** | REAL |
| **Frontend Location** | `src/lib/@embedpdf/core/src/PrintPlugin.ts` |
| **Key Classes** | `PrintPlugin` |
| **Architecture** | Triggers browser `window.print()` with PDF rendered in iframe |
| **PDFium Involvement** | Indirect — rendered via PDFium but printed via browser |
| **Current Limitations** | Print quality depends on browser print pipeline; limited print settings |
| **C++ Implementation** | `FPDF_RenderPage` to printer DC, or `FPDF_SaveAsDocument` + ShellExecute print |
| **Reuse Potential** | MEDIUM |
| **Migration Difficulty** | MEDIUM |
| **Priority** | P1 |

---

## 7. Features NOT in Trimmed Set (Removed from Scope)

These features exist in Stirling PDF but are NOT part of the 17-tool trimmed feature set:

| Feature | Reason for Exclusion |
|---------|----------------------|
| OCR (OCRmyPDF/Tesseract) | External tool dependency; not in trimmed set |
| Format Conversion (LibreOffice) | External tool dependency; not in trimmed set |
| Java/JS Execution | PDFium doesn't support; not in trimmed set |
| Digital Signatures (Certificate) | Complex PKI; not in trimmed set |
| Watermarks | Not in trimmed 17 tools |
| Headers/Footers | Not in trimmed 17 tools |
| Form Filling | Not in trimmed 17 tools (annotations handle text) |
| Stamp | Not in trimmed 17 tools |
| PDF/A Conversion | External tool dependency; not in trimmed set |
| Compare/Diff | Not in trimmed 17 tools |
| Compress/Optimize | Not in trimmed 17 tools |
| Protect/Encrypt | Not in trimmed 17 tools |
| Flatten | Not in trimmed 17 tools |
| AI Features (Python FastAPI) | Python dependency; not in trimmed set |

---

## 8. Priority Summary

## 8. Priority Summary

### P0 — Must Have for MVP
| Feature | Migration Difficulty |
|---------|-------------------|
| Core PDF Rendering | MEDIUM |
| Tile Rendering | MEDIUM |
| Zoom | LOW |
| Scroll | LOW |
| Text Selection | MEDIUM |
| Copy/Paste | LOW |
| Search | LOW |
| Keyboard Shortcuts | LOW |
| Toolbar | MEDIUM |
| Merge | LOW |
| Split | LOW |
| Rotate | LOW |
| Reorganize Pages | LOW |
| Annotation CRUD | MEDIUM |
| Undo/Redo History | REAL (Native C++) |

### P1 — Second Wave
| Feature | Migration Difficulty |
|---------|-------------------|
| Text Editing (Find/Replace) | HIGH |
| Thumbnails | LOW |
| Page Navigation | LOW |
| Tab Management | MEDIUM |
| Bookmark Sidebar | LOW |
| Context Menus | LOW |
| Drag & Drop | LOW |
| File Associations | LOW |
| Dark Mode | MEDIUM |
| Print | MEDIUM |
| Extract Pages | LOW |
| Remove Pages | LOW |
| Insert Blank Pages | LOW |
| Page Numbers | MEDIUM |
| Add/Overlay Image | MEDIUM |

### P2 — Third Wave
| Feature | Migration Difficulty |
|---------|-------------------|
| Spread View | MEDIUM |
| Auto-Rotate | MEDIUM |
| Redaction | HIGH |
| Auto-Update | MEDIUM |
| Extract Images | MEDIUM |
| Remove Images | LOW |
| Replace Image | MEDIUM |

### P3 — Nice to Have
| Feature | Migration Difficulty |
|---------|-------------------|
| Attachment Sidebar | LOW |
| Layer Sidebar | MEDIUM |
| Sepia Mode | LOW |
| Reading Mode | LOW |
| Connection Modes | N/A |
