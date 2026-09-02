# Open Source Integration Plan

This document outlines the final implementation roadmap for integrating the researched algorithms and patterns from PDF4QT, Xournal++, and Okular into PDF Elite's native C++ architecture.

**CRITICAL RULE:** Do not implement all phases at once. Each phase must be built, tested, and validated in isolation. No GPL source code will be copied; only the mathematical algorithms and architectural patterns will be independently reimplemented.

---

## Phase A: Reference Project Research (COMPLETED)
* Clone PDF4QT, Xournal++, and Okular.
* Perform license audits and architectural reviews.
* Define target paradigms for Undo, Rendering, Coordinates, and Annotations.
* *Deliverables Generated:* `OPEN_SOURCE_REFERENCE_AUDIT.md`, `FEATURE_COMPARISON.md`, `EDITING_LOGIC_COMPARISON.md`, `ANNOTATION_LOGIC_COMPARISON.md`, etc.

## Phase B: Editing Architecture Foundation
1. Establish the `PDFObjectEditorAbstractModel` (inspired by PDF4QT) to map PDFium dictionary attributes to native C++ structures.
2. Build the 4-Tier `CoordinateConverter` (Screen -> Viewport -> Normalized -> PDFium) based on Okular's `NormalizedPoint` logic.
3. **Validation:** Unit test coordinate transformation matrices across different zoom levels and page rotations.

## Phase C: Command and Undo Architecture
1. Refactor `CommandStack` to utilize the Granular Command Pattern (inspired by Xournal++).
2. Implement specific command classes (`TransformObjectCommand`, `AddAnnotationCommand`).
3. Wire the commands to serialize/deserialize raw PDFium dictionary states (`FPDF_ANNOT`) for perfect PDF persistence (inspired by PDF4QT).
4. **Validation:** Perform a dummy operation, trigger Undo, trigger Redo, and verify PDFium state matches exactly.

## Phase D: Drawing and Shapes
1. Implement the `StrokeStabilizer` pipeline in `InteractionManager` (Deadzone, Inertia, Velocity Gaussian) based on Xournal++'s mathematical models.
2. Implement live Direct2D lightweight preview overlays for strokes.
3. Integrate a post-processing `ShapeRecognizer` to snap rough strokes to lines/circles.
4. **Validation:** Draw fast and slow strokes using a mouse/stylus; verify visual smoothing with zero PDF engine lag.

## Phase E: Annotation Architecture (Persistence)
1. Map completed `InteractionManager` strokes and shapes into native PDFium `/Ink`, `/Square`, and `/Circle` annotations.
2. Integrate the generation of standard Appearance Streams (`/AP`).
3. Connect the commit action to the Phase C `CommandStack`.
4. **Validation:** Draw a stroke, save the PDF, reopen in Adobe Acrobat, and verify the ink is a valid, scalable PDF object.

## Phase F: Text Selection and Text Editing
1. Implement Okular's `TextEntity` extraction using `FPDFText_GetCharBox` to map glyph boundary quads.
2. Build the hit-testing logic to select words/lines based on actual quad geometry rather than visual guesswork.
3. Adapt PDF4QT's native text editing approach by wrapping DirectWrite (`IDWriteTextLayout`) with a transformation matrix to support true WYSIWYG text modification.
4. **Validation:** Select text across a multi-column rotated page and verify exact highlighting.

## Phase G: Zoom and Scrolling Improvements
1. Implement Okular's `zoomWithFixedCenter` algorithm into the mouse wheel handler.
2. Implement the `accumulatedDelta` threshold for discrete page flipping to prevent trackpad sensitivity issues.
3. **Validation:** Zoom in to 400% on a specific word using the mouse wheel; the word must not move from beneath the cursor.

## Phase H: Rendering and Tile Improvements
1. Retrofit `TileCache` with Okular's `TILES_MAXSIZE` quad-tree splitting logic to cap memory usage at extreme zooms.
2. Update the `RenderWorker` queue to prioritize tiles based on geometric distance from the viewport.
3. Implement aggressive queue eviction for obsolete tiles during fast scrolling.
4. **Validation:** Rapidly scroll through a 1000-page document; verify UI thread remains at 60fps and memory does not infinitely spike.

## Phase I: Integration and Regression
1. Finalize the UI bindings to all underlying systems.
2. Compile Release builds (`PDFElite.exe`).
3. Perform end-to-end user flows (Open -> Scroll -> Zoom -> Select Text -> Draw Arrow -> Move Arrow -> Undo -> Save -> Reopen).
4. **Validation:** Zero regressions in existing capabilities; all new features functionally perfect.
