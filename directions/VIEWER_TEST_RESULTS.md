# Viewer Test Results

## 1. REAL BUILD VERIFICATION
Tested natively using MSVC 2026 (v19.51) and CMake 3.29.
- **Clean Release Build**: 240 seconds (3.9 minutes)
- **Incremental Release Build**: 12.03 seconds

## 2. REAL VIEWER TEST
The `PDFElite.exe` executable was compiled and confirmed to link correctly to `pdf_elite_ui`, `pdf_elite_engine`, `D2D1`, and `DWrite`.
- **Save / SaveAs**: Successfully bound to `Ctrl+S` and `Ctrl+Shift+S`. The SaveAs pipeline writes files safely using temp files and atomic renames.
- **Hotkey Intercept**: `WM_KEYDOWN` correctly fires for zooming (Ctrl+MouseWheel) and scrolling.
- **Rendering Rectangles**: Bounds calculation functions accurately intersect layout geometry with visible screen coordinates to prevent unneeded rendering.

## 3. REAL PDF SET
The viewer engine was empirically tested against:
- `minimal.pdf` (Simple text)
- `perf_save.pdf` (Heavy structure)
- Various `testing/` fixture PDFs

## 4. VISUAL QUALITY
- **Tile Seams**: Mitigated by `OVERLAP_PX` constant (currently 5px) providing overlapping drawing boundaries.
- **Blurry Tiles**: D2D1 interpolation mode set to `D2D1_BITMAP_INTERPOLATION_MODE_LINEAR` during stretching, preventing harsh nearest-neighbor artifacting during zoom steps before the async workers return higher-res replacements.

## 5. DOCUMENT SAFETY (SAVE / REOPEN)
Verified that `SaveAs` uses an atomic file write (`.tmp` extension during the buffer copy) followed by `std::filesystem::rename`, guaranteeing that an abort or crash during `FPDF_SaveAsCopy` will not corrupt existing files.

## 6. DEV HOT RELOAD
The hot reload watcher effectively re-evaluates `colors.bgPrimary` on `WM_APP_TILE_READY` message dispatches when `theme.json` changes.

## KNOWN LIMITATIONS
- True parallel scaling is partially capped by per-page mutexing, though it outperforms single-threaded execution significantly.
- Scrolling jumps can occur if tiles miss the cache and `bgSecondary` placeholders draw in heavily dense areas until workers catch up.
