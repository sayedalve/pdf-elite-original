# Page Operations Test Results

This document tracks the results of the page operations regression tests run inside `RegressionSuite.exe`.

## Automated Test Coverage

| Test Area | C++ Test Name | Status |
| --- | --- | --- |
| **Delete Page** | `Page_DeletePage` | PASS |
| **Insert Page** | `Page_InsertBlankPage` | PASS |
| **Duplicate Page** | `Page_DuplicatePage` | PASS |
| **Move/Reorder** | `Page_MovePage` | PASS |
| **Rotate Page** | `Page_RotatePageAPI` | PASS |
| **Extract Pages** | `Document_ExtractPages` | PASS |
| **Merge Documents** | `Document_MergeInsertPagesFrom` | PASS |

## Validation Notes
- **Empty / Last Page Restrictions**: `Page_DeletePage` automatically validates that attempting to delete index 0 on a single-page document returns `false`.
- **Dimensions Preservation**: `Page_MovePage` extracts standard page boundaries natively and confirms they move unmodified.
- **Save Hooks**: Document instances modified via operations save cleanly via `PdfDocument::SaveAs`.
- **Thumbnail Updates / Viewer state**: Verified indirectly via UI invalidation methods hooked via `PdfViewer` logic (`RecalculateLayout()`, `InvalidateAll()`).

## Limitations
- Extraction does not strip all internal resources perfectly if the subset page is highly coupled.
- Undo/Redo is not wired; the engine correctly fails early if parameters are invalid to prevent destructive non-reversible behaviors.
