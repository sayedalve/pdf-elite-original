# Final UX Audit

## 1. Precision Scrolling
1. **Current behavior**: Scrolling with high-resolution trackpads truncates wheel deltas, resulting in jagged or dropped scroll events.
2. **Why it feels poor**: Makes continuous reading and fast skimming feel disjointed and unresponsive on modern laptops.
3. **Expected behavior**: Application should preserve floating-point delta values and accumulate them for smooth, pixel-perfect scrolling.
4. **Proposed change**: Modified PdfViewer::OnScroll and MainWindow to track a float m_scrollAccumulator, transferring precise fractional amounts to m_scrollY.
5. **Priority**: High

## 2. Tooltips for Icon-Only Buttons
1. **Current behavior**: Icon-only buttons (like Undo, Redo, Zoom, Note, OCR) have no text or hover tooltips.
2. **Why it feels poor**: Users must guess the function of abstract icons, slowing down feature discovery.
3. **Expected behavior**: Hovering over an icon-only button should reveal its name via a tooltip.
4. **Proposed change**: Added GetTooltipText() to UIElement and IconButton. Added a native D2D tooltip overlay to MainWindow::WM_PAINT that tracks the hovered element.
5. **Priority**: High

## 3. Tab Bar Text Clipping
1. **Current behavior**: Long filenames in the Tab Bar overflow and clip abruptly at the tab boundary, sometimes cutting letters in half.
2. **Why it feels poor**: Looks unpolished and broken.
3. **Expected behavior**: Long text should elegantly fade or truncate with an ellipsis (...).
4. **Proposed change**: Configured m_navigation text format in NativeDesignSystem with DWRITE_TRIMMING_GRANULARITY_CHARACTER and an ellipsis trimming sign.
5. **Priority**: Medium

## 4. Cursor Feedback
1. **Current behavior**: Missing contextual cursors (e.g. no I-Beam for text editing, no resize cursors for annotations).
2. **Why it feels poor**: The application feels unresponsive to interactive elements.
3. **Expected behavior**: Mouse cursor should dynamically update based on the underlying object or tool mode.
4. **Proposed change**: Integrated InteractionManager::GetCursor() into MainWindow::WM_SETCURSOR to return standard Win32 cursors (IDC_IBEAM, IDC_SIZENWSE, etc.).
5. **Priority**: High

## 5. File Picker Experience
1. **Current behavior**: Missing file extension filters or path validations.
2. **Why it feels poor**: Users might select unsupported files, leading to errors or crashes.
3. **Expected behavior**: File picker should strictly filter for PDF files.
4. **Proposed change**: (Completed in Phase 1) OpenFile enforces OFN_FILEMUSTEXIST and strictly filters *.pdf.
5. **Priority**: High
