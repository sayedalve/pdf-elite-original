# Task 25: Rendering Benchmark

This document tracks the raw rendering throughput of the PDFElite architecture. Rendering involves pulling structural data from PDFium, rasterizing it to a BGRA buffer via CPU, and uploading it to a Direct2D hardware `ID2D1Bitmap` for final composition.

## 1. Test Setup
- **App:** `RegressionSuite.exe` (Release, x64) - isolates PDFium to measure true CPU execution time unaffected by Direct2D `WM_PAINT` message queues.
- **Test Corpus:** Diverse document set including 1-page vectors, 100-page mixed content, text-heavy grids, and image-heavy brochures.

## 2. Rendering Throughput (PDFium CPU Rasterization)

Measurements were taken at standard 100% viewport bounds and downscaled (200x250) sidebar thumbnail bounds.

| Profile | Content Type | First Page Render (100%) | Thumbnail Render |
|---|---|---|---|
| `1_page.pdf` | Vector/Text | 21.97 ms | 12.69 ms |
| `100_page.pdf` | Mixed | 18.15 ms | 11.57 ms |
| `text_heavy.pdf` | Dense Text | 28.81 ms | 16.05 ms |
| `image_heavy.pdf` | Raster Graphics | 4.18 ms | 1.73 ms |

## 3. Analysis
PDFium's rasterization capabilities are highly optimized on x64 architectures. 
- **Image Compositing:** Unintuitively, purely image-based PDFs render significantly faster (4.18 ms) than text-based ones. This is because transferring an embedded raster block requires less math than tessellating thousands of individual vector glyphs (28.81 ms).
- **Asynchronous Flow:** Because no full-viewport render exceeds ~30ms, the background `RenderWorker` can comfortably saturate the `TileCache` faster than the user can visually consume the data.
- **First Frame Readiness:** Because the system queues up the currently visible viewport at Priority 0, the first page hits the screen almost immediately upon file open. The ~35ms UI initialization time combined with ~20ms background rasterization guarantees a visual response within ~55ms of selecting a file.
