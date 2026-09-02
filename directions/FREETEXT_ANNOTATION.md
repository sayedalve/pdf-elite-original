# FreeText Annotation Workflow

## Overview
FreeText annotations (Type `FPDF_ANNOT_FREETEXT`) allow users to type text directly onto the PDF. Unlike modifying the underlying PDF text (which is a page layout operation), FreeText annotations act as independent layered objects over the document.

## Implementation Details

### User Interface
- **Tool Mode**: Activated via `ToolMode::FreeText`.
- **Text Input**: Utilizes a native Windows `EDIT` control configured with `WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN`. This ensures Unicode compatibility (including complex scripts like Bangla via Uniscribe) and standard Windows keyboard shortcuts.
- **Preview Integration**: The native `EDIT` control acts as both the input and the immediate preview surface.

### Engine Architecture
- **Coordinate Transformation**: Input events are captured in screen coordinates and transformed precisely into PDF coordinates via `CoordinateConverter::ScreenToPdf`.
- **Commit**: Upon losing focus or hitting `Ctrl+Enter`, the contents of the native `EDIT` control are serialized as UTF-8.
- **Persistence**: `PdfAnnotation::SetContents()` converts UTF-8 back to UTF-16 and stores the payload using `FPDFAnnot_SetStringValue(..., "Contents", ...)`. The bounding box (`FPDFAnnot_SetRect`) is set to closely match the `EDIT` control dimensions scaled to PDF units.

### Known Limitations
- Full native rich-text editing (varying fonts within the same box) is not implemented.
- Alignment properties and font size overrides are pending full `FPDFAnnot_SetAP` (Appearance Stream) generation if native PDFium defaults are insufficient for advanced styling.
