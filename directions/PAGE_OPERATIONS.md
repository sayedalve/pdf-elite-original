# Page Operations Architecture

The PDF Elite page operations layer allows structural modification of loaded PDFs through the native C++ engine, guaranteeing robust memory management and reliable state syncing.

## 1. Engine Layer (`pdf_engine`)
The `IDocument` interface has been extended with the following operations:
- `DeletePage(int index)`
- `InsertBlankPage(int index, double width, double height)`
- `DuplicatePage(int index)`
- `MovePage(int sourceIndex, int destIndex)`
- `RotatePage(int index, int rotationDegrees)`
- `ExtractPages(const std::vector<int>& indices)`
- `InsertPagesFrom(IDocument* sourceDoc, const std::vector<int>& sourceIndices, int destIndex)`

These APIs map directly to stable PDFium abstractions such as `FPDF_ImportPagesByIndex` and `FPDFPage_Delete`. The PDFium API is completely encapsulated; no raw pointers or `FPDF_` handles leak into the UI tier.

## 2. Page Identity and Caches
Operations that modify page order (Insert, Delete, Duplicate, Move) invalidate text search caches, selection states (`InteractionManager`), and trigger a complete `RecalculateLayout()` via `PdfViewer`. `TileCache::InvalidateAll()` is dispatched to clear out texture memory for shifted pages, ensuring no ghosting.
Operations that merely alter metadata (Rotate) selectively invoke `TileCache::InvalidatePage(index)`.

## 3. Save Safety
Constructive/Destructive operations mutate the active in-memory PDFium `FPDF_DOCUMENT`. All data is preserved into a valid PDF stream atomically when `SaveAs` commits via temporary files.

## 4. Testing & Validation
All page operations have automated validation cases within `RegressionSuite.cpp`.
- Creation of correct page counts
- Correct insertion sizing
- Movement bounds tracking
- Rotation attribute persistence

## 5. Performance Context
By executing through PDFium's `FPDF_MovePages` and `FPDF_ImportPagesByIndex`, the heavy lifting is handled intrinsically at the C-layer without decoding the full page DOM structures. Memory allocations remain low.

## 6. Undo/Redo Strategy
All `PdfDocument` mutation points accept well-typed parameters, enabling straightforward extraction into a Command pattern architecture for Undo/Redo in a future task.
