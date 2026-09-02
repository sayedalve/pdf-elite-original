# Text Selection and Text Editing Research

This document outlines the findings for Phase 4, focusing on Text Selection, Selection Geometry, and Text Object Editing derived from Okular, PDF4QT, and Xournal++.

## 1. Text Selection & Geometry (Okular)
**Strongest Reference:** Okular

**Concept:** 
Okular's text extraction relies on the `TextPage` and `TextEntity` model. 
* A `TextEntity` is the absolute smallest selectable element (ideally a single glyph). It maps a `QString` (the actual Unicode text) to a `NormalizedRect` (the exact quad boundary of that glyph on the page).
* Once the `TextPage` is populated, Okular algorithms reorder these raw glyph bounding boxes into logically contiguous words, lines, and paragraphs.
* When a user drags the mouse to select text, Okular computes geometric intersections against the reordered lines/words, mapping screen coordinates to `NormalizedPoint`s, resolving precisely which glyphs fall inside the selection marquee.

**Adaptation for PDF Elite:**
* We must avoid generating "fake visual selections" (e.g., just drawing a blue box over the screen).
* We will extract glyph boundaries (`FPDFText_GetCharBox` via PDFium) and build our own `TextEntity` cache per page.
* Selections will calculate the exact quad points of the text underlying the cursor, handling rotated pages and mixed font sizes naturally because the quads track the glyphs directly.

## 2. Multi-line & Word Selection
**Reference:** Okular & PDF4QT

**Concept:**
* **Double-click (Word Selection):** Resolves the hit-tested `TextEntity` and expands outwards within the contiguous "word" boundary array.
* **Multi-line:** Because the `TextPage` pre-processes raw glyphs into lines, selecting across vertical bounds simply adds the intermediate `Line` structures to the selection cache.

## 3. Caret Placement & Text Editing (PDF4QT)
**Strongest Reference:** PDF4QT

**Concept:**
* PDF4QT uses a `PdfTextEditPseudoWidget` built on top of Qt's `QTextLayout`.
* It binds cursor movement (`getCursorForward`, `getCursorLineUp`) and selection ranges (`m_selectionStart`, `m_selectionEnd`) directly to the underlying string layout.
* It uses a transformation matrix `createTextBoxTransformMatrix()` to map the widget's internal 2D cursor positions back out to the PDF Page coordinate space.

**Adaptation for PDF Elite:**
* Since we use a native Windows architecture, we will bind DirectWrite (`IDWriteTextLayout`) to the underlying text objects.
* We will extract the native PDFium text metrics and instantiate a DirectWrite layout that perfectly matches the PDF. 
* User interaction (click, drag, keyboard movement) will query the DirectWrite layout for caret metrics (`HitTestTextPosition`, `HitTestPoint`), and we will apply a coordinate transform matrix to project that caret visually onto our Direct2D render surface.

## Summary
* **Okular** provides the best model for extracting, ordering, and selecting static text.
* **PDF4QT** provides the best model for modifying/editing text by delegating caret and layout math to a native rich-text system (like Qt or DirectWrite) bound by a coordinate transform matrix.
