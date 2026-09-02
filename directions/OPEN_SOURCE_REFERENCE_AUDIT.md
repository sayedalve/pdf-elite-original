# Open Source Reference Audit

This document contains the initial architectural audit of the three reference PDF applications as requested in Phase 0.

## 1. PDF4QT
**Repository:** https://github.com/JakubMelka/PDF4QT

* **Language:** C++
* **PDF Engine:** Custom (Pdf4QtLibCore), built from scratch on top of Qt, rather than using Poppler or PDFium.
* **Renderer:** Qt-based rendering pipeline (Pdf4QtLibGui/Widgets).
* **Annotation Architecture:** [To be inferred from source] Likely native custom objects mapped closely to PDF specifications, due to custom core engine.
* **Document Model:** [To be inferred from source]
* **Page Model:** [To be inferred from source]
* **Selection Model:** [To be inferred from source]
* **Text Editing Model:** [To be inferred from source] Includes rich text editing support since it bills itself as an editor.
* **Object Model:** [To be inferred from source]
* **Rendering Architecture:** [To be inferred from source]
* **Tile/Cache Architecture:** [To be inferred from source]
* **Interaction Model:** [To be inferred from source]
* **Undo/Redo:** [To be inferred from source] Likely uses QUndoCommand/QUndoStack or custom implementation.
* **Coordinate Conversion:** [To be inferred from source]
* **Save/Persistence:** [To be inferred from source] Direct PDF writing from custom engine.
* **Threading:** [To be inferred from source]
* **Strengths:** Fully self-contained editing logic; doesn't rely on Poppler's read-only biases; native Qt integration; active development for editing features.
* **Weaknesses:** Custom PDF engine might not have the rendering compatibility breadth of PDFium/Poppler; smaller community compared to KDE/Okular.
* **Useful components for PDF Elite:** Text object editing, object manipulation algorithms, UI interaction patterns for editing.
* **Licensing Constraints:** MIT License. Highly compatible. We can port or directly use algorithms/code with attribution.

---

## 2. Xournal++
**Repository:** https://github.com/xournalpp/xournalpp

* **Language:** C++
* **PDF Engine:** Poppler
* **Renderer:** Cairo/GTK
* **Annotation Architecture:** Custom proprietary layer on top of PDF. Stores annotations in a separate `.xopp` XML file by default, or can export to PDF.
* **Document Model:** [To be inferred from source]
* **Page Model:** [To be inferred from source]
* **Selection Model:** [To be inferred from source]
* **Text Editing Model:** [To be inferred from source]
* **Object Model:** [To be inferred from source]
* **Rendering Architecture:** [To be inferred from source]
* **Tile/Cache Architecture:** [To be inferred from source]
* **Interaction Model:** Excellent stylus/pen input, pressure sensitivity, direct manipulation.
* **Undo/Redo:** [To be inferred from source]
* **Coordinate Conversion:** [To be inferred from source]
* **Save/Persistence:** [To be inferred from source]
* **Threading:** [To be inferred from source]
* **Strengths:** Best-in-class freehand drawing, stylus handling, pressure sensitivity, shape recognition, smooth stroke generation.
* **Weaknesses:** Uses separate file format for active edits; GTK based (harder to adapt to Direct2D directly); relies on Poppler.
* **Useful components for PDF Elite:** Ink smoothing algorithms, freehand drawing logic, stylus pressure handling, shape creation/editing UI patterns.
* **Licensing Constraints:** GPL v2.0 or later. We **cannot** copy this code directly into a proprietary or non-GPL compatible product. We must strictly study the algorithms and reimplement independently without copying structure or code.

---

## 3. Okular
**Repository:** https://github.com/KDE/okular

* **Language:** C++
* **PDF Engine:** Poppler (via Okular generator architecture)
* **Renderer:** Qt (QPainter)
* **Annotation Architecture:** Standard PDF annotations natively, plus custom local XML storage for annotations if modifying the PDF directly isn't possible.
* **Document Model:** [To be inferred from source]
* **Page Model:** [To be inferred from source]
* **Selection Model:** Robust text selection, geometric selection, word/glyph boundaries.
* **Text Editing Model:** [To be inferred from source]
* **Object Model:** [To be inferred from source]
* **Rendering Architecture:** Highly mature. Page virtualization, progressive rendering.
* **Tile/Cache Architecture:** [To be inferred from source]
* **Interaction Model:** [To be inferred from source]
* **Undo/Redo:** [To be inferred from source]
* **Coordinate Conversion:** [To be inferred from source]
* **Save/Persistence:** [To be inferred from source]
* **Threading:** Asynchronous rendering with ThreadWeaver/QThread.
* **Strengths:** Extremely mature reading, text selection, scrolling, and rendering architectures. Very stable memory management and caching.
* **Weaknesses:** Complex KDE/Qt dependency tree; primarily a viewer, not an editor.
* **Useful components for PDF Elite:** Text selection logic (quad points, glyph bounds), rendering cache design, zoom/scroll architecture.
* **Licensing Constraints:** GPL v2.0 or later. We **cannot** copy this code directly. We must strictly study the algorithms and reimplement independently.
