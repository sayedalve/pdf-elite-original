# Drawing and Direct Manipulation Research

This document outlines the findings for Phase 5, focusing on Drawing, Direct Manipulation, and Interaction Techniques derived primarily from Xournal++.

## 1. Freehand Strokes & Smoothing (Xournal++)
**Strongest Reference:** Xournal++
**Location:** `src/core/control/tools/StrokeStabilizer.cpp`

**Concept:** 
Xournal++ achieves best-in-class stroke smoothing using a dedicated `StrokeStabilizer` pipeline.
It processes raw stylus/mouse events (`PositionInputData`) through:
* **Preprocessors:** `Deadzone` (ignores micro-movements to reduce jitter) or `Inertia` (simulates physical mass/drag for buttery smooth curves).
* **Averaging Methods:** `Arithmetic` (moving average) or `Velocity Gaussian` (adjusts smoothing weight based on drawing speed).

**Adaptation for PDF Elite:**
* We will port these stabilization math models (`VelocityGaussianInertia`, etc.) into our native C++ `InteractionManager`.
* **Lightweight Native Preview:** During the `OnMouseMove` phase, the stabilized points will be rendered immediately using Direct2D (hardware accelerated) as a temporary overlay layer. 
* **Commit:** Expensive PDFium mutations (`FPDFPage_InsertObject`, path construction) will **only** occur on `OnMouseUp`. The temporary Direct2D overlay is then discarded, and the actual PDF annotation is rendered.

## 2. Shape Recognition & Creation
**Strongest Reference:** Xournal++
**Location:** `src/core/control/shaperecognizer/ShapeRecognizer.cpp`

**Concept:**
Xournal++ includes an explicit `ShapeRecognizer` engine that evaluates collected points after a stroke is completed. It calculates bounding boxes, error margins, and geometric distance to snap rough drawings into perfect Circles, Lines, or Polygons.

**Adaptation for PDF Elite:**
* Shape recognition will be integrated as a post-processing step before the stroke is committed to PDFium.
* If shape recognition is enabled and successful, the raw points are discarded, and a native PDF `/Square`, `/Circle`, or `/Line` annotation is injected instead of an `/Ink` annotation.

## 3. Object Selection, Move, Resize, Rotate
**Strongest Reference:** Xournal++ (Interaction) & PDF4QT (Object Bound)

**Concept:**
* Xournal++ handles direct manipulation flawlessly using a generic `Element` bound box. When selected, the UI draws selection handles.
* Dragging these handles issues continuous geometric transform matrices (`ScaleUndoAction`, `MoveUndoAction`), applying live visual updates without touching the persistent storage.

**Adaptation for PDF Elite:**
* We will adopt the 8-point selection handle interaction model for PDF Elite's Direct2D canvas.
* As the user drags handles to resize or rotate, we apply an affine transformation (`D2D1::Matrix3x2F`) to the preview bitmap or vector paths.
* The PDF engine only receives the final `FPDFPath_SetMatrix` mutation when the interaction ends, preserving UI thread performance and avoiding PDFium locking issues on every tick of the mouse.
