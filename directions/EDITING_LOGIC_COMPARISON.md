# Editing Logic Comparison

This document compares the editing architecture of PDF Elite with the reference implementations (primarily PDF4QT and Xournal++) as requested in Phase 2.

---

## 1. Native Text and Object Edit Widgets

* **Current PDF Elite logic:** Basic or viewer-only with limited text modification capability.
* **Reference project:** PDF4QT
* **Reference source location:** `Pdf4QtLibWidgets/sources/pdftexteditpseudowidget.h`, `Pdf4QtLibWidgets/sources/pdfpagecontenteditorwidget.h`
* **Problem with current implementation:** Lacks true PDF-level text editing capability; typically relies on disconnected overlays that don't match underlying PDF font metrics.
* **Proposed improvement:** Implement a native pseudo-widget that computes real cursor boundaries, selections, and text layouts within the PDF coordinate space.
* **Why the reference is better:** PDF4QT uses Qt's `QTextLayout` wrapped in a transformation matrix (`createTextBoxTransformMatrix`) to map text metrics directly into the page space, enabling true WYSIWYG PDF text editing.
* **Integration strategy:** Adapt this logic using Windows DirectWrite (`IDWriteTextLayout`) instead of `QTextLayout`. We will extract font metrics from PDFium and use DirectWrite to handle cursor placement and selection boundaries natively.
* **Risk:** High. Translating PDF font metrics to DirectWrite metrics seamlessly is mathematically complex.
* **License considerations:** MIT (PDF4QT). The algorithm and matrix transformation logic can be safely adapted and incorporated.

---

## 2. Granular Command / Undo Architecture

* **Current PDF Elite logic:** Basic `CommandStack` with generic or limited actions.
* **Reference project:** Xournal++ (and PDF4QT)
* **Reference source location:** `src/core/undo/` (Xournal++)
* **Problem with current implementation:** Undo operations are not granular enough and don't cover all interaction states cleanly.
* **Proposed improvement:** Implement a strictly typed Command Pattern where every distinct operation (Add, Delete, Move, Rotate, ColorChange, StrokeErase) has a dedicated inverse command object.
* **Why the reference is better:** Xournal++ uses extremely granular commands (`AddUndoAction`, `RotateUndoAction`, `TextBoxUndoAction`), ensuring robust state restoration without deep coupling to the core document state.
* **Integration strategy:** Map the conceptual `UndoAction` interface to PDF Elite's existing `CommandStack`. Create specialized command classes for all new editing operations before building the operations themselves.
* **Risk:** Low. This is a standard and highly understood software engineering pattern.
* **License considerations:** GPL v2.0 (Xournal++). We **cannot** copy the source code or class structures. We will implement the standard Command Pattern independently in our own native C++ architecture.

---

## 3. Data-bound PDF Object Editing Model

* **Current PDF Elite logic:** Ad-hoc property parsing for annotations and objects.
* **Reference project:** PDF4QT
* **Reference source location:** `Pdf4QtLibCore/sources/pdfobjecteditormodel.h`
* **Problem with current implementation:** Adding new editable properties requires writing repetitive UI and extraction code for every object type.
* **Proposed improvement:** Create a declarative metadata model (`PDFObjectEditorAbstractModel`) that maps UI attributes (category, min/max, default value, type flags) directly to underlying PDF dictionary keys.
* **Why the reference is better:** It is highly scalable. PDF4QT defines attributes and flags once, and the UI automatically renders property grids that map directly to the PDF object's state.
* **Integration strategy:** Implement a similar attribute metadata mapping in C++ using structs or templates that interface with PDFium's dictionary accessors (`FPDF_ANNOT`, etc.).
* **Risk:** Medium. Requires careful mapping of PDFium's C API to a robust C++ object model.
* **License considerations:** MIT (PDF4QT). The structural design and concepts can be freely adapted with attribution.
