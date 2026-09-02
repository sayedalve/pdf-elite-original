# FEATURE_SPEC.md — PDF Elite Native Functional Requirements

> **Version:** 1.0 | **Status:** Draft | **Traces to:** PRODUCT_SPEC.md §4 (must-preserve features)

Each feature is tagged with a priority and traced to the current implementation where applicable.

---

## Priority Legend

| Priority | Meaning |
|---|---|
| **P0** | Must-have for v1.0 launch. Blocks release if missing. |
| **P1** | Should-have for v1.0. Can defer to v1.1 if timeline pressure. |
| **P2** | Nice-to-have. Planned for v1.1+. |

---

## 1. Viewing

### 1.1 PDF Rendering

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| V-001 | Render PDF pages using PDFium at native resolution | P0 | Current: EmbedPDF (Pdfium WASM + 20 plugins) |
| V-002 | Tile-based rendering: 768px tiles with 5px overlap | P0 | FACT: Current tile config from repo analysis |
| V-003 | Pre-fetch 1 extra ring of tiles beyond visible area | P0 | FACT: Current pre-fetch config from repo analysis |
| V-004 | LRU tile cache to limit memory usage | P0 | RECOMMENDATION: 256 MB default cache |
| V-005 | Render in sRGB color space | P1 | Current: Canvas 2D (sRGB by default) |
| V-006 | Support PDF 1.4–2.0 spec rendering | P0 | FACT: PDFium supports up to PDF 2.0 |

**Acceptance Criteria:**
- AC-001: A 100-page PDF renders without visible gaps between tiles at any zoom level.
- AC-002: Scrolling through a 500-page document at 100% zoom is smooth (≥30 FPS).
- AC-003: Memory usage does not grow unbounded when scrolling through large documents.
- AC-004: Tiles render in <50ms each for letter-size pages at 150% zoom.

### 1.2 Zoom

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| V-010 | Zoom levels: 50%, 75%, 100%, 125%, 150%, 200%, 300%, 400% | P0 | Current: similar preset levels |
| V-011 | Fit Page zoom (entire page visible) | P0 | Current: `ViewerContext` fit-page mode |
| V-012 | Fit Width zoom (page width fills viewport) | P0 | Current: `ViewerContext` fit-width mode |
| V-013 | Continuous zoom via Ctrl+Mouse Wheel (10%–800%) | P0 | RECOMMENDATION: Adobe-style continuous zoom |
| V-014 | Zoom to selection (zoom to selected area) | P1 | Current: UNKNOWN |
| V-015 | Zoom preserves scroll center point | P0 | Current: Bridge pattern maintains position |

**Acceptance Criteria:**
- AC-010: Ctrl+Mouse Wheel zooms in 5% increments, centered on cursor position.
- AC-011: Switching between Fit Page and Fit Width does not lose the current page.
- AC-012: Zoom percentage displayed in toolbar and status bar.

### 1.3 Navigation

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| V-020 | Navigate to next page | P0 | Current: `PageContext` navigation |
| V-021 | Navigate to previous page | P0 | Current: `PageContext` navigation |
| V-022 | Go to specific page number (Ctrl+G dialog) | P0 | Current: `PageContext` go-to-page |
| V-023 | Scroll / pan via mouse drag | P0 | Current: Bridge pattern for scroll events |
| V-024 | Scroll via mouse wheel (vertical) | P0 | Current: native webview scroll |
| V-025 | Scroll via trackpad gestures | P1 | RECOMMENDATION: Win32 smooth scrolling |
| V-026 | Page number display in status bar: "Page X of Y" | P0 | Current: `PageContext` + status bar |

**Keyboard Shortcuts:**

| Action | Shortcut | Notes |
|---|---|---|
| Next page | `Page Down`, `→`, `↓` | Wraps to next page at bottom |
| Previous page | `Page Up`, `←`, `↑` | Wraps to previous page at top |
| First page | `Home` | |
| Last page | `End` | |
| Go to page | `Ctrl+G` | Opens dialog |
| Zoom in | `Ctrl+=`, `Ctrl+Mouse Wheel Up` | |
| Zoom out | `Ctrl+-`, `Ctrl+Mouse Wheel Down` | |
| Fit page | `Ctrl+0` | RECOMMENDATION |
| Fit width | `Ctrl+1` | RECOMMENDATION |
| Zoom 100% | `Ctrl+2` | RECOMMENDATION |

### 1.4 Thumbnails

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| V-030 | Page thumbnail sidebar (left panel) | P0 | Current: `ThumbnailContext` + pdfjs-dist 5.4.149 |
| V-031 | Click thumbnail to navigate to page | P0 | Current: thumbnail click → page change |
| V-032 | Thumbnail size adjustable (small/medium/large) | P1 | Current: UNKNOWN |
| V-033 | Current page highlighted in thumbnail panel | P0 | Current: active page indicator |
| V-034 | Thumbnail generation is non-blocking | P0 | Current: likely async via pdfjs-dist |
| V-035 | Thumbnails generated at low resolution (e.g., 200px wide) | P0 | RECOMMENDATION for performance |

**Acceptance Criteria:**
- AC-030: Thumbnail panel is visible by default, toggleable via toolbar button or `F4`.
- AC-031: Clicking a thumbnail navigates to that page instantly (<16ms).
- AC-032: Thumbnails for a 100-page document generate within 3 seconds.

---

## 2. Text Editing

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| T-001 | Click on existing text to select text block | P0 | Current: `pdfTextEditor` tool + PdfJsonConversionService |
| T-002 | Edit selected text inline | P0 | Current: text overlay in React |
| T-003 | Modify font family of selected text | P1 | Current: UNKNOWN — verify via PdfJsonConversionService schema |
| T-004 | Modify font size of selected text | P1 | Current: UNKNOWN |
| T-005 | Modify text color of selected text | P1 | Current: UNKNOWN |
| T-006 | Modify font weight (bold/italic) | P2 | RECOMMENDATION: complex in PDFium |
| T-007 | Delete text from PDF | P0 | Current: PDFBox text removal |
| T-008 | Insert new text at cursor position | P0 | Current: PDFBox text insertion via PdfJsonConversionService |
| T-009 | Text changes are undoable/redoable | P0 | Current: `HistoryContext` undo/redo |
| T-010 | Text selection via click-drag | P0 | Current: `SelectionContext` |

**Acceptance Criteria:**
- AC-001: Clicking on a text block in a PDF opens an inline editor at that position.
- AC-002: Modifying text and pressing Enter commits the change. Pressing Escape cancels.
- AC-003: Undo (Ctrl+Z) reverts the text change. Redo (Ctrl+Y) re-applies it.
- AC-004: Edited text renders in the saved PDF identically to how it appeared in the editor.
- AC-005: Text editing works on PDFs with embedded and standard fonts.

**FACT:** The current `PdfJsonConversionService` (~2000 lines) performs bidirectional PDF↔JSON conversion. This is the most complex feature to replicate.

**UNKNOWN: Requires verification.** Exact capabilities of text editing — can the current app change fonts, or only text content?

---

## 3. Annotations

### 3.1 General Annotation Requirements

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| A-001 | Support 11 annotation types (see table below) | P0 | FACT: from repo analysis |
| A-002 | Select annotation by clicking on it | P0 | Current: `AnnotationContext` |
| A-003 | Move annotation by dragging | P0 | Current: annotation drag handler |
| A-004 | Resize annotation by dragging handles | P1 | Current: UNKNOWN |
| A-005 | Delete selected annotation (Delete key) | P0 | Current: annotation deletion |
| A-006 | Modify annotation properties (color, opacity, etc.) | P0 | Current: annotation property panel |
| A-007 | Annotations are undoable/redoable | P0 | Current: `HistoryContext` |
| A-008 | Annotations render correctly in Adobe Reader | P0 | RECOMMENDATION: compatibility testing |
| A-009 | Annotations saved in PDF are standard PDF annotations | P0 | FACT: PDFium uses standard annotation objects |

### 3.2 Annotation Types

| Type | ID | Properties | Current Trace |
|---|---|---|---|
| **Highlight** | A-100 | Color, opacity, quad points | FACT: supported in current app |
| **Underline** | A-110 | Color, quad points | FACT: supported |
| **StrikeOut** | A-120 | Color, quad points | FACT: supported |
| **Ink** | A-130 | Color, stroke width, ink list (points) | FACT: supported |
| **StickyNote** | A-140 | Contents (text), color, icon name | FACT: supported |
| **FreeText** | A-150 | Contents, font, size, color, rect | FACT: supported |
| **Link** | A-160 | Action (URI or GoTo), highlight mode | FACT: supported |
| **Square** | A-170 | Color, border, fill, rect | FACT: supported |
| **Circle** | A-180 | Color, border, fill, rect | FACT: supported |
| **Line** | A-190 | Color, line width, start/end points | FACT: supported |
| **Polygon** | A-200 | Color, border, vertices | FACT: supported |
| **Polyline** | A-210 | Color, border, vertices | FACT: supported |

### 3.3 Annotation Tool Behavior

| ID | Requirement | Priority |
|---|---|---|
| A-300 | Click "Highlight" tool → cursor changes to text selection → drag to highlight | P0 |
| A-301 | Click "StickyNote" tool → click on page → note icon appears → click icon to edit text | P0 |
| A-302 | Click "Ink" tool → draw freehand with mouse → release to commit | P0 |
| A-303 | Click "FreeText" tool → click on page → text box appears → type text | P0 |
| A-304 | Click "Link" tool → drag rectangle → dialog for URL or page destination | P0 |
| A-305 | Click "Square"/"Circle" → drag to draw shape | P0 |
| A-306 | Click "Line" → click start point → drag to end point → release | P0 |
| A-307 | Click "Polygon" → click vertices → double-click to close | P0 |
| A-308 | Click "Polyline" → click vertices → double-click to finish | P0 |

**Acceptance Criteria:**
- AC-A001: All 11 annotation types can be created on any PDF page.
- AC-A002: Annotations persist after save/reopen.
- AC-A003: Annotations created in PDF Elite render identically in Adobe Acrobat Reader.
- AC-A004: Annotations created in Adobe Acrobat Reader render identically in PDF Elite.
- AC-A005: Undo/redo works for all annotation create/modify/delete operations.

---

## 4. Page Management

### 4.1 Page Reorganization

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| P-001 | Reorder pages via drag-and-drop in thumbnail panel | P0 | Current: `reorganizePages` tool |
| P-002 | Move page up/down via toolbar buttons | P0 | Current: `reorganizePages` tool |
| P-003 | Move page to specific position (dialog) | P1 | Current: UNKNOWN |
| P-004 | Page reorder is undoable | P0 | Current: `HistoryContext` |

**Acceptance Criteria:**
- AC-P001: Dragging a thumbnail to a new position in the sidebar reorders the page.
- AC-P002: Moving a page updates all page references (bookmarks, links to page numbers).

### 4.2 Page Insertion & Deletion

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| P-010 | Insert blank page after current page | P0 | Current: `insertBlankPages` tool |
| P-011 | Insert blank page before current page | P0 | Current: `insertBlankPages` tool |
| P-012 | Choose blank page size (Letter, A4, Legal, custom) | P1 | Current: UNKNOWN — verify page size options |
| P-013 | Choose blank page orientation (portrait/landscape) | P1 | Current: UNKNOWN |
| P-014 | Delete current page | P0 | Current: `removePages` tool |
| P-015 | Delete selected pages (multi-select) | P0 | Current: `removePages` tool |
| P-016 | Page deletion is undoable | P0 | Current: `HistoryContext` |
| P-017 | Confirmation dialog before deleting pages | P0 | RECOMMENDATION: prevent accidental data loss |

### 4.3 Page Extraction

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| P-020 | Extract selected pages to a new PDF file | P0 | Current: `extractPages` tool |
| P-021 | Extract page range (e.g., pages 3-7) | P0 | Current: `extractPages` tool |
| P-022 | Extracted PDF opens in new tab | P1 | RECOMMENDATION |

**Acceptance Criteria:**
- AC-P020: Extracting pages creates a valid, standalone PDF containing only those pages.
- AC-P021: Extracted PDF preserves all content (text, images, annotations) of extracted pages.

### 4.4 Page Rotation

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| P-030 | Rotate selected pages 90° clockwise | P0 | Current: `rotate` tool |
| P-031 | Rotate selected pages 90° counter-clockwise | P0 | Current: `rotate` tool |
| P-032 | Rotate selected pages 180° | P0 | Current: `rotate` tool |
| P-033 | Rotate all pages | P0 | Current: `rotate` tool |
| P-034 | Auto-rotate pages based on content orientation | P1 | Current: `autoRotate` tool |
| P-035 | Page rotation is undoable | P0 | Current: `HistoryContext` |

### 4.5 Page Numbers

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| P-040 | Add page numbers to all pages | P0 | Current: `addPageNumbers` tool |
| P-041 | Page number position: top-left, top-center, top-right, bottom-left, bottom-center, bottom-right | P0 | Current: `addPageNumbers` tool |
| P-042 | Page number format: "Page X", "X of Y", "X/Y", custom template | P1 | Current: UNKNOWN — verify format options |
| P-043 | Page number font size adjustable | P1 | Current: UNKNOWN |
| P-044 | Start page number from a specific number (not 1) | P1 | Current: UNKNOWN |
| P-045 | Page number addition is undoable | P0 | Current: `HistoryContext` |

---

## 5. Image Management

### 5.1 Add Image

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| I-001 | Add image (PNG, JPEG, BMP, TIFF) to current page | P0 | Current: `addImage` tool |
| I-002 | Click to place image at cursor position | P0 | Current: `addImage` tool |
| I-003 | Drag to define image bounds (maintain aspect ratio with Shift) | P0 | RECOMMENDATION |
| I-004 | Move placed image by dragging | P0 | Current: UNKNOWN |
| I-005 | Resize placed image by dragging handles | P1 | Current: UNKNOWN |
| I-006 | Image addition is undoable | P0 | Current: `HistoryContext` |

### 5.2 Extract Images

| ID | Requirement | Priority | Trace |
|---|---|---|
| I-010 | List all images in the document | P0 | Current: `extractImages` tool |
| I-011 | Preview images before extraction | P1 | Current: UNKNOWN |
| I-012 | Extract selected images to files | P0 | Current: `extractImages` tool |
| I-013 | Extract all images to a folder | P0 | Current: `extractImages` tool |
| I-014 | Choose output format (original, PNG, JPEG) | P1 | RECOMMENDATION |

### 5.3 Remove Image

| ID | Requirement | Priority | Trace |
|---|---|---|
| I-020 | Click to select an image on a page | P0 | Current: `removeImage` tool |
| I-021 | Delete selected image (Delete key or toolbar) | P0 | Current: `removeImage` tool |
| I-022 | Image removal is undoable | P0 | Current: `HistoryContext` |

### 5.4 Replace Image

| ID | Requirement | Priority | Trace |
|---|---|---|
| I-030 | Select an image on a page | P0 | Current: `replaceImage` tool |
| I-031 | Choose replacement image file | P0 | Current: `replaceImage` tool |
| I-032 | Replacement preserves original position and size | P0 | RECOMMENDATION |
| I-033 | Image replacement is undoable | P0 | Current: `HistoryContext` |

---

## 6. Document Operations

### 6.1 File Operations

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| D-001 | Open PDF file via File → Open (Ctrl+O) | P0 | Current: Tauri file dialog |
| D-002 | Open PDF file via drag-and-drop onto window | P0 | Current: `DragDropContext` |
| D-003 | Open PDF file via Windows Explorer (file association) | P0 | RECOMMENDATION |
| D-004 | Open recent file from File → Recent Files | P1 | RECOMMENDATION |
| D-005 | Save (Ctrl+S) — overwrite original | P0 | Current: Tauri IPC → Spring Boot → PDFBox save |
| D-006 | Save As (Ctrl+Shift+S) — save to new location | P0 | Current: Tauri IPC → Spring Boot → PDFBox save |
| D-007 | Save incrementally (fast save, append changes) | P1 | RECOMMENDATION: PDFium `FPDF_SaveAsCopy` |
| D-008 | Close document (Ctrl+W) | P0 | Current: `FileContext` close |
| D-009 | Warn on close with unsaved changes | P0 | Current: modification check in `FileContext` |
| D-010 | Open multiple documents in tabs | P0 | Current: `FileContext` multi-file support |
| D-011 | Close all documents | P1 | RECOMMENDATION |
| D-012 | New blank PDF document (Ctrl+N) | P1 | Current: UNKNOWN |

### 6.2 Merge

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| D-020 | Merge 2+ PDF files into one | P0 | Current: `merge` tool → PDFBox `PDDocument` merge |
| D-021 | Drag-and-drop reorder files before merging | P0 | Current: `merge` tool UI |
| D-022 | Select specific page ranges from each file | P1 | Current: UNKNOWN |
| D-023 | Preview pages before merging | P1 | RECOMMENDATION |

**Acceptance Criteria:**
- AC-D020: Merging 3 PDFs produces a single PDF with all pages in the specified order.
- AC-D021: Bookmarks, links, and annotations from source PDFs are preserved where possible.

### 6.3 Split

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| D-030 | Split PDF by page ranges (e.g., 1-3, 5, 7-10) | P0 | Current: `split` tool → PDFBox |
| D-031 | Split PDF into individual pages | P0 | Current: `split` tool |
| D-032 | Split every N pages | P1 | RECOMMENDATION |
| D-033 | Split at every blank page | P2 | RECOMMENDATION |
| D-034 | Output: save each split as separate PDF file | P0 | Current: `split` tool |

---

## 7. Search

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| S-001 | Text search via Ctrl+F | P0 | Current: `SearchContext` |
| S-002 | Search highlights all matches on current page | P0 | Current: `SearchContext` |
| S-003 | Navigate between matches (F3 / Shift+F3) | P0 | Current: `SearchContext` |
| S-004 | Case-sensitive search toggle | P1 | Current: UNKNOWN |
| S-005 | Whole-word search toggle | P1 | RECOMMENDATION |
| S-006 | Search across all pages (show match count) | P0 | Current: `SearchContext` |
| S-007 | Search results panel listing all matches with page numbers | P1 | RECOMMENDATION |

**Acceptance Criteria:**
- AC-S001: Pressing Ctrl+F opens a search bar at the top of the viewer.
- AC-S002: Typing a query highlights all matches on the current page within 500ms.
- AC-S003: Pressing F3 navigates to the next match, wrapping to the next page if needed.
- AC-S004: Match count displayed: "3 of 47 matches".

---

## 8. Printing

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| PR-001 | Print current page (Ctrl+P → select "Current Page") | P0 | Current: `PrintContext` |
| PR-002 | Print all pages | P0 | Current: `PrintContext` |
| PR-003 | Print page range | P0 | Current: `PrintContext` |
| PR-004 | Print with annotations | P0 | Current: UNKNOWN |
| PR-005 | Print without annotations | P1 | RECOMMENDATION |
| PR-006 | Print preview | P1 | RECOMMENDATION |
| PR-007 | Fit to page / actual size / shrink to fit options | P1 | RECOMMENDATION |

---

## 9. Settings & Preferences

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| SET-001 | Dark theme / Light theme toggle | P0 | FACT: `ThemeContext` + `design-tokens.css` |
| SET-002 | Default zoom level setting | P1 | Current: `SettingsContext` |
| SET-003 | Default page layout (single page, continuous, two-page) | P1 | RECOMMENDATION |
| SET-004 | Sidebar visibility (thumbnails on/off) | P0 | Current: `SidebarContext` |
| SET-005 | Toolbar customization | P2 | RECOMMENDATION |
| SET-006 | Keyboard shortcut customization | P2 | RECOMMENDATION |
| SET-007 | Default save format (PDF version) | P2 | RECOMMENDATION |

---

## 10. Undo/Redo

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| U-001 | Undo last operation (Ctrl+Z) | P0 | Current: `HistoryContext` |
| U-002 | Redo last undone operation (Ctrl+Y / Ctrl+Shift+Z) | P0 | Current: `HistoryContext` |
| U-003 | Undo/redo for text edits | P0 | Current: `HistoryContext` |
| U-004 | Undo/redo for annotations (create, modify, delete) | P0 | Current: `HistoryContext` |
| U-005 | Undo/redo for page operations (insert, delete, rotate, reorder) | P0 | Current: `HistoryContext` |
| U-006 | Undo/redo for image operations (add, remove, replace) | P0 | Current: `HistoryContext` |
| U-007 | Undo/redo for page numbers | P1 | Current: `HistoryContext` |
| U-008 | Clear undo history on save | P1 | RECOMMENDATION |
| U-009 | Undo stack limit: minimum 50 operations | P0 | RECOMMENDATION |

**Acceptance Criteria:**
- AC-U001: After performing 20 operations, Ctrl+Z pressed 20 times returns the document to its original state.
- AC-U002: After undoing 10 operations, Ctrl+Y pressed 10 times re-applies them in order.
- AC-U003: Undo/redo does not corrupt the PDF structure.

---

## 11. Document Properties

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| DP-001 | View document metadata (title, author, subject, keywords, creator, producer) | P0 | Current: pdfjs-dist metadata extraction |
| DP-002 | Edit document metadata fields | P1 | RECOMMENDATION |
| DP-003 | View PDF version | P0 | Current: pdfjs-dist |
| DP-004 | View page count | P0 | Current: `FileContext` |
| DP-005 | View file size | P1 | RECOMMENDATION |
| DP-006 | View page dimensions (width × height) | P1 | RECOMMENDATION |

---

## 12. Multi-Tab Support

| ID | Requirement | Priority | Trace |
|---|---|---|---|
| MT-001 | Open multiple PDFs in tabs | P0 | Current: `FileContext` multi-file |
| MT-002 | Switch between tabs via click | P0 | Current: tab bar UI |
| MT-003 | Switch tabs via Ctrl+Tab / Ctrl+Shift+Tab | P0 | RECOMMENDATION |
| MT-004 | Close individual tabs (middle-click or X button) | P0 | Current: tab close button |
| MT-005 | Tab shows filename (truncated if long) | P0 | Current: tab title |
| MT-006 | Modified indicator (*) on tab title | P0 | Current: modification flag in `FileContext` |
| MT-007 | Drag tabs to reorder | P2 | RECOMMENDATION |

---

## 13. Keyboard Shortcuts Summary

| Action | Shortcut | Category |
|---|---|---|
| Open file | `Ctrl+O` | File |
| Save file | `Ctrl+S` | File |
| Save As | `Ctrl+Shift+S` | File |
| Close document | `Ctrl+W` | File |
| Print | `Ctrl+P` | File |
| Undo | `Ctrl+Z` | Edit |
| Redo | `Ctrl+Y` / `Ctrl+Shift+Z` | Edit |
| Select all | `Ctrl+A` | Edit |
| Delete | `Delete` | Edit |
| Find | `Ctrl+F` | View |
| Find next | `F3` | View |
| Find previous | `Shift+F3` | View |
| Go to page | `Ctrl+G` | Navigate |
| Zoom in | `Ctrl+=` | View |
| Zoom out | `Ctrl+-` | View |
| Fit page | `Ctrl+0` | View |
| Fit width | `Ctrl+1` | View |
| Toggle sidebar | `F4` | View |
| Next tab | `Ctrl+Tab` | Window |
| Previous tab | `Ctrl+Shift+Tab` | Window |
| Next page | `Page Down` | Navigate |
| Previous page | `Page Up` | Navigate |
| First page | `Home` | Navigate |
| Last page | `End` | Navigate |
| Escape | `Esc` | General (cancel/close) |

---

## 14. Error Handling

| ID | Requirement | Priority |
|---|---|---|
| E-001 | Corrupt PDF: show error dialog with filename and reason, do not crash | P0 |
| E-002 | Password-protected PDF: show password dialog, allow cancel | P0 |
| E-003 | File in use by another process: show "file locked" error | P0 |
| E-004 | Disk full on save: show error, do not corrupt the file | P0 |
| E-005 | Out of memory on large file: show error, offer file-backed mode | P1 |
| E-006 | Invalid annotation data: remove corrupt annotation, show warning | P0 |
| E-007 | All errors logged to `%LOCALAPPDATA%\PdfElite\logs\` | P0 |

---

*This feature specification is the authoritative source for implementation. Update it as features are added, modified, or deferred.*