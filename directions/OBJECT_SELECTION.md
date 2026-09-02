# Object Selection Framework

The object selection framework provides a unified mechanism for selecting non-text, absolute-positioned PDF objects such as images, shapes, and annotations. 

## Architecture

- **`ISelectableObject`**: The interface that represents any selectable object on a PDF page. It provides methods for retrieving and setting an object's ID, page index, bounding box, and rotation. This abstraction decouples the selection logic from `pdf_engine` and PDFium-specific structures.
- **`SelectionModel`**: Manages the collection of currently selected `ISelectableObject`s. It supports Single, Multi (Shift-click), Add, Toggle, and Clear selection operations.

## Capabilities

- Multiple object selection via Shift-click or marquee drag.
- Bounding box and resize/rotation handles automatically compute their screen coordinates using the `InteractionManager`.
- Selection handles are rendered separately from the D2D page tiles to ensure high-performance rendering.

## Known Limitations

- Text selection currently operates via a separate mechanism (`ITextPage` character indices) and does not yet share the unified `ISelectableObject` model. Full object editing operations (e.g. commands for undo/redo) are pending implementation.
