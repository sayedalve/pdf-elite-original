# Stirling PDF Migration Guide

> Complete migration plan for eliminating the Java/Spring Boot backend and replacing all operations with native PDFium C API calls.

---

## 1. Goal

**Eliminate the entire Java/Spring Boot/Rust/Tauri stack.**

The current architecture has a massive dependency chain for operations that PDFium can handle natively:

```
CURRENT:  React UI → Tauri IPC → Rust → Spring Boot HTTP → Java Controller → PDFBox → PDF file
NATIVE:   Win32 UI → PDFium C API → PDF file
```

This eliminates:
- Java 25 + Spring Boot 4.0.6 runtime (~200MB+ JVM overhead)
- PDFBox 3.0.7 library
- JPDFium 1.0.2 private Maven artifact
- Rust Tauri sidecar process
- HTTP IPC between frontend and backend
- Ghostscript dependency (for text editing font normalization)
- All Docker/container infrastructure for the backend

**FACT:** Stirling PDF is a web application framework. PDF Elite wraps it in Tauri. For a native desktop app, Stirling PDF provides zero architectural value.

**FACT:** After trimming to 17 core tools, every remaining backend operation can be handled by PDFium directly.

---

## 2. Migration Master Table

| # | Operation | Current Controller | Current Library | External Tool? | PDFium Can Handle? | Native C++ Approach | Difficulty | Notes |
|---|-----------|-------------------|----------------|----------------|-------------------|--------------------|------------|-------|
| 1 | Text Edit (Find/Replace) | `EditTextController.java` | PDFBox + Ghostscript | Ghostscript | **YES** (with caveats) | `FPDFPageObj` text manipulation | HIGH | See §3.1 |
| 2 | Merge | `MergeController.java` | PDFBox `PDFMergerUtility` | No | **YES** | `FPDF_ImportPages` | LOW | Direct replacement |
| 3 | Split (by ranges) | `SplitPDFController.java` | PDFBox | No | **YES** | `FPDF_ImportPagesByIndex` + save | LOW | Direct replacement |
| 4 | Split (by fixed count) | `SplitPDFController.java` | PDFBox | No | **YES** | Compute page ranges + `FPDF_ImportPagesByIndex` | LOW | Math + import |
| 5 | Split (by file size) | `SplitPDFController.java` | PDFBox | No | **YES** | Binary search on page ranges + `FPDF_ImportPagesByIndex` | MEDIUM | Needs size estimation |
| 6 | Split (every N pages) | `SplitPDFController.java` | PDFBox | No | **YES** | Compute ranges + `FPDF_ImportPagesByIndex` | LOW | Simple math |
| 7 | Rotate | `RotationController.java` | PDFBox | No | **YES** | `FPDFPage_SetRotation` | LOW | Direct replacement |
| 8 | Auto-Rotate | `AutoRotateController.java` | PDFBox | No | **YES** | `FPDFText_GetCharBox` angle analysis + `FPDFPage_SetRotation` | MEDIUM | Custom orientation detection |
| 9 | Page Reorder | `RearrangePagesPDFController.java` | PDFBox | No | **YES** | `FPDF_ImportPagesByIndex` in new order | LOW | Direct replacement |
| 10 | Extract Pages | `RearrangePagesPDFController.java` | PDFBox | No | **YES** | `FPDF_ImportPagesByIndex` → new doc | LOW | Direct replacement |
| 11 | Remove Pages | `RearrangePagesPDFController.java` | PDFBox | No | **YES** | `FPDFPage_Delete` | LOW | Direct replacement |
| 12 | Insert Blank Pages | `BlankPageController.java` | PDFBox | No | **YES** | `FPDFPage_New` | LOW | Direct replacement |
| 13 | Page Numbers | `PageNumbersController.java` | PDFBox | No | **YES** | `FPDFPageObj_CreateTextObj` + position | MEDIUM | Font loading, positioning |
| 14 | Extract Images | `ExtractImagesController.java` | PDFBox | No | **YES** | `FPDFPageObj_GetType` → `FPDFImageObj_GetBitmap` | MEDIUM | Image format conversion |
| 15 | Remove Images | `RemoveImagesController.java` | PDFBox | No | **YES** | `FPDFPageObj_GetType` filter → `FPDFPage_RemoveObject` | LOW | Direct replacement |
| 16 | Replace Image | `ReplaceImageController.java` | PDFBox | No | **YES** | `FPDFPageObj_NewImageObj` + replace | MEDIUM | Coordinate mapping |
| 17 | Add Image | `OverlayImageController.java` | PDFBox | No | **YES** | `FPDFPageObj_NewImageObj` + `FPDFImageObj_SetBitmap` | MEDIUM | Positioning, sizing |
| 18 | Annotations (Comments) | `AddCommentsController.java` | PDFBox | No | **YES** (already done on frontend) | Frontend PDFium already handles this | LOW | Move to native, no backend needed |
| 19 | PDF Info | `GetInfoOnPDF.java` | PDFBox | No | **YES** | `FPDF_GetMetaText` | LOW | Direct replacement |

---

## 3. Detailed Migration for Each Operation

### 3.1 Text Editing (Find & Replace)

**Current Implementation:**

```
Frontend (React)                       Backend (Java)
├─ User selects text in EmbedPDF       ├─ EditTextController receives request
├─ Sends selection + replacement text  ├─ PdfJsonConversionService (~2000 lines)
│  via Tauri IPC → Spring Boot HTTP    │  ├─ Ghostscript normalizes fonts
│                                     │  ├─ PDFBox extracts full document as structured JSON
│                                     │  │  (each text chunk: {text, font, size, x, y, color, ...})
│                                     │  ├─ JSON manipulation: find/replace text in chunks
│                                     │  ├─ PDFBox rebuilds PDF from modified JSON
│                                     │  └─ Returns new PDF to frontend
└─ Receives new PDF, reloads           └─ (Ghostscript called as external process)
```

**External Dependencies:**
| Tool | Purpose | Still Needed? |
|------|---------|--------------|
| Ghostscript | Font name normalization before JSON conversion | NO — see below |
| PDFBox | Full document serialization to JSON | NO — PDFium handles natively |

**Why Ghostscript Was Needed:**

Ghostscript normalizes font references so PDFBox can reliably extract text with consistent font mappings. Without this step, the same visual text might have different internal font references across pages, making find/replace unreliable.

**Native C++ Approach:**

```
Option A: Direct PDFium Text Object Manipulation (RECOMMENDED for simple cases)
├─ FPDFText_GetCharBox → identify text regions matching search term
├─ FPDFPage_GetObject / FPDFPageObj_GetType → find text page objects
├─ For each matching text object:
│  ├─ FPDFPageObj_GetTextRenderMode → verify it's visible text
│  ├─ Compute replacement text object metrics
│  ├─ Create new text object with replacement text (same font, size, color, position)
│  ├─ FPDFPage_RemoveObject (old) + FPDFPage_InsertObject (new)
│  └─ FPDFPage_GenerateContent
└─ Save document

Option B: Content Stream Parsing (for complex cases)
├─ Parse PDF content streams directly (PDF specification)
├─ Modify text operators (Tj, TJ, ') in content stream
├─ Handle font encoding, text positioning matrices
└─ Write modified content stream back

Option C: Page Flattening + Re-edit (fallback)
├─ Render page with PDFium
├─ OCR-style text detection for positioning
├─ Create new text layer with replaced content
└─ Not recommended — loses original text quality
```

**RECOMMENDATION:** Start with Option A for the majority of cases (straightforward text replacement). PDFium's `FPDFPageObj_CreateTextObj` can create text objects with the same font metrics. For edge cases where text objects span multiple lines or have complex positioning, fall back to Option B (direct content stream editing).

**ASSUMPTION:** Ghostscript is NOT needed for the native rewrite. PDFium natively handles font references and text object creation without font normalization. The Ghostscript dependency in the current stack is an artifact of PDFBox's limitations, not a fundamental PDF requirement.

**Migration Difficulty:** HIGH — This is the most complex operation to migrate. The `PdfJsonConversionService` (~2000 lines) encodes significant domain knowledge about PDF text layout. However, much of that complexity exists because PDFBox requires full document serialization. PDFium's per-object approach is fundamentally simpler.

---

### 3.2 Merge PDFs

**Current Implementation:**

```
MergeController.java
├─ Receives list of uploaded PDF files
├─ PDFMergerUtility.mergeDocuments() (PDFBox)
├─ Handles bookmark conflicts (merges all)
├─ Returns merged PDF
└─ Files transferred via HTTP multipart upload
```

**Native C++ Approach:**

```cpp
// Single function replaces entire controller + service + HTTP layer
bool MergePDFs(const std::vector<std::wstring>& input_paths,
               const std::wstring& output_path) {
    FPDF_DOCUMENT dest = FPDF_CreateNewDocument();
    int page_index = 0;

    for (const auto& path : input_paths) {
        FPDF_DOCUMENT src = FPDF_LoadDocument(ToUtf8(path).c_str(), nullptr);
        if (!src) continue;

        int page_count = FPDF_GetPageCount(src);
        // Import all pages from source
        FPDF_ImportPages(dest, src, nullptr, page_index);
        // nullptr for page range means "all pages"

        FPDF_CloseDocument(src);
        page_index += page_count;
    }

    FPDF_SaveAsDocument(dest, ToUtf8(output_path).c_str(),
                         FPDF_SAVE_NO_LINEARIZATION);
    FPDF_CloseDocument(dest);
    return true;
}
```

**Migration Difficulty:** LOW — PDFium's `FPDF_ImportPages` is designed exactly for this.

**FACT:** `FPDF_ImportPages` handles all cross-document complexities: font deduplication, resource merging, and xobject remapping. No manual handling needed.

---

### 3.3 Split PDF (4 Modes)

**Current Implementation:**

```
SplitPDFController.java — 4 endpoints:
├─ /split-pdf              — Split by page ranges (user-specified)
├─ /split-pdf-by-size      — Split by output file size target
├─ /split-pdf-by-fixed-count — Split into N equal parts
└─ /split-pdf-by-every-n-pages — Split every N pages
```

**Native C++ Approach:**

```cpp
// Mode 1: Split by page ranges
std::vector<FPDF_DOCUMENT> SplitByRanges(FPDF_DOCUMENT src,
                                          const std::vector<PageRange>& ranges) {
    std::vector<FPDF_DOCUMENT> results;
    for (const auto& range : ranges) {
        FPDF_DOCUMENT doc = FPDF_CreateNewDocument();
        int indices[range.count()];
        for (int i = 0; i < range.count(); i++) indices[i] = range.start + i;
        FPDF_ImportPagesByIndex(doc, src, indices, range.count(), 0);
        results.push_back(doc);
    }
    return results;
}

// Mode 2: Split by file size (binary search)
std::vector<FPDF_DOCUMENT> SplitBySize(FPDF_DOCUMENT src, size_t target_bytes) {
    // 1. Save full document to temp, get actual size as upper bound
    // 2. Binary search for split points that produce ~target_bytes files
    // 3. Use SplitByRanges with computed ranges
}

// Mode 3: Split into N parts
std::vector<FPDF_DOCUMENT> SplitByCount(FPDF_DOCUMENT src, int n) {
    int total = FPDF_GetPageCount(src);
    int per_doc = total / n;
    // ... compute ranges, delegate to SplitByRanges
}

// Mode 4: Split every N pages
std::vector<FPDF_DOCUMENT> SplitEveryN(FPDF_DOCUMENT src, int n) {
    // ... compute ranges, delegate to SplitByRanges
}
```

**Migration Difficulty:** LOW for modes 1, 3, 4. MEDIUM for mode 2 (file size splitting requires iterative save+measure to estimate split points).

---

### 3.4 Rotate Pages

**Current Implementation:**

```
RotationController.java
├─ Receives page indices and rotation angle
├─ PDFBox: PDPage.setRotation(rotation)
├─ Saves modified document
└─ Returns to frontend

Frontend RotatePlugin.ts
├─ Visual rotation via CSS transform (immediate preview)
└─ Sends rotation to backend for persistence
```

**Native C++ Approach:**

```cpp
void RotatePages(FPDF_DOCUMENT doc, const std::vector<int>& pages, int degrees) {
    int rotation;
    switch (degrees) {
        case 90:  rotation = 1; break;
        case 180: rotation = 2; break;
        case 270: rotation = 3; break;
        default:  rotation = 0; break;
    }
    for (int page_idx : pages) {
        FPDF_PAGE page = FPDF_LoadPage(doc, page_idx);
        FPDFPage_SetRotation(page, rotation);
        FPDF_ClosePage(page);
    }
}
```

**Migration Difficulty:** LOW — Direct API mapping. Eliminates the dual code path (CSS visual + backend persistent).

---

### 3.5 Auto-Rotate

**Current Implementation:**

```
AutoRotateController.java
├─ PDFBox extracts text from each page
├─ Analyzes text bounding boxes to determine dominant text angle
├─ Applies rotation if text is predominantly landscape/portrait
└─ Returns rotated document
```

**Native C++ Approach:**

```cpp
int DetectDominantAngle(FPDF_PAGE page) {
    FPDF_TEXTPAGE text_page = FPDFText_LoadPage(page);
    int char_count = FPDFText_CountChars(text_page);

    // Sample characters and compute angle from bounding box orientation
    double total_angle = 0;
    int samples = 0;
    for (int i = 0; i < char_count; i += 10) {  // Sample every 10th char
        double left, right, bottom, top;
        FPDFText_GetCharBox(text_page, i, &left, &right, &bottom, &top);
        double width = right - left;
        double height = top - bottom;
        // If height > width, text is likely rotated 90/270
        if (height > width * 2) total_angle += 90;
        else if (width > height * 2) total_angle += 0;
        samples++;
    }
    FPDFText_ClosePage(text_page);

    return (samples > 0) ? static_cast<int>(total_angle / samples) : 0;
}
```

**Migration Difficulty:** MEDIUM — The angle detection algorithm is heuristic and needs tuning. PDFium provides all the primitives (`FPDFText_GetCharBox`).

---

### 3.6 Page Reorder / Extract / Remove

**Current Implementation:**

```
RearrangePagesPDFController.java
├─ Receives new page order (array of page indices)
├─ PDFBox creates new document with pages in specified order
├─ For extract: subset of pages
├─ For remove: all pages except specified
└─ Returns modified document
```

**Native C++ Approach:**

```cpp
// Reorder: create new document, import pages in new order
FPDF_DOCUMENT ReorderPages(FPDF_DOCUMENT src, const std::vector<int>& new_order) {
    FPDF_DOCUMENT dest = FPDF_CreateNewDocument();
    FPDF_ImportPagesByIndex(dest, src, new_order.data(), new_order.size(), 0);
    return dest;
}

// Extract: same as reorder with a subset
FPDF_DOCUMENT ExtractPages(FPDF_DOCUMENT src, const std::vector<int>& pages) {
    return ReorderPages(src, pages);
}

// Remove: delete specified pages from document
void RemovePages(FPDF_DOCUMENT doc, const std::vector<int>& pages) {
    // Sort descending to avoid index shift
    auto sorted = pages;
    std::sort(sorted.rbegin(), sorted.rend());
    for (int idx : sorted) {
        FPDFPage_Delete(doc, idx);
    }
}
```

**Migration Difficulty:** LOW — All three operations are trivially simple with PDFium.

---

### 3.7 Insert Blank Pages

**Current Implementation:**

```
BlankPageController.java
├─ Receives page positions and sizes
├─ PDFBox creates blank PDPage with specified dimensions
├─ Inserts at specified positions
└─ Returns modified document
```

**Native C++ Approach:**

```cpp
void InsertBlankPage(FPDF_DOCUMENT doc, int position, double width_pt, double height_pt) {
    FPDF_PAGE page = FPDFPage_New(doc, position, width_pt, height_pt);
    FPDF_ClosePage(page);  // FPDFPage_New creates and inserts; close the handle
}
```

**Migration Difficulty:** LOW — One function call.

---

### 3.8 Add Page Numbers

**Current Implementation:**

```
PageNumbersController.java
├─ Receives: position (top/bottom), format, start number, font size
├─ PDFBox creates PDPageContentStream for each page
├─ Writes text at computed position
├─ Handles multi-page numbering
└─ Returns modified document
```

**Native C++ Approach:**

```cpp
void AddPageNumbers(FPDF_DOCUMENT doc, const PageNumberConfig& config) {
    int page_count = FPDF_GetPageCount(doc);

    // Load a standard font
    FPDF_FONT font = FPDFText_LoadStandardFont(doc, "Helvetica");

    for (int i = 0; i < page_count; i++) {
        FPDF_PAGE page = FPDF_LoadPage(doc, i);
        double page_w = FPDF_GetPageWidthF(page);
        double page_h = FPDF_GetPageHeightF(page);

        // Create text object
        FPDF_PAGEOBJECT text_obj = FPDFPageObj_CreateTextObj(doc, font, config.font_size);

        // Set text content
        std::wstring text = FormatPageNumber(i + config.start_number, config.format);
        FPDFText_SetText(text_obj, ToUtf16BE(text).c_str());

        // Position at bottom center
        double text_w = GetTextWidth(text_obj, text);
        double x = (page_w - text_w) / 2;
        double y = config.position == BOTTOM ? 30 : page_h - 30;

        FS_MATRIX matrix = {1, 0, 0, 1, x, y};
        FPDFPageObj_SetMatrix(text_obj, &matrix);

        FPDFPage_InsertObject(page, text_obj);
        FPDFPage_GenerateContent(page);
        FPDF_ClosePage(page);
    }
}
```

**Migration Difficulty:** MEDIUM — Font loading, text measurement, and positioning require care. PDFium's standard font set is limited; for custom fonts, use `FPDFText_LoadFont` with a font file.

---

### 3.9 Image Operations (Extract / Remove / Replace / Add)

**Current Implementation:**

```
ExtractImagesController.java  — PDFBox: enumerate PDImageXObject → save to files
RemoveImagesController.java   — PDFBox: remove image XObjects from page resources
ReplaceImageController.java   — PDFBox: find image by index → replace with new image
OverlayImageController.java   — PDFBox: add new image XObject at specified position
```

**Native C++ Approach:**

```cpp
// Extract images: iterate page objects, find images, save bitmaps
std::vector<ExtractedImage> ExtractImages(FPDF_DOCUMENT doc) {
    std::vector<ExtractedImage> images;
    int page_count = FPDF_GetPageCount(doc);
    for (int i = 0; i < page_count; i++) {
        FPDF_PAGE page = FPDF_LoadPage(doc, i);
        int obj_count = FPDFPage_CountObjects(page);
        for (int j = 0; j < obj_count; j++) {
            FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, j);
            if (FPDFPageObj_GetType(obj) == FPDF_PAGEOBJ_IMAGE) {
                FPDF_BITMAP bitmap = FPDFImageObj_GetBitmap(obj);
                // Convert to PNG/JPEG and save
                images.push_back({bitmap, i, j});
            }
        }
        FPDF_ClosePage(page);
    }
    return images;
}

// Remove images: filter and delete image objects
void RemoveImages(FPDF_DOCUMENT doc, int page_index) {
    FPDF_PAGE page = FPDF_LoadPage(doc, page_index);
    std::vector<FPDF_PAGEOBJECT> to_remove;
    int obj_count = FPDFPage_CountObjects(page);
    for (int j = 0; j < obj_count; j++) {
        FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, j);
        if (FPDFPageObj_GetType(obj) == FPDF_PAGEOBJ_IMAGE) {
            to_remove.push_back(obj);
        }
    }
    for (auto obj : to_remove) {
        FPDFPage_RemoveObject(page, obj);
    }
    FPDFPage_GenerateContent(page);
    FPDF_ClosePage(page);
}

// Replace image: find image object, create new, swap
void ReplaceImage(FPDF_DOCUMENT doc, int page_index, int image_index,
                  const uint8_t* new_image_data, size_t data_len) {
    FPDF_PAGE page = FPDF_LoadPage(doc, page_index);
    FPDF_PAGEOBJECT old_obj = FPDFPage_GetObject(page, image_index);

    // Get position of old image
    FS_MATRIX matrix;
    FPDFPageObj_GetMatrix(old_obj, &matrix);

    // Create new image object
    FPDF_PAGEOBJECT new_obj = FPDFPageObj_NewImageObj(doc);
    FPDF_BITMAP bitmap = FPDFBitmap_CreateFromBuffer(...);
    // Or use FPDFImageObj_LoadJpegFile / FPDFImageObj_SetBitmap

    // Set same position
    FPDFPageObj_SetMatrix(new_obj, &matrix);

    // Swap
    FPDFPage_RemoveObject(page, old_obj);
    FPDFPage_InsertObject(page, new_obj);
    FPDFPage_GenerateContent(page);
    FPDF_ClosePage(page);
}

// Add/Overlay image: create image object at specified position
void AddImage(FPDF_DOCUMENT doc, int page_index, const ImagePlacement& placement) {
    FPDF_PAGE page = FPDF_LoadPage(doc, page_index);
    FPDF_PAGEOBJECT img_obj = FPDFPageObj_NewImageObj(doc);
    // Load image from file or buffer
    // Set position, size via FS_MATRIX
    FPDFPage_InsertObject(page, img_obj);
    FPDFPage_GenerateContent(page);
    FPDF_ClosePage(page);
}
```

**Migration Difficulty:** MEDIUM — Image handling requires understanding of PDFium's image object lifecycle, coordinate systems, and bitmap format conversions.

---

### 3.10 Annotations (Add Comments)

**Current Implementation:**

```
AddCommentsController.java — Backend endpoint for adding annotations

BUT: The frontend AnnotationPlugin.ts already handles annotation CRUD locally
     via PDFium WASM! The backend controller appears to be a Stirling PDF
     legacy endpoint that may not be actively used by the trimmed frontend.
```

**FACT:** Annotations are already handled entirely on the frontend via PDFium WASM (`AnnotationPlugin.ts`). The backend `AddCommentsController` is likely unused in the trimmed feature set.

**Native C++ Approach:** Direct PDFium C API annotation handling (see PDF_ENGINE.md §9). No backend needed.

**Migration Difficulty:** LOW — Already a PDFium operation, just move from WASM to native C API.

---

### 3.11 PDF Info / Metadata

**Current Implementation:**

```
GetInfoOnPDF.java (or GetInfoOnPDFController.java)
├─ PDFBox: PDDocument.getDocumentInformation()
├─ Extracts: Title, Author, Subject, Keywords, Creator, Producer, Creation/Mod dates, Page count, File size
└─ Returns as JSON
```

**Native C++ Approach:**

```cpp
struct PdfInfo {
    std::wstring title, author, subject, keywords, creator, producer;
    std::wstring creation_date, mod_date;
    int page_count;
    size_t file_size;
};

PdfInfo GetPdfInfo(FPDF_DOCUMENT doc) {
    PdfInfo info;
    info.page_count = FPDF_GetPageCount(doc);

    auto get_meta = [&](const char* tag) -> std::wstring {
        wchar_t buf[256];
        int len = FPDF_GetMetaText(doc, tag, buf, sizeof(buf));
        return std::wstring(buf, len / sizeof(wchar_t));
    };

    info.title = get_meta("Title");
    info.author = get_meta("Author");
    info.subject = get_meta("Subject");
    info.keywords = get_meta("Keywords");
    info.creator = get_meta("Creator");
    info.producer = get_meta("Producer");
    info.creation_date = get_meta("CreationDate");
    info.mod_date = get_meta("ModDate");

    return info;
}
```

**Migration Difficulty:** LOW — Direct API mapping.

---

## 4. External Tool Elimination

### 4.1 Ghostscript

| Aspect | Detail |
|--------|--------|
| **Current Use** | Font normalization in `PdfJsonConversionService` for text editing |
| **Used By** | `EditTextController` only |
| **Still Needed?** | NO — not for trimmed feature set |
| **Why** | PDFium handles font references natively; text editing via page object manipulation doesn't need font normalization |
| **Elimination Path** | Remove entirely when text editing is reimplemented with PDFium page objects |

**FACT:** Ghostscript is the only external tool used by the trimmed feature set. All other external tools (OCRmyPDF, Tesseract, LibreOffice, qpdf) are used by features that have been trimmed.

### 4.2 LibreOffice

| Aspect | Detail |
|--------|--------|
| **Current Use** | Format conversion (DOCX/XLSX/PPTX → PDF) |
| **Used By** | Conversion tools (trimmed away) |
| **Still Needed?** | NO |

### 4.3 Python (FastAPI AI Engine)

| Aspect | Detail |
|--------|--------|
| **Current Use** | AI features (summarization, Q&A) on port 5001 |
| **Used By** | AI tools (trimmed away from core) |
| **Still Needed?** | NO for core features. Can be re-added as optional module later. |

### 4.4 qpdf

| Aspect | Detail |
|--------|--------|
| **Current Use** | PDF optimization, linearization, encryption |
| **Used By** | Optimization/security tools (trimmed away) |
| **Still Needed?** | NO |

---

## 5. Stack Elimination Summary

| Component | Lines of Code (est.) | Can Be Removed? | Replacement |
|-----------|---------------------|-----------------|-------------|
| Java Spring Boot backend | ~50,000+ | YES | PDFium C++ directly |
| `PdfJsonConversionService` | ~2,000 | YES | PDFium text object manipulation |
| All Spring Boot controllers | ~5,000 | YES | PDFium C API calls |
| PDFBox dependency | ~3MB JAR | YES | PDFium DLL (~30MB, but replaces PDFBox + PDF.js + JPDFium) |
| JPDFium 1.0.2 | Unknown (private) | YES | Single native PDFium DLL |
| Tauri Rust sidecar | ~10,000 | YES | Pure Win32 C++ application |
| Tauri IPC bridge | ~5,000 | YES | Direct function calls |
| Ghostscript | External binary | YES | Not needed for PDFium text editing |
| PDF.js (thumbnails) | ~1MB JS | YES | PDFium at lower scale |
| React + Mantine + TailwindCSS | ~100,000+ | YES | Win32 + D2D UI |
| EmbedPDF library | ~20,000 | YES | Custom C++ PDFium wrappers |

**FACT:** The entire Java/Spring Boot/Rust/Tauri/React stack exists to bridge between "web technologies for UI" and "Java for PDF operations." With a native C++ app using PDFium, this entire bridge becomes unnecessary.

---

## 6. Migration Phases

### Phase 1: Core Engine (Weeks 1-4)
- Build PDFium for Windows x64
- Implement RAII wrappers (PdfDocument, PdfPage, etc.)
- Implement rendering pipeline (PDFium → D2D)
- Implement tile cache and viewport
- Get a basic PDF viewer working

### Phase 2: Navigation & Text (Weeks 5-8)
- Text extraction and selection
- Search (find in document)
- Bookmarks, page navigation, zoom, scroll
- Thumbnails (from PDFium, eliminate PDF.js)

### Phase 3: Annotations (Weeks 9-12)
- All 12 annotation types
- Annotation rendering overlay
- Annotation CRUD operations

### Phase 4: Page Manipulation (Weeks 13-16)
- Merge, split (all 4 modes)
- Rotate, auto-rotate
- Page reorder, extract, remove
- Insert blank pages
- Page numbers

### Phase 5: Image Operations (Weeks 17-20)
- Extract, remove, replace, add images
- Image overlay

### Phase 6: Text Editing (Weeks 21-26)
- Find and replace via PDFium page objects
- Most complex migration — dedicated phase
- Eliminates Ghostscript dependency

### Phase 7: Polish (Weeks 27-32)
- Tab management, dark mode, print
- Keyboard shortcuts, context menus
- File associations, drag & drop
- Performance optimization, memory profiling

---

## 7. Risk Register

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|-----------|
| Text editing via PDFium page objects doesn't match PDFBox quality | MEDIUM | HIGH | Implement content stream parsing as fallback (Option B) |
| PDFium redaction burn-in leaves recoverable text | MEDIUM | HIGH | Custom content removal pass before PDFium redaction; verify with binary inspection |
| Complex PDF files render differently in native PDFium vs WASM PDFium | LOW | MEDIUM | Test against Chrome's PDF viewer (same PDFium engine) as reference |
| Font handling in page numbers differs from PDFBox output | MEDIUM | LOW | Use Windows system fonts via `FPDFText_LoadFont` with .ttf files |
| Large document (>1000 pages) performance regresses | LOW | MEDIUM | LRU page cache, tile eviction, progressive loading |
| PDFium build from source fails on specific Windows versions | LOW | HIGH | Use prebuilt binaries as fallback; pin to supported Windows SDK version |