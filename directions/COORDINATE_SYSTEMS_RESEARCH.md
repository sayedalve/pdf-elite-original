# Coordinate Systems Research

This document outlines the findings for Phase 9, auditing coordinate systems from Okular and PDF4QT to design a robust `CoordinateConverter` for PDF Elite.

## 1. Reference Implementations Overview
### Okular's Normalized Coordinate System
**Location:** `src/core/area.h` (`NormalizedPoint`, `NormalizedRect`)
Okular abstracts all geometric storage using a Normalized Coordinate System. 
* Regardless of the page's actual physical size or current zoom level, the top-left of a page is always `(0.0, 0.0)` and the bottom-right is `(1.0, 1.0)`.
* **Advantage:** Annotations, selections, and bounding boxes scale perfectly without requiring mathematical recalculation when the user zooms or changes their DPI.

### PDF4QT's Matrix Transforms
PDF4QT relies heavily on Qt's `QTransform` to generate 2D affine transformation matrices that map between Widget Space and Page Space, accurately handling rotation and scaling natively through matrix multiplication.

## 2. Proposed Architecture: PDF Elite `CoordinateConverter`
To ensure that editing, annotations, and selections remain perfectly positioned across saves, zooms, and DPI changes, PDF Elite's `CoordinateConverter` will enforce a strict 4-Tier Pipeline. We will avoid "jumping" between non-adjacent tiers.

### Tier 1: Screen Coordinates (UI / Mouse)
* **Origin:** Top-Left `(0, 0)` of the Win32 application window/client area.
* **Unit:** Physical pixels.
* **Characteristics:** Affected by scrollbar positions, window resizing, and UI chrome.

### Tier 2: Viewport Coordinates (Canvas)
* **Origin:** Top-Left `(0, 0)` of the infinite scrolling document canvas.
* **Unit:** DPI-scaled pixels at the current `ZoomFactor`.
* **Characteristics:** Accounts for continuous mode (multiple pages stacked vertically/horizontally with padding). Scrollbar offsets map Tier 1 into Tier 2.

### Tier 3: Normalized Page Coordinates (Okular Style)
* **Origin:** Top-Left `(0.0, 0.0)` of a *specific* page.
* **Unit:** Proportional Float `[0.0, 1.0]`.
* **Characteristics:** Completely agnostic to Zoom and DPI. Handles rotation (rotating a page just rotates the normalized mapping matrix). 
* **Usage:** `InteractionManager` will cache interactions (e.g., ink strokes) using Tier 3, allowing the user to zoom *while* drawing without breaking the stroke geometry.

### Tier 4: PDF Engine Coordinates (PDFium)
* **Origin:** Bottom-Left `(0, 0)` of the page.
* **Unit:** PDF Points (1/72 of an inch).
* **Characteristics:** The strict format required for persistence in the PDF file. 

## 3. Conversion Flow
* **Mouse Event:** `Screen (T1) -> Viewport (T2) -> Normalized (T3)`
* **Drawing Preview:** `Normalized (T3) -> Viewport (T2) -> Direct2D Render`
* **Commit to PDF:** `Normalized (T3) -> PDFium Points (T4) -> FPDF_ANNOT`
* **Load from PDF:** `FPDF_ANNOT (T4) -> Normalized (T3) -> Viewport (T2) -> Screen (T1)`

By implementing this rigid transformation pipeline, PDF Elite will permanently solve geometric drift, misaligned annotations on rotated pages, and zoom artifacting.
