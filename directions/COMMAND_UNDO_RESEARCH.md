# Command and Undo Architecture Research

This document outlines the findings for Phase 8, comparing the Undo/Redo architectures of PDF4QT and Xournal++ against PDF Elite's current `CommandStack`.

## 1. Xournal++: The Granular Command Pattern
**Strongest Reference for Interactions:** Xournal++
**Location:** `src/core/undo/UndoRedoHandler.h`, `UndoAction.h`

**Concept:**
* Xournal++ uses a classic, strictly-typed Command Pattern.
* Every single user action has a dedicated inverse class: `AddUndoAction`, `MoveUndoAction`, `EraseUndoAction`, `RotateUndoAction`, etc.
* **Strengths:** Extremely memory efficient for vector editing. A move operation only stores a translation matrix (`dx`, `dy`), not the whole object.
* **Weaknesses:** Highly coupled to their proprietary `Element` DOM.

## 2. PDF4QT: Object-Level State Reversion
**Strongest Reference for PDF Data:** PDF4QT
**Location:** `Pdf4QtLibGui/pdfundoredomanager.h`

**Concept:**
* PDF4QT's `PDFUndoRedoManager` operates at the document/object level rather than the UI interaction level.
* When an edit occurs, it creates a `PDFModifiedDocument` which tracks the delta or cloned state of the modified PDF objects.
* **Strengths:** 100% accurate to the PDF specification. Ensures that restoring state restores the exact underlying dictionary properties (fonts, colors, bounds, streams).
* **Weaknesses:** Can be heavier in memory if massive objects are cloned repeatedly.

## 3. PDF Elite: Architectural Synthesis & Improvement
PDF Elite currently uses a basic `CommandStack`. To meet the requirements of true PDF editing (text, shapes, movement, annotations) without adding a secondary undo system, we must improve the existing stack by synthesizing both reference models.

**Proposed Architecture:**
1. **Unified Interface:** Retain PDF Elite's `CommandStack`.
2. **Granular Classes (Xournal++ Style):** Create specific commands for logical operations (e.g., `AddAnnotationCommand`, `TransformObjectCommand`, `EditTextCommand`).
3. **Dictionary State Tracking (PDF4QT Style):** Instead of storing proprietary UI coordinates, the commands will store serialized PDFium dictionary states (the "Before" and "After" snapshots of the specific object being modified).
   * *Example (Move Object):* The `TransformObjectCommand` does not just store dx/dy. It reads the `/Rect` and `/Matrix` from PDFium *before* the move, and stores it. On `Undo()`, it overwrites the PDFium object's `/Rect` with the cached original, ensuring perfect PDF compliance.
4. **Logical Grouping:** Complex interactions (like drawing a stroke that also deletes an intersecting line) will be batched into a `MacroCommand` so that one user action = one logical undo step.

**Conclusion:**
By combining Xournal++'s granular interaction commands with PDF4QT's PDF-dictionary serialization approach, PDF Elite's `CommandStack` will be robust enough to handle any editing operation natively without risking corruption or desync between the UI and the PDF engine.
