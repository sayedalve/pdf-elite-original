# Subsystem Migration Report: ViewportEngine & Focal Zoom Invariants

## 1. Subsystem Overview
- **Module:** `ui::viewport::ViewportEngine`
- **Location:** `native/src/ui/include/viewport/ViewportEngine.h`, `native/src/ui/src/viewport/ViewportEngine.cpp`
- **Scope:** Continuous multi-page document layout caching, focal-point preserving zoom invariant math, exponential zoom steps, and fit mode computations (`FitWidth`, `FitPage`, `FitHeight`).

## 2. Reference Project & Clean-Room License Audit
- **Inspiration Source:** PDF4QT (`PageViewPort`, focal zoom anchoring) & Okular (`PageView`)
- **Reference License:** LGPL-3.0 (PDF4QT) / GPL-2.0+ (Okular)
- **License Decision:** STRICT CLEAN-ROOM REIMPLEMENTATION. Mathematical formulations for focal-point zoom offsets and continuous page stacking were independently implemented from first principles in C++20.

## 3. Architecture & Adaptations Made
- **Focal Point Invariant Math:**
  $$\text{docX} = \frac{\text{focalScreenX} + \text{scrollX} - \text{pageOffsetX}}{\text{currentZoom}}$$
  $$\text{newScrollX} = \text{docX} \cdot \text{newZoom} - (\text{focalScreenX} - \text{pageOffsetX})$$
  Ensures that the exact pixel under the user's cursor remains anchored at the identical screen location before and after zoom.
- **Continuous Multi-Page Layout & Contiguous Gap Mapping:** Stacks pages vertically with configurable padding gaps, precomputes scaled dimensions, and provides fast $O(1)$/$O(\log N)$ visible page range queries. `GetPageAtOffset` partitions the vertical coordinate space into contiguous intervals $[y_i, y_{i+1})$, correctly mapping inter-page gaps to the preceding page rather than falling through to the document end.
- **Exponential Zoom Steps:** Scales zoom by factors of $1.15^k$, preserving geometric stepping across zoom ranges ($1\%$ to $6400\%$).
- **Preset Zoom Level Snapping:** Snaps through standard document publishing ratios ($25\%$, $50\%$, $75\%$, $100\%$, $125\%$, $150\%$, $200\%$, $300\%$, $400\%$, etc.).

## 4. Old Code Removed / Superseded
- Replaced ad-hoc layout loops and imprecise zoom origin calculations in `PdfViewer.cpp`.
- Replaced non-contiguous page hit-testing in `GetPageAtOffset` that suffered from inter-page gap fallthrough.
