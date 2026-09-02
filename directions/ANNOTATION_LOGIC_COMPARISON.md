# Annotation Logic Comparison

This document details the annotation architectures of Xournal++ and PDF4QT, and how they map to PDF Elite's goals (Phase 3).

## Reference Architectural Overview

### 1. Xournal++ (Proprietary Element Model)
Xournal++ uses an abstract `Element` model (`src/core/model/Element.h`) for all document objects.
* **Creation Model:** Highly interactive. Input from stylus/mouse is captured by handlers (e.g., `StrokeHandler`), run through a `StrokeStabilizer` to reduce jitter, and baked into a `Stroke`.
* **Selection Model:** Hit testing against strokes and bounding boxes.
* **Geometry Model:** Points, pressure arrays, and contour widths (`StrokeContour.h`).
* **Rendering Model:** Rendered dynamically using Cairo onto a transparent overlay layer (`StrokeView.h`).
* **PDF Persistence Model:** None natively. Annotations are persisted into an XML `.xopp` file. Exporting to PDF converts strokes to PDF paths, but the source of truth is XML.
* **Undo Model:** Explicit `UndoAction` objects (`AddUndoAction`, `EraseUndoAction`).
* **Interaction Model:** Direct manipulation, low latency, robust stylus support.

### 2. PDF4QT (Native PDF Model)
PDF4QT operates directly on the PDF specification.
* **Creation Model:** GUI interactions create native `PDFAnnotation` instances mapped to standard PDF dictionaries (`Text, Ink, Square, Circle, Highlight`, etc.).
* **Selection Model:** Geometric hit testing mapped to PDF page coordinates.
* **Geometry Model:** QuadPoints, Rects, and Path arrays strictly adhering to the PDF spec.
* **Rendering Model:** Parses the PDF object's Appearance Stream (`/AP`) and renders it via Qt's `QPainter`.
* **PDF Persistence Model:** 100% native. Edits update the PDF dictionaries and are saved via incremental or full rewrites.
* **Undo Model:** Model-view updates tracked via Qt's QUndoStack.
* **Interaction Model:** Object-based editing (select object, drag handles to resize).

---

## Detailed Mapping to PDF Elite

The goal for PDF Elite is to synthesize **Xournal++'s Interaction & Creation** with **PDF4QT's Persistence & Geometry**.

### 1. Freehand Ink & Shapes
* **Creation & Interaction (Xournal++):** We will adapt Xournal++'s point collection, `StrokeStabilizer`, and pressure handling to provide a smooth, modern drawing experience in our Direct2D UI.
* **Geometry & Persistence (PDF4QT):** Once the user commits a stroke (lifts the pen), the stabilized geometry will be serialized into a native `/Ink` PDF annotation (or `/Square`, `/Circle` for shapes), generating standard PDF Appearance Streams.
* **Mapping:**
  - `InteractionManager` -> Handles the live preview (Xournal++ style).
  - `IAnnotation / PdfAnnotation` -> Wraps the generated PDFium `/Ink` dictionary (PDF4QT style).

### 2. Text Markup (Highlight, Underline, Strikethrough)
* **Creation & Selection (Okular / PDF4QT):** Rely heavily on text quad-point extraction to snap markup directly to underlying text glyphs.
* **Persistence:** Native `/Highlight`, `/Underline`, `/StrikeOut` annotations.
* **Mapping:**
  - `AnnotationSelectableObject` -> Binds to the text quad boundaries.

### 3. Comments and Sticky Notes
* **Creation:** Standard pop-up text entry.
* **Persistence:** Native `/Text` annotations with standard icons (Note, Comment, Key).
* **Mapping:**
  - Bind to native UI popup boxes overlaying the page.

### 4. Hit Testing, Editing, and Deletion
* **Hit Testing:** Perform geometric intersection (point-in-rect or point-in-path) mapped via PDF Elite's `CoordinateConverter`.
* **Editing/Deletion:** Updating or deleting the native PDFium annotation dictionary (`FPDFPage_RemoveAnnot`).
* **Undo/Redo:** Every committed annotation change will yield a specialized Command in the `CommandStack` (e.g., `AddAnnotationCommand`, `DeleteAnnotationCommand`), storing the serialized PDF dictionary state for restoration.

## Summary of Integration Strategy
Do not replicate Xournal++'s proprietary DOM and separate storage. Instead, adapt its high-fidelity input algorithms. Apply these algorithms directly to PDF Elite's `PdfAnnotation` wrappers around PDFium, ensuring everything is natively compliant and saved directly into the PDF.
