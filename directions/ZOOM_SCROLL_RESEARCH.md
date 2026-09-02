# Zoom and Scroll Research

This document outlines the findings for Phase 7, focusing on Zoom, Scroll, and Viewport Calculation derived primarily from Okular's mature viewport architecture.

## 1. Cursor-Centered Zoom (`zoomWithFixedCenter`)
**Strongest Reference:** Okular
**Location:** `part/pageview.cpp`

**Concept:**
Okular implements an extremely stable `zoomWithFixedCenter` function. 
* It intercepts mouse wheel + Ctrl (or pinch-to-zoom gestures).
* It captures the absolute mouse position mapped to the underlying page (`contentAreaPoint`).
* After the scale factor changes, it recalculates the scrollbar offsets (`horizontalScrollBar()->setValue`) such that the exact pixel on the document beneath the cursor remains stationary on the screen.

**Adaptation for PDF Elite:**
* We must adapt this exact math into PDF Elite's mouse wheel handler. The user should never lose their focus point when zooming. The `CoordinateConverter` will be responsible for locking the target coordinate, scaling the viewport, and mapping the offset back to the native Windows scrollbars.

## 2. Scroll Anchoring & Continuous Mode
**Strongest Reference:** Okular

**Concept:**
* Okular's `PageView` handles continuous mode smoothly by calculating an accumulated bounding box of all pages (`page->croppedGeometry()`). 
* Scrollbars are mapped to this massive virtual canvas.
* `singlePageWheelAccumulatedDelta` tracks residual wheel deltas in non-continuous mode to trigger discrete page flips once a threshold is crossed.

**Adaptation for PDF Elite:**
* PDF Elite already supports continuous scrolling. We will improve it by adopting Okular's "accumulated delta" technique for single-page discrete mode, preventing accidental page flips on overly sensitive trackpads.

## 3. Viewport Calculation & Lazy Rendering
**Strongest Reference:** Okular

**Concept:**
* Instead of rendering the whole page, Okular calculates the exact `visibleRect` (the intersection of the `QAbstractScrollArea` viewport and the page geometry).
* This rect is passed to the `TilesManager`, which immediately issues render requests strictly for tiles intersecting this rect, with a padding radius for prefetching.

**Adaptation for PDF Elite:**
* PDF Elite's `PdfViewer` will compute the strict visible intersection rect on every `WM_VSCROLL` / `WM_HSCROLL`.
* We will only dispatch jobs to the `RenderWorker` for the intersecting tiles, instantly canceling background render requests for tiles that have scrolled out of bounds.

## Summary
The combination of `zoomWithFixedCenter` math, "accumulated delta" thresholding for trackpads, and strict viewport-intersect lazy rendering forms the gold standard for PDF reader navigation. PDF Elite will adopt these exact mathematical models into its Direct2D/Win32 scroll framework.
