# Undo/Redo Command System Architecture

The Undo/Redo system in PDF Elite leverages the Command pattern using RAII principles, centralizing all document mutation tracking.

## 1. Command Architecture
- **`ICommand`**: Interface declaring `Execute()`, `Undo()`, `GetDescription()`, and `GetMemoryFootprint()`.
- **`CommandStack`**: Manages two contiguous vectors for undo and redo operations. Responsible for maximum depth enforcement, redo invalidation, and emitting state change callbacks.

## 2. History Ownership & Document State
History states reside within the `IDocument` interface layer (`PdfDocument` class). This guarantees that history lifetimes strictly correlate with the opened document, avoiding cross-contamination between tabs or views.
The history tracks a concept of a `Saved` position. Any divergence from this index makes the document `IsDirty()`. Save operations advance this `m_savePosition` without purging the undo stacks.

## 3. Implemented Commands
### Page Commands
- `DeletePageCommand`: Utilizes `FPDF_ImportPages` dynamically into a temporary decoupled `FPDF_DOCUMENT` backing buffer to preserve a single deleted page, ready for injection on undo.
- `InsertBlankPageCommand`: Straightforward `DeletePage` execution on undo.
- `DuplicatePageCommand`: Preserved by reversing with `DeletePage`.
- `MovePageCommand`: Undone by computing reverse index transformations.
- `RotatePageCommand`: Queries old rotation prior to mutations and executes inverse rotation.

### Annotation Commands
- `AddAnnotationCommand`: Uses native `RemoveAnnotation` for undo while maintaining the internal `IAnnotation` wrapper object for subsequent recreation upon redo.
- `DeleteAnnotationCommand`: Snapshots comprehensive state properties (`AnnotationState` containing Type, Bounds, Contents, Quads) prior to native deletion. Reconstructs annotation attributes dynamically on undo.
- `MoveAnnotationCommand`: Restores previous bounding rect coordinates.

## 4. Safety and Atomicity
The framework executes the payload logic (via `Execute()`) *before* pushing onto the command stack. Commands failing validation checks (returning `false`) are seamlessly discarded.

## 5. UI Integration
- Keystrokes (`Ctrl+Z`, `Ctrl+Y`) directly target the active `IDocument`'s command stack.
- Triggered commands cascade up via UI layout recalculations (`InvalidateAll()`).
