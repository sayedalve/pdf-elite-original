# IMAGE ENGINE

## Architecture

The PDF Elite Native engine handles images via a unified architecture integrating WIC (Windows Imaging Component), PDFium, and Direct2D.

### 1. Image Object Abstraction
The `IImage` interface abstracts image components, implemented natively by `PdfImage`. 
`PdfImage` encapsulates a PDFium `FPDF_PAGEOBJECT` of type `FPDF_PAGEOBJ_IMAGE`.

### 2. Loading and Decoding
Images are loaded via `WicImageLoader`, which decodes BMP, PNG, JPEG, and Clipboard data into BGRA 32-bit buffers. These buffers are injected directly into PDFium via `FPDFImageObj_SetBitmap`.

### 3. Coordinate Systems
PDFium operates on a Y-up coordinate system. The `CoordinateConverter` handles bi-directional mappings between the viewer's screen space and the PDF's logical space. 

### 4. Direct2D Rendering
The actual rendering of the PDF page with the image is performed by PDFium to a bitmap, which is then drawn to the screen using Direct2D. During interactive dragging, a preview bounding box is rendered directly via Direct2D for immediate responsive feedback.
