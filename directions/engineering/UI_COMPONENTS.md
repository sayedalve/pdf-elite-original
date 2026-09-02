# UI Components: Comprehensive Mapping Table

> **Document ID:** FILE-011 | **Status:** DRAFT | **Depends on:** UI_DESIGN.md, FEATURE_SPEC.md

---

## Table of Contents

1. [Component Mapping Table](#1-component-mapping-table)
2. [Component Categories](#2-component-categories)
3. [Custom Controls Requiring D2D](#3-custom-controls-requiring-d2d)
4. [Dialog Components](#4-dialog-components)
5. [Overlay Components](#5-overlay-components)
6. [Status & Utility Components](#6-status--utility-components)

---

## 1. Component Mapping Table

> **FACT:** All React component paths are relative to `src/components/` in the current Tauri application. Proposed C++ class names follow PascalCase with no prefix.

### 1.1 Shell & Layout Components

| Component | Current React File | Current Behavior | Proposed Win32 Control | Proposed C++ Class |
|-----------|-------------------|------------------|----------------------|-------------------|
| `ViewerShell` | `viewer/ViewerShell.tsx` | Top-level orchestrator: tabs, toolbar, left rail, viewer, right panel, status bar. Manages layout proportions. | `WS_OVERLAPPEDWINDOW` main window | `MainWindow` |
| `TabBar` | `viewer/TabBar.tsx` | Horizontal tab strip showing open documents. Tab close button, active tab highlight, drag-reorder. Shows file name + modified indicator. | Custom-drawn `WS_CHILD` window with tab hit-testing | `TabBarWindow` |
| `TabItem` | `viewer/TabBar.tsx` (internal) | Individual tab: icon, filename (truncated), close button (X on hover). Modified dot indicator. | Hit-tested region within `TabBarWindow` | (part of `TabBarWindow`) |
| `ContextualToolbar` | `viewer/ContextualToolbar.tsx` | Mode-dependent toolbar. Shows different button sets for View/Search/Annotate/Edit/Organize modes. | Custom-drawn flat toolbar `WS_CHILD` | `ToolbarWindow` |
| `ViewerLeftRail` | `viewer/ViewerLeftRail.tsx` | Left panel container: mode selector icons, sidebar toggle buttons, page navigation, sidebar content area. | `WS_CHILD` window with child panels | `LeftRailWindow` |
| `WorkbenchBar` | `viewer/WorkbenchBar.tsx` | Bottom status bar: page number (current/total), zoom percentage, tool state, any warnings. | `CreateStatusWindow` or custom `WS_CHILD` | `StatusBarWindow` |
| `Viewer` | `viewer/Viewer.tsx` | EmbedPDF wrapper: creates EmbedPDF instance, passes plugins, handles mode transitions. | Not a window — this is the PDF canvas + overlay manager | `PdfViewerController` |

### 1.2 Left Rail Components

| Component | Current React File | Current Behavior | Proposed Win32 Control | Proposed C++ Class |
|-----------|-------------------|------------------|----------------------|-------------------|
| `ThumbnailSidebar` | `viewer/ThumbnailSidebar.tsx` | Scrollable grid of page thumbnails. Click to navigate. Active page highlighted. Uses `ThumbnailGenerationService` (Pdfium WASM, 10-doc LRU, 1GB session cache). | Custom scrollable `WS_CHILD` with bitmap thumbnails | `ThumbnailPanel` |
| `ThumbnailItem` | `viewer/ThumbnailSidebar.tsx` (internal) | Single page thumbnail: rendered bitmap, page number label, selected state border. | Owner-drawn item in `ThumbnailPanel` | (part of `ThumbnailPanel`) |
| `BookmarkSidebar` | `viewer/BookmarkSidebar.tsx` | Tree view of PDF bookmarks (outline). Click to navigate. Expand/collapse. Shows bookmark title. | `SysTreeView32` with custom draw | `BookmarkPanel` |
| `AttachmentSidebar` | `viewer/AttachmentSidebar.tsx` | List of embedded attachments. Shows filename, size, type. Click to save/open. | `SysListView32` in report mode or custom list | `AttachmentPanel` |
| `LayerSidebar` | `viewer/LayerSidebar.tsx` | Checkable list of PDF optional content layers. Toggle visibility. | Custom check-list `WS_CHILD` | `LayerPanel` |
| `CommentSidebar` | `viewer/CommentSidebar.tsx` | List of annotations/comments with author, date, text. Click to navigate to page. | Custom list with detail view | `CommentPanel` |
| `ModeSelector` | `viewer/ViewerLeftRail.tsx` (internal) | Vertical icon strip: View, Comment, Edit, Organize, Tools modes. Active mode highlighted. | Custom icon button column | (part of `LeftRailWindow`) |
| `SidebarToggle` | `viewer/ViewerLeftRail.tsx` (internal) | Toggle buttons for Thumbnails, Bookmarks, Attachments, Layers, Comments. Active sidebar highlighted. | Custom toggle icon buttons | (part of `LeftRailWindow`) |
| `PageNavigation` | `viewer/ViewerLeftRail.tsx` (partial) | Current page input + total pages label. Up/down arrows. Enter to navigate. | `EditControl` + spin button pair | `PageNavigationCtrl` |

### 1.3 Right Panel Components

| Component | Current React File | Current Behavior | Proposed Win32 Control | Proposed C++ Class |
|-----------|-------------------|------------------|----------------------|-------------------|
| `SearchResultsPanel` | `viewer/right-panel/SearchResultsPanel.tsx` | Shows search matches: page number, context snippet, match count. Click to navigate. Next/prev buttons. | Custom scrollable list `WS_CHILD` | `SearchResultsPanel` |
| `SearchResultItem` | `viewer/right-panel/SearchResultsPanel.tsx` (internal) | Single match: page number, highlighted context text. | Owner-drawn list item | (part of `SearchResultsPanel`) |
| `BookmarksTreePanel` | `viewer/right-panel/BookmarksTreePanel.tsx` | Same bookmark data as left sidebar, but in right panel context. | `SysTreeView32` | `BookmarksTreePanel` |
| `DocumentInfoPanel` | `viewer/right-panel/DocumentInfoPanel.tsx` | PDF metadata: title, author, subject, keywords, creation date, modification date, page count, file size. | Custom `WS_CHILD` with label-value pairs | `DocumentInfoPanel` |

### 1.4 Toolbar & Tool Components

| Component | Current React File | Current Behavior | Proposed Win32 Control | Proposed C++ Class |
|-----------|-------------------|------------------|----------------------|-------------------|
| `ZoomControls` | `viewer/toolbar/ZoomControls.tsx` | Zoom in/out buttons, zoom percentage display, fit-width/fit-page dropdown. Range: 0.2x–5.0x. Default: FitWidth. | Toolbar button group + dropdown | `ZoomControls` |
| `SearchInterface` | `viewer/toolbar/SearchInterface.tsx` | Search input field with 300ms debounce. Next/prev navigation. Match count display. Highlight all toggle. Ctrl+F to focus. | Edit control + toolbar buttons in toolbar area | `SearchBar` |
| `AnnotationTypeButtons` | `viewer/toolbar/AnnotationTypeButtons.tsx` | Buttons for 11 annotation types: Highlight, Ink, StickyNote, Underline, StrikeOut, Link, FreeText, Square, Circle, Line, Polygon, Polyline. | Toolbar button group (toggle buttons) | `AnnotationToolbar` |
| `ToolPicker` | `viewer/toolbar/ToolPicker.tsx` | Dropdown or grid showing available tools. Active tool highlighted. Tool descriptions on hover. | Popup menu or custom dropdown | `ToolPickerMenu` |
| `FilePicker` | `viewer/toolbar/FilePicker.tsx` (partial) | File open button triggering system file dialog. | `GetOpenFileName` / `IFileOpenDialog` | (part of `ToolbarWindow`) |

### 1.5 Canvas Overlay Components

| Component | Current React File | Current Behavior | Proposed Win32 Control | Proposed C++ Class |
|-----------|-------------------|------------------|----------------------|-------------------|
| `FormFieldOverlay` | `viewer/overlays/FormFieldOverlay.tsx` | Renders form field widgets (text inputs, checkboxes, dropdowns, buttons) positioned over PDF form fields. | Custom-drawn controls on D2D canvas or child windows positioned over canvas | `FormFieldOverlay` |
| `ButtonAppearanceOverlay` | `viewer/overlays/ButtonAppearanceOverlay.tsx` | Renders PDF button appearances (push buttons, radio buttons) with custom appearance streams. | Custom-drawn buttons on D2D canvas | `ButtonAppearanceOverlay` |
| `TextSelectionOverlay` | `viewer/overlays/TextSelectionOverlay.tsx` | Highlights selected text regions on the PDF. Shows selection handles. Tolerance factor 3. Marquee disabled. | D2D-drawn highlight rectangles + handle rectangles | `SelectionOverlay` |
| `AnnotationOverlay` | `viewer/overlays/AnnotationOverlay.tsx` | Renders all visible annotations (highlights, ink, sticky notes, shapes) positioned on the page. | D2D-drawn annotation graphics | `AnnotationOverlay` |
| `RedactionOverlay` | `viewer/overlays/RedactionOverlay.tsx` | Shows pending redaction regions (red outlines). Uses `RedactionPendingTracker`. | D2D-drawn red rectangles | `RedactionOverlay` |
| `AnnotationPopup` | `viewer/overlays/AnnotationPopup.tsx` | Popup for editing annotation properties (color, opacity, author, text). | `WS_POPUP` window or popup menu | `AnnotationPopup` |
| `TextSelectionMenu` | `viewer/overlays/TextSelectionMenu.tsx` | Context menu appearing on text selection: Copy, Highlight, Underline, StrikeOut, Search web. | `TrackPopupMenu` | `TextSelectionMenu` |
| `AnnotationSelectionMenu` | `viewer/overlays/AnnotationSelectionMenu.tsx` | Context menu on annotation click: Delete, Properties, Comment. | `TrackPopupMenu` | `AnnotationContextMenu` |
| `RedactionSelectionMenu` | `viewer/overlays/RedactionSelectionMenu.tsx` | Context menu on redaction mark: Apply, Remove, Properties. | `TrackPopupMenu` | `RedactionContextMenu` |

### 1.6 Page & Document Components

| Component | Current React File | Current Behavior | Proposed Win32 Control | Proposed C++ Class |
|-----------|-------------------|------------------|----------------------|-------------------|
| `HomePage` | `home/HomePage.tsx` | Landing page shown when no documents are open. Recent files list, drag-drop target, file open button. | Custom `WS_CHILD` filling client area | `HomePageWindow` |
| `Workbench` | `workbench/Workbench.tsx` | Workbench interface for batch operations (if applicable). | Not ported in v1 | — |
| `LoadingFallback` | `viewer/LoadingFallback.tsx` | Loading spinner shown during document open / render. | Centered `WS_CHILD` with animated progress indicator | `LoadingOverlay` |
| `EncryptedPdfUnlockModal` | `viewer/EncryptedPdfUnlockModal.tsx` | Modal dialog for password-protected PDFs. Password input field, unlock/cancel buttons. | `DialogBoxParam` or `CreateDialogParam` | `PasswordDialog` |
| `PrintDialog` | (via EmbedPDF PrintPlugin) | Print dialog triggered by toolbar button. Uses browser `window.print()`. | `PrintDlg` / `PrintDlgEx` | (system dialog) |
| `SaveAsDialog` | (via ExportPlugin) | Export/save-as dialog for modified PDFs. | `GetSaveFileName` / `IFileSaveDialog` | (system dialog) |

### 1.7 Utility & Service Components (Non-Visual)

| Component | Current React File | Current Behavior | Proposed C++ Equivalent | Proposed C++ Class |
|-----------|-------------------|------------------|----------------------|-------------------|
| `PageMemoryService` | `services/PageMemoryService.tsx` | Persists last-viewed page per document to `localStorage`. On reopen, restores page position. | `std::map<DocId, int>` serialized to JSON settings file | `PageMemoryService` |
| `ThumbnailGenerationService` | `services/ThumbnailGenerationService.tsx` | Generates page thumbnails via Pdfium WASM. 10-doc LRU cache. 1GB session bitmap cache. | Worker thread + LRU cache of `HBITMAP` / D2D bitmaps | `ThumbnailService` |
| `FileContext` | `contexts/FileContext.tsx` | Central file state: `files.byId` map, active file ID, file open/close actions. Uses `useRef` for Blobs, split context (state/actions), selectors. | `DocumentController` managing open document list | `DocumentController` |
| `ViewerBridgeRegistry` | `viewer/ViewerBridgeRegistry.tsx` | Bypasses React for high-frequency state updates (scroll, zoom, cursor). Directly mutates DOM. | Unnecessary in native — use `InvalidateRect` / `SetScrollInfo` directly | (eliminated) |
| `HotkeyContext` | `contexts/HotkeyContext.tsx` | Keyboard shortcut registry. Customizable. Stored in `localStorage`. Maps key combos to actions. | `std::unordered_map<KeyCombo, Action>` + `WM_KEYDOWN` / `WM_SYSKEYDOWN` handler | `HotkeyManager` |
| `ToolRegistryContext` | `contexts/ToolRegistryContext.tsx` | Registry of 17 core tools. Manages active tool, tool switching, file-chaining (consume/undo). | `ToolController` with active tool state | `ToolController` |
| `PreferencesContext` | `contexts/PreferencesContext.tsx` | User preferences: theme, default zoom, UI density, sidebar width, etc. | JSON settings file + `PreferencesController` | `PreferencesController` |
| `ThemeContext` | `contexts/ThemeContext.tsx` | Theme management: light/dark/system. Applies CSS variables. | `ThemeController` with `ThemeTokens` swap | `ThemeController` |
| `SidebarContext` | `contexts/SidebarContext.tsx` | Sidebar state: which sidebar is open, sidebar width, collapsed state. | `SidebarController` managing panel visibility | `SidebarController` |

---

## 2. Component Categories

### 2.1 Category Summary

| Category | Count | Win32 Approach |
|----------|-------|---------------|
| Shell/Layout | 6 | Custom `WS_CHILD` windows with manual layout |
| Left Rail | 9 | Mix of custom panels + `SysTreeView32` |
| Right Panel | 4 | Custom panels + `SysTreeView32` |
| Toolbar/Tools | 5 | Custom flat toolbar + popup menus |
| Canvas Overlays | 9 | D2D-drawn overlays on PDF canvas |
| Page/Document | 6 | Custom windows + system dialogs |
| Utility/Service | 9 | Pure C++ classes, no windows |
| **Total** | **48** | |

### 2.2 Components Eliminated in Migration

| Component | Reason |
|-----------|--------|
| `ViewerBridgeRegistry` | Unnecessary — native Win32 has no DOM bridge overhead |
| `ErrorBoundary` (React) | Replaced by C++ exception handling + structured error reporting |
| `BannerContext` | Replaced by `StatusBarWindow` messages |
| `PosthogTracking` / `ScarfTracking` | Replaced by optional telemetry service (if any) |
| `UpdateStartupPopup` | Replaced by Windows Update or installer check |
| `AppConfigLoader` / `ServerDefaultsSync` | Replaced by local config file |
| `FilesModalContext` | Replaced by `IFileOpenDialog` |
| `TourOrchestration` / `AdminTourOrchestration` | Not ported to v1 (optional later) |
| `FolderFileContext` / `FolderContext` | Not ported to v1 (no folder management in standalone) |

---

## 3. Custom Controls Requiring D2D

These components cannot use standard Win32 controls and require Direct2D custom rendering:

| Control | D2D Element | Reason for Custom Draw |
|---------|-------------|----------------------|
| `PdfCanvas` | `ID2D1HwndRenderTarget` | Tile compositing, annotation rendering, selection highlight |
| `ThumbnailPanel` | Individual `ID2D1Bitmap` items | Bitmap grid with selection state, variable page sizes |
| `TabBarWindow` | `ID2D1HwndRenderTarget` | Custom tab shape with close button, modified indicator, drag reorder |
| `ToolbarWindow` | `ID2D1HwndRenderTarget` | Flat icon buttons, group separators, dropdown arrows, checked state |
| `FormFieldOverlay` | Child windows positioned over canvas | Form widgets must align precisely with PDF coordinates |
| `SelectionOverlay` | Drawn on PDF canvas D2D target | Highlight rectangles, selection handles must align with text |
| `AnnotationOverlay` | Drawn on PDF canvas D2D target | Ink strokes, shapes, sticky note icons overlaid on PDF |
| `RedactionOverlay` | Drawn on PDF canvas D2D target | Red outlined regions aligned with PDF content |
| `LoadingOverlay` | `ID2D1HwndRenderTarget` or `ID2D1DeviceContext` | Animated spinner or progress bar |

> **RECOMMENDATION:** Create a base `D2DWindow` class that handles `ID2D1HwndRenderTarget` creation, resize, and paint cycle. All custom-drawn controls inherit from this.

```cpp
class D2DWindow {
protected:
    ComPtr<ID2D1HwndRenderTarget> target_;
    virtual void OnPaint(ID2D1RenderTarget* ctx) = 0;
    void HandlePaint() {
        target_->BeginDraw();
        OnPaint(target_.Get());
        target_->EndDraw();
    }
public:
    void Create(HWND parent, RECT bounds, DWORD style = WS_CHILD | WS_VISIBLE);
};
```

---

## 4. Dialog Components

| Dialog | Trigger | Win32 Implementation | Modal? |
|--------|---------|---------------------|--------|
| File Open | Toolbar open button, Ctrl+O, drag-drop | `IFileOpenDialog` with PDF filter | Modal (blocking) |
| File Save As | File > Save As, Ctrl+Shift+S | `IFileSaveDialog` with PDF filter | Modal |
| Print | Toolbar print button, Ctrl+P | `PrintDlgEx` with page range selection | Modal |
| Password Unlock | Opening encrypted PDF | `CreateDialogParam` with password edit + OK/Cancel | Modal (app-modal) |
| Preferences | Edit > Preferences | Modeless property sheet or tabbed dialog | Modeless |
| Keyboard Shortcuts | Edit > Keyboard Shortcuts | Custom dialog with key capture | Modeless |
| About | Help > About | `ShellAbout` or custom dialog | Modal |
| Annotation Properties | Double-click annotation | `WS_POPUP` positioned near annotation | Modeless |

> **FACT:** The current `EncryptedPdfUnlockModal` shows after a failed PDF load. In native, PDFium returns `FPDF_ERR_PASSWORD` from `FPDF_LoadMemDocument`, at which point the password dialog is shown and the load is retried with the provided password.

---

## 5. Overlay Components

### 5.1 Overlay Z-Order (Bottom to Top)

| Layer | Component | Drawn By | Interaction |
|-------|-----------|----------|-------------|
| 1 | PDF page tiles | `PdfViewerController` (D2D) | None (read-only) |
| 2 | Selection highlight | `SelectionOverlay` (D2D) | Click-drag to select, handles for extend |
| 3 | Annotation render | `AnnotationOverlay` (D2D) | Click to select, double-click to edit |
| 4 | Redaction marks | `RedactionOverlay` (D2D) | Click to select, context menu |
| 5 | Form field widgets | `FormFieldOverlay` (child windows) | Type, click, dropdown |
| 6 | Context menus | `TrackPopupMenu` (Win32) | Click to select, Esc to dismiss |
| 7 | Annotation popups | `AnnotationPopup` (WS_POPUP) | Edit text, click OK/Cancel |

### 5.2 Hit Testing

> **FACT:** The current EmbedPDF `InteractionManager` handles hit testing for annotations, text selection, and form fields. In native, this translates to coordinate transformation from screen → page → PDF space, then querying PDFium for text/annotation hits.

```cpp
struct HitTestResult {
    enum Type { None, Text, Annotation, FormField, Link };
    Type type;
    int pageIndex;
    POINTF pdfPoint;  // PDF coordinate space
    int annotationIndex;
    int formFieldIndex;
};

HitTestResult HitTest(int screenX, int screenY);
```

---

## 6. Status & Utility Components

### 6.1 Status Bar Fields

The `WorkbenchBar` displays multiple fields. In native, use `StatusBarWindow` with multiple parts:

| Part | Width | Content | Source |
|------|-------|---------|--------|
| 1 | 200px | "Page X of Y" | `PageNavigationCtrl` state |
| 2 | 80px | "Zoom: 150%" | `ZoomControls` state |
| 3 | 200px | Tool name ("Highlight" / "Hand") | `ToolController` active tool |
| 4 | flexible | Status message ("Saved", "3 matches found", etc.) | `AppController` status queue |
| 5 | 40px | Icons (encrypted, signed, form) | `DocumentController` document flags |

### 6.2 Context Menus

| Context Menu | Shown When | Menu Items |
|-------------|-----------|------------|
| `TextSelectionMenu` | Text is selected | Copy, Highlight, Underline, StrikeOut, Search Web, Select All |
| `AnnotationContextMenu` | Annotation is selected | Open, Delete, Properties, Set Color, Add Reply |
| `RedactionContextMenu` | Redaction mark is selected | Apply Redaction, Remove, Properties |
| `PageContextMenu` | Right-click on page (no selection) | Zoom In, Zoom Out, Fit Page, Fit Width, Rotate CW, Rotate CCW |
| `TabContextMenu` | Right-click on tab | Close, Close Others, Close All, Reveal in Explorer |
| `FormFieldContextMenu` | Right-click on form field | Cut, Copy, Paste, Select All, Undo |

### 6.3 Keyboard Shortcuts (Default Mapping)

| Shortcut | Action | Context |
|----------|--------|----------|
| Ctrl+O | Open file | Global |
| Ctrl+S | Save | Global (when document modified) |
| Ctrl+P | Print | Global |
| Ctrl+F | Search | Global |
| Ctrl+Z | Undo | Global (when applicable) |
| Ctrl+Shift+Z | Redo | Global (when applicable) |
| Ctrl+C | Copy selected text | When text selected |
| Ctrl+A | Select all text on page | When no annotation active |
| Ctrl+Plus | Zoom in | Global |
| Ctrl+Minus | Zoom out | Global |
| Ctrl+0 | Reset zoom to FitWidth | Global |
| Home | First page | Global |
| End | Last page | Global |
| Page Up / Page Down | Scroll by page | Global |
| Left / Right arrows | Navigate page (when not in text field) | Global |
| Escape | Deselect / close popup | Global |
| H | Hand tool (View mode) | View mode |
| V | Select tool | View mode |
| 1-9 | Select annotation type | Annotate mode |

> **FACT:** The current `HotkeyContext` stores customizations in `localStorage`. In native, store in the preferences JSON file. Shortcuts are loaded at startup and can be edited in the Keyboard Shortcuts dialog.

---

## Appendix A: EmbedPDF Plugin-to-Component Mapping

The current EmbedPDF instance has ~20 plugins that map to UI components:

| EmbedPDF Plugin | UI Component(s) | Native Equivalent |
|----------------|-----------------|-------------------|
| `DocumentManager` | TabBar, FileContext | `DocumentController` |
| `Viewport` | Viewer (canvas area) | `PdfViewerController::viewport_` |
| `Scroll` | Viewer scroll behavior | `ScrollWindowEx` + `WM_VSCROLL`/`WM_HSCROLL` |
| `Render` | Canvas 2D tile rendering | `TileRenderer` (D2D) |
| `InteractionManager` | Click/hover handling | `PdfViewerController::OnMouse*` |
| `Selection` | `TextSelectionOverlay` | `SelectionOverlay` |
| `History` | Undo/redo | `CommandHistory` (C++ Command pattern) |
| `Annotation` | `AnnotationOverlay`, `AnnotationPopup` | `AnnotationOverlay`, `AnnotationController` |
| `Redaction` | `RedactionOverlay`, `RedactionPendingTracker` | `RedactionOverlay`, `RedactionController` |
| `Pan` | Hand tool drag | `PdfViewerController` (WM_MOUSEMOVE drag) |
| `Zoom` | `ZoomControls`, scroll-wheel zoom | `ZoomControls`, WM_MOUSEWHEEL |
| `Tiling` | Tile generation/management | `TileCache` |
| `Spread` | Page spread layout | `PageLayoutController` |
| `Search` | `SearchInterface`, `SearchResultsPanel` | `SearchBar`, `SearchResultsController` |
| `Thumbnail` | `ThumbnailSidebar` | `ThumbnailPanel`, `ThumbnailService` |
| `Bookmark` | `BookmarkSidebar`, `BookmarksTreePanel` | `BookmarkPanel` |
| `Attachment` | `AttachmentSidebar` | `AttachmentPanel` |
| `Rotate` | Rotate buttons in toolbar | `DocumentController::RotatePage` |
| `Export` | Save As dialog | `IFileSaveDialog` + PDFium save |
| `Print` | Print dialog | `PrintDlgEx` + PDFium print |

---

*End of FILE-011: UI_COMPONENTS.md*