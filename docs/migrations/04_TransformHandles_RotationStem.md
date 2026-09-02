# Subsystem Migration Report: TransformHandles & Rotation Stem Mathematics

## 1. Subsystem Overview
- **Module:** `ui::selection::TransformHandles`
- **Location:** `native/src/ui/include/selection/TransformHandles.h`, `native/src/ui/src/selection/TransformHandles.cpp`
- **Scope:** 8-way resize handle geometry generation, rotation stem handle placement, angle snapping, inverse affine matrix hit testing, and Direct2D handle/marquee rendering.

## 2. Reference Project & Clean-Room License Audit
- **Inspiration Source:** PDF4QT (`PDFTransformHandles`, 8-way handle math, rotation stem) & Xournal++ (`SelectionTransform`)
- **Reference License:** LGPL-3.0 (PDF4QT) / GPL-2.0+ (Xournal++)
- **License Decision:** STRICT CLEAN-ROOM REIMPLEMENTATION. Mathematical formulations and geometry layout algorithms were independently derived and implemented in C++20 with Direct2D drawing commands.

## 3. Architecture & Adaptations Made
- **Constant DIP Footprint:** Handles render at a fixed 8 DIP dimension regardless of document zoom ($10\%$ to $6400\%$) with a 5 DIP hit-testing tolerance.
- **8-Way Symmetrical Layout:** Generates `NW`, `N`, `NE`, `E`, `SE`, `S`, `SW`, `W`, `Rotation`, and `Body` hit regions.
- **Rotation Math & Snapping:** Calculates pointer angles via `atan2`, rotates handle positions around object center, and supports $15^\circ$ angle quantization when `Shift` is held.
- **Inverse Matrix Hit Testing:** Uses inverted $3\times2$ affine transformation matrices to transform pointer points into local object coordinate space for precise rotated hit detection.

## 4. Old Code Removed / Superseded
- Replaced fixed-pixel handle drawing in legacy `InteractionManager.cpp`.
