# Annotation System Completion Architecture Review

## Core Principles
The PDF-Elite annotation system employs a unified abstraction via `IAnnotation`. All annotation types (Highlight, Underline, Strikeout, FreeText, and Sticky Note) utilize the same exact memory lifecycle and bridge perfectly to the UI via `AnnotationSelectableObject`.

## Abstraction Compliance
- **Ownership & Lifetime**: `PdfAnnotation` holds the PDFium `FPDF_ANNOTATION` handle. The handle is safely tied to the page lifecycle and does not leak.
- **Selection & Interaction**: `InteractionManager` treats all annotations as standard `ui::interaction::ISelectableObject` objects.
- **Rendering**: The PDFium engine rasterizes the annotations natively. The system deliberately prevents "fake" custom UI overlays from masquerading as PDF annotations; everything committed must survive a roundtrip serialize cycle.
- **Save & Reopen**: Native dictionary persistence operates through `PdfDocument::SaveAs`. No side-car files or separate UI metadata is required.
- **Text & Unicode**: `PdfAnnotation::SetContents()` natively bridges UTF-8 strings into the required UTF-16LE payload for `FPDF_WIDESTRING`. Bangla and other complex scripts pass flawlessly into the PDF standard representation.

## Architectural Discoveries
- PDFium `Line` geometry manipulation lacks standard C APIs for coordinate setting (`FPDFAnnot_SetLine`), necessitating careful abstraction limits on complex drawings without raw dictionary bridging.
- By isolating the `EDIT` control logic inside `PdfViewer` but delegating the text serialization to `PdfAnnotation`, the engine remains 100% agnostic to Win32 `HWND` controls.

## Conclusion
The fundamental annotation architecture is stable and robust. All core text-based annotation tools have unified around standard patterns.
