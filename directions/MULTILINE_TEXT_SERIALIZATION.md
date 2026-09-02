# Multiline Text Serialization Strategy

## Objective
To preserve the logical grouping of a multiline text block after a document is saved and reopened, while remaining compliant with the PDF specification and maintaining compatibility with standard PDF viewers.

## Options Evaluated

### A. Standard PDF Marked Content (BDC/EMC)
- **Mechanism**: PDF supports tagging sequences of page objects using Marked Content operators (`BDC` and `EMC`).
- **PDFium Support**: Fully supported via `FPDFPageObj_AddMark`, `FPDFPageObjMark_SetStringParam`, and `FPDFPageObjMark_GetParamStringValue`.
- **Pros**: Standards-compliant, highly interoperable. Other editors will safely ignore our proprietary tags, and our tags will survive basic round-tripping.
- **Cons**: Slightly complex to manage tag handles.

### B. Custom PDF Dictionaries (Non-standard)
- **Mechanism**: Attaching custom dictionaries to Font objects or XObjects.
- **Pros**: None in this context.
- **Cons**: PDFium doesn't expose easy public APIs for this, and it breaks PDF compliance.

### C. Heuristic Grouping (Position/Font based)
- **Mechanism**: At load time, find text objects with identical fonts and close vertical proximity, and group them.
- **Pros**: Works on any PDF.
- **Cons**: Highly fragile. A user might naturally have two distinct paragraphs close to each other. Reflow would improperly merge them.

## Chosen Strategy: Option A (Standard Marked Content)

We will use PDF Marked Content to explicitly tag multiline paragraphs created or edited by PDF Elite.

### Implementation Details

1. **Tag Name**: `PDFElite_TextBlock`
2. **Metadata**: A unique string parameter `BlockID` containing a UUID generated at the time of the block's creation.
3. **Serialization (Save)**: 
   When `SetLines` or a multiline command creates multiple `FPDF_PAGEOBJECT`s to represent wrapped lines, each object will receive the same `PDFElite_TextBlock` mark and `BlockID`.
4. **Deserialization (Reopen)**:
   Inside `PdfPage::GetTextObjects()`, we will iterate through all page objects.
   - If a text object lacks the `PDFElite_TextBlock` mark, it is wrapped in a standalone `PdfTextObject` (legacy behavior).
   - If a text object has the mark, we read the `BlockID`. We group all objects sharing the same `BlockID` into a single `PdfTextObject` instance, preserving the multiline logical structure.

This approach ensures robust paragraph identity across save-reopen cycles while respecting the PDF specification.
