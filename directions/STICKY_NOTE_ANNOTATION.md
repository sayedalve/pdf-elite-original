# Sticky Note Annotation Workflow

## Overview
Sticky Note annotations (Type `FPDF_ANNOT_TEXT`) allow users to attach pop-up comments to specific areas of the PDF document.

## Implementation Details

### User Interface
- **Tool Mode**: Activated via `ToolMode::StickyNote`.
- **Text Input**: Shares the same native Windows `EDIT` control foundation as the FreeText tool. This guarantees high-quality text input handling and Unicode compatibility. 
- **Editor Dimensions**: The native control for a Sticky Note defaults to 150x100 screen pixels, providing sufficient space for a standard comment.

### Engine Architecture
- **Coordinate Transformation**: Input points are transformed exactly like other interactive objects.
- **Commit**: The note's contents are serialized as UTF-8, then converted to UTF-16, and stored using the standard `FPDFAnnot_SetStringValue` for the `"Contents"` dictionary entry. 
- **Persistence**: Unlike FreeText, Sticky Notes conventionally require an associated Pop-up annotation or just rely on the PDF viewer to read the "Contents" dictionary upon hover/click. PDFium persists the `Text` annotation type accurately, and standard PDF readers will recognize the sticky note icon and display the `Contents`.

### Known Limitations
- The custom popup UI for reading existing sticky notes outside of the `EDIT` state is pending full UI-layer integration. The data is accurately saved to the PDF document, but the viewer needs an interactive popup surface to read it back elegantly during normal `ToolMode::Select` interaction.
