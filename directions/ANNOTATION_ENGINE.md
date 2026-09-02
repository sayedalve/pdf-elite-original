# Annotation Engine Architecture

The Annotation Engine operates entirely within the native C++ `pdf_engine` module, wrapping the raw PDFium `FPDF_ANNOTATION` abstractions into safe `IAnnotation` interfaces.

## 1. Abstraction Layer
The base `IAnnotation` class exposes standard geometrical and property modification methods:
- `GetBounds()` / `SetBounds()`
- `GetColor()` / `SetColor()`
- `SetFillColor()`

This guarantees zero leakage of `#include "fpdf_annot.h"` into the UI layers or the `InteractionManager`.

## 2. Interaction Integration
Annotations are wrapped within `AnnotationSelectableObject` that inherits from `ui::interaction::ISelectableObject`. This enables uniform handling by the `SelectionModel` and `InteractionManager`. 

## 3. Persistence
All modifications applied via `IAnnotation` immediately execute against the PDFium document dictionary. Invoking `PdfDocument::SaveToFile()` seamlessly serializes these mutated annotations to disk, ensuring 100% standard-compliance.

## 4. Status (Task 11E update)
All annotation types (markups, drawings, text, and sticky notes) are fully supported natively.
