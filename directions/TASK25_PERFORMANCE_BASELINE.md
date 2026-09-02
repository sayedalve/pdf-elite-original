# Task 25: Performance Baseline

This document captures the initial performance metrics for PDF Elite (Release Build) before optimization. All measurements were captured via exact timing logs using `std::chrono::high_resolution_clock` and automated test fixtures.

## Methodology
- **Build Configuration**: `Release` (x64, C++20, MSVC)
- **PDF Engine**: PDFium
- **Test Files**: Generated using `reportlab` inside `tests/fixtures/perf/`:
  - `1_page.pdf` (Minimal text)
  - `10_page.pdf` (Mixed content)
  - `50_page.pdf` (Mixed content)
  - `100_page.pdf` (Mixed content)
  - `text_heavy.pdf` (Dense text, 10 pages)
  - `image_heavy.pdf` (Large image fills, 10 pages)
- **Measurement Strategy**: 
  - `PdfViewer` UI rendering and input handling measured dynamically using Windows messages and direct instrumentation.
  - Raw PDFium rendering isolated and timed synchronously to remove Direct2D/OS composition artifacts from background test limitations.

---

## 1. Core Timings (Averages)

### Application & Document Load
- **App Startup (WinMain -> ShowWindow):** ~6 - 8 ms
- **OpenFileDirect (Parse + Layout + UI Init):** ~24 - 35 ms
- **Document Load (Raw PDFium parsing):** ~0.5 - 0.9 ms

### Rendering (First Page / Synchronous Block)
| Document | 100% Scale Render | Thumbnail Render (200x250) |
|---|---|---|
| `1_page.pdf` | 21.97 ms | 12.69 ms |
| `10_page.pdf` | 18.41 ms | 20.27 ms |
| `50_page.pdf` | 30.88 ms | 14.01 ms |
| `100_page.pdf` | 18.15 ms | 11.57 ms |
| `text_heavy.pdf` | 28.81 ms | 16.05 ms |
| `image_heavy.pdf` | 4.18 ms | 1.73 ms |

### UI & Interaction
- **PdfViewer_Render (Direct2D Draw Loop):**
  - **Cold Draw (waiting on worker tiles):** ~15 - 28 ms
  - **Warm Draw (cached tiles):** ~0.7 - 2.5 ms
- **PdfViewer_RenderThumbnails (Sidebar):** ~0.02 - 0.05 ms (Consistently fast, drawn via hardware acceleration)
- **Scroll Response (`PdfViewer_OnScroll`):**
  - Varies wildly depending on document size.
  - Average: ~10 - 20 ms per scroll tick.
  - Worst case (`100_page.pdf`): **42.37 ms** (Stutters user experience).

---

## 2. Metric Analysis & Observations

1. **Rendering is actually quite fast:** PDFium raw rendering times for standard pages stay under 30ms. Image rendering is unexpectedly fast (4ms) because the PDF doesn't have complex vector tessellation. Text-heavy rendering takes the longest due to glyph caching and vector drawing.
2. **Scrolling is the primary bottleneck:** `OnScroll` routinely takes 15-40ms on large documents. Because scroll events fire rapidly (every few milliseconds on modern mice/trackpads), synchronous 40ms blocks will cause dropped frames and severe input lag. 
3. **App Startup and Load are excellent:** A native C++ startup of 6ms and an open time of 30ms is effectively instant for the user. We do not need to heavily prioritize load times; they are already optimal.
4. **Thumbnail overhead is negligible:** Due to downscaling, rendering thumbnails is fast (10-20ms raw) and extremely fast to paint to the screen (0.02ms). 

---

## 3. Next Steps (Task 25 Focus Areas)

Based on these baselines, our optimizations should target the specific pain points:
1. **Scroll Coalescing & Asynchronous Layout**: `OnScroll` must not block for 40ms. Scroll events must be coalesced, and page layout intersection calculations should be optimized (e.g., binary search for visible pages instead of linear scans).
2. **Tile Cache Optimization**: Ensure the `RenderWorker` prioritizes the immediately visible viewport first and preemptively caches adjacent pages efficiently without choking the CPU.
3. **Memory Analysis**: We need to establish memory footprint baselines for the loaded tile caches and document states before optimizing memory usage.
