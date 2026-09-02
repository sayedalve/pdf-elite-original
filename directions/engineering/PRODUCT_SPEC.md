# PRODUCT_SPEC.md — PDF Elite Native Product Specification

> **Version:** 1.0 | **Status:** Draft | **Platform:** Windows 10/11 x64 only

---

## 1. Product Definition

**PDF Elite** is a professional PDF editor for Windows. It allows knowledge workers to view, annotate, edit text, rearrange pages, manage images, and perform common PDF operations — entirely locally, without internet connectivity, cloud services, or subscriptions.

The native rebuild replaces the current Tauri+Rust+React+Java/Spring Boot (Stirling PDF fork) architecture with a single native C++/Win32/PDFium executable, achieving dramatically faster performance, smaller footprint, and simpler installation.

### 1.1 Name
- **Product Name:** PDF Elite
- **Internal Code Name:** pdf-elite-native
- **Executable:** `pdf_elite.exe`

### 1.2 Versioning
- Semantic versioning: `MAJOR.MINOR.PATCH`
- Native v1.0.0 targets feature parity with existing v2.14.2 core tools.

---

## 2. Target Users

### 2.1 Primary Personas

| Persona | Description | Key Needs |
|---|---|---|
| **Legal Paralegal** | Prepares court documents, redacts sensitive info, adds page numbers to exhibits | Annotate, redact, page numbers, merge exhibits, professional output |
| **Financial Analyst** | Combines reports, extracts data, adds stamps/headers | Merge, split, extract pages, add images (logos), search |
| **Administrative Assistant** | Handles daily document workflow: scan, organize, distribute | View, rotate, reorder, split, simple edits |
| **Project Manager** | Reviews and marks up shared documents | Annotate, highlight, sticky notes, page management |
| **IT Administrator** | Manages document archives, removes sensitive metadata | View, extract images, remove pages, metadata editing |

### 2.2 User Skill Level
- **Intermediate to advanced** Windows users.
- Familiar with PDF editors (Adobe Acrobat, Foxit, Nitro).
- NOT developers. UI must be intuitive without a manual.

---

## 3. Product Goals

### 3.1 Primary Goals

| # | Goal | Success Indicator |
|---|---|---|
| 1 | **Lightweight** — Small install size, low resource usage | Install size < 60 MB; idle memory < 100 MB |
| 2 | **Fast** — Sub-second operations for common tasks | Open < 100 ms (small PDF); render first page < 50 ms |
| 3 | **Private** — All processing local, no telemetry | Zero network calls in the application; no analytics; no phone-home |
| 4 | **Professional** — Feature-complete for core PDF editing workflows | All 17 core tools working at parity with v2.14.2 |
| 5 | **Reliable** — Handles large files, corrupt PDFs, edge cases gracefully | No crashes on files up to 500 MB; graceful error messages |

### 3.2 Non-Goals

| Non-Goal | Reason |
|---|---|
| SaaS / cloud features | Privacy-first design; no server infrastructure |
| Web application | Windows-native only |
| macOS / Linux support | Windows 10/11 is the only target platform |
| OCR (text recognition from scanned images) | Requires Tesseract/OCRmyPDF external deps; defer to v2+ |
| Format conversion (Office→PDF, HTML→PDF, eBook→PDF) | Requires LibreOffice, WeasyPrint, Calibre; out of scope |
| Digital signatures / certificate management | Complex PKI; defer to v2+ |
| Form filling / AcroForm | FUTURE: not in v1 scope |
| PDF/A compliance | FUTURE: not in v1 scope |
| JavaScript in PDFs | Not supported; security risk |
| Collaboration / real-time editing | Local-only application |
| Batch processing of multiple files | Not in v1 scope |
| Plugin/extension system | 17 tools is the complete set |

---

## 4. Core Features (MUST Preserve from v2.14.2)

These features exist in the current application and MUST be in the native v1.0 release.

### 4.1 Viewer
- [x] PDF rendering with zoom (fit-page, fit-width, percentage)
- [x] Tile-based rendering for smooth panning
- [x] Page thumbnails sidebar
- [x] Page navigation (prev/next/go-to-page)
- [x] Keyboard shortcuts for navigation

### 4.2 Text Editing
- [x] Click-to-edit text blocks
- [x] Inline text modification (font, size, color) 
- [x] FACT: Current implementation uses PdfJsonConversionService (~2000 lines, bidirectional PDF↔JSON)

### 4.3 Annotations (11 types)
- [x] Highlight, Underline, StrikeOut
- [x] Ink (freehand drawing)
- [x] StickyNote (popup text)
- [x] Link (URL or page destination)
- [x] FreeText (inline text)
- [x] Square, Circle (shape annotations)
- [x] Line, Polygon, Polyline (drawing annotations)

### 4.4 Page Management
- [x] Reorder pages (drag-and-drop or move buttons)
- [x] Insert blank pages
- [x] Extract pages to new document
- [x] Remove/delete pages
- [x] Rotate pages (90°, 180°, 270°)
- [x] Auto-rotate (detect content orientation)
- [x] Add page numbers (custom format, position)

### 4.5 Image Management
- [x] Add image to page (PNG, JPEG, BMP, TIFF)
- [x] Extract images from PDF
- [x] Remove image from PDF
- [x] Replace image in PDF

### 4.6 Document Operations
- [x] Merge multiple PDFs
- [x] Split PDF by page ranges
- [x] Open, save, save-as
- [x] Print

### 4.7 General
- [x] Dark theme / light theme
- [x] Multiple document tabs
- [x] Text search within document
- [x] Document properties view
- [x] Undo/redo
- [x] Keyboard shortcuts

---

## 5. Features to Add (Native Advantage)

These features are enabled by the native architecture and should be added.

### 5.1 Performance
- Direct file open from Windows Explorer (no Tauri bridge delay)
- Instant cold start (no Java/Spring Boot boot time)
- Memory-mapped file I/O for large PDFs
- GPU-accelerated rendering (Direct2D) — RECOMMENDATION for v1.1

### 5.2 Windows Integration
- Windows file association for `.pdf` ("Open with PDF Elite")
- Shell context menu: "Merge selected PDFs"
- Shell context menu: "Split PDF"
- Jump list (recent files) on taskbar icon
- Thumbnail preview in Windows Explorer
- Drag-and-drop files onto the window

### 5.3 UX Improvements
- Non-modal tool properties panels (dockable sidebar)
- Continuous zoom (Ctrl+Scroll, like Adobe Acrobat)
- Smooth animated page transitions
- Mini-map navigator for large pages
- Ruler/guide overlays (optional)

### 5.4 Reliability
- Auto-save recovery (restore unsaved changes after crash)
- File change detection (warn if file modified externally)
- Validated PDF output (check PDF spec compliance on save)

---

## 6. Platform Requirements

### 6.1 Supported Platforms

| Platform | Version | Architecture | Notes |
|---|---|---|---|
| Windows 10 | 1809+ (Build 17763) | x64 | Minimum for dark mode support |
| Windows 11 | 21H2+ | x64 | Full support |
| Windows Server | 2019+ | x64 | RECOMMENDATION: test but not primary |

### 6.2 Runtime Dependencies

| Dependency | Version | Bundled? |
|---|---|---|
| Visual C++ Runtime | 2022 (14.x) | Yes (via vcruntime140.dll) |
| PDFium | Latest stable | Yes (pdfium.dll) |
| DirectX / Direct2D | Win10 built-in | No (OS-provided) |

### 6.3 Hardware Requirements

| Resource | Minimum | Recommended |
|---|---|---|
| CPU | 1 GHz dual-core | 2 GHz quad-core |
| RAM | 2 GB | 4 GB |
| Disk | 60 MB install | 200 MB with temp files |
| Display | 1280×720 | 1920×1080 |

---

## 7. Success Metrics

### 7.1 Performance Targets

| Metric | Current (FACT/Estimate) | Native Target | Measurement Method |
|---|---|---|---|
| Cold start time | 3-5 seconds (Java boot) | < 1 second | Stopwatch from process creation to first render |
| Install size | ~200+ MB (JRE + JARs) | < 60 MB | File size of installer / installed directory |
| Idle memory | 500 MB+ (JVM) | < 100 MB | Task Manager working set |
| Open 10-page PDF | ~500 ms | < 100 ms | Stopwatch from file dialog close to first render |
| Open 100-page PDF | Unknown | < 500 ms | Stopwatch |
| Open 100 MB PDF | Unknown | < 2 seconds | Stopwatch |
| Render first page | ~200 ms (WASM) | < 50 ms | Frame time measurement |
| Zoom response | ~100 ms (React state) | < 16 ms (1 frame) | Frame time measurement |
| Save (incremental) | Unknown | < 200 ms | Stopwatch |
| Rotate single page | ~200 ms (full round-trip) | < 50 ms | Stopwatch (no IPC) |

### 7.2 Quality Targets

| Metric | Target |
|---|---|
| Zero crashes on PDF 2.0 spec test suite | Must pass |
| Handle corrupt PDFs gracefully | Show error, never crash |
| All 17 tools functional | 100% tool coverage |
| Undo/redo for all destructive operations | 100% coverage |
| Dark/light theme consistency | No visual glitches in either theme |

### 7.3 User Experience Targets

| Metric | Target |
|---|---|
| Time to complete "merge 3 PDFs" | < 10 seconds (including file selection) |
| Time to complete "annotate and save" | < 30 seconds typical workflow |
| Discoverability of tools | All tools accessible within 2 clicks from main toolbar |

---

## 8. Distribution & Installation

### 8.1 Distribution Format
- **Primary:** Single `.exe` installer (NSIS or WiX)
- **Alternative:** Portable `.zip` (no installer, extract and run)
- **RECOMMENDATION:** Use WiX Toolset v4 for MSI installer (better enterprise deployment).

### 8.2 Installation Requirements
- No admin rights required for portable version
- Installer requires admin for file association registration
- No internet connection during installation or use
- No license activation server (if licensing is needed, use local license file)

### 8.3 Update Mechanism
- **FACT:** Current app may have auto-update via Tauri.
- **RECOMMENDATION:** Native app uses simple download-and-replace. Check version URL on startup (opt-in). No forced updates.

---

## 9. Branding & Design

### 9.1 Visual Identity
- **FACT:** Current app uses dark theme via CSS variables + Mantine + `design-tokens.css`.
- The native app should visually match the current dark theme as closely as possible.
- UNKNOWN: Requires verification of exact color values from `design-tokens.css`.

### 9.2 Icon & Branding
- Application icon: PDF document with elite/editing motif
- File type icon: Registered for `.pdf` association
- UNKNOWN: Current icon assets location in the repository. Requires verification.

---

## 10. Risks & Mitigations

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| PDFium lacks feature parity with PDFBox | Medium | High | Verify each of 17 tools against PDFium API before implementation |
| Text editing fidelity lower than current | High | High | Study PdfJsonConversionService JSON schema; may need custom layout engine |
| Win32 UI is perceived as "dated" | Medium | Medium | Invest in custom-drawn controls, smooth animations, modern flat design |
| Large PDF files cause OOM | Medium | High | Implement 3-tier memory model (CustomPDFDocumentFactory pattern) |
| Annotation compatibility with Adobe Reader | Low | High | Test all 11 annotation types against Adobe Reader rendering |
| PDFium API documentation gaps | Medium | Medium | Read PDFium source code; reference `fpdfsdk/fpdfview.h` etc. |

---

*This product specification is the authoritative source for what PDF Elite Native is and is not.*