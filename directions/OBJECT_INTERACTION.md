# Object Interaction Framework

The interaction framework (`InteractionManager`) handles user input events (mouse, keyboard) targeted at selectable objects and renders interaction overlays natively using Direct2D.

## Architecture

- **`InteractionManager`**: Hooks into `PdfViewer` to intercept `OnLButtonDown`, `OnMouseMove`, `OnLButtonUp`, and `OnKeyDown`. It converts view (screen) coordinates to document (page) coordinates using the injected `viewToPage` and `pageToView` callbacks.
- **Hit Testing**: Mouse events are first tested against active object handles (resize, rotation), then against object bounds, and finally default to creating a marquee selection rectangle if no object is hit.

## Supported Interactions

- **Selection**: Click to select, Shift-click to toggle selection.
- **Marquee Selection**: Drag on empty space to draw a selection rectangle. All objects intersecting the rectangle upon release will be selected.
- **Moving**: Drag an object to translate it across the page.
- **Resizing**: 8 directional handles allow resizing objects.
- **Rotation**: A single rotation handle allows adjusting the object's angle (currently a mock implementation).
- **Keyboard Navigation**: Arrow keys allow fine-tuning the position of selected objects.

## Rendering

Overlays are rendered natively via Direct2D using semi-transparent solid color brushes and stroke outlines for handles, ensuring crisp visualization regardless of zoom level. The interaction layer is decoupled from PDF parsing to allow fast 60fps overlay updates during drags.
