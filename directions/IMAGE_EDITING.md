# IMAGE EDITING

## Workflow

### 1. Image Insertion
Images can be inserted via:
* **File Dialog**: Users select an image file (PNG, JPG, BMP) which is decoded by WIC and prepared for insertion.
* **Clipboard**: Users paste an image from the Windows clipboard.

### 2. Interactive Placement
Once an image is selected for insertion, the viewer enters `ToolMode::InsertImage`. 
As the user moves the mouse, a preview of the image placement is rendered using Direct2D. A left-click commits the insertion by generating an `InsertImageCommand` for Undo/Redo tracking.

### 3. Selection and Interaction
Inserted images are wrapped in an `ImageSelectableObject` which the `InteractionManager` uses to provide:
* Bounding boxes
* Resize handles
* Drag-to-move capabilities

### 4. Undo/Redo Integration
All image manipulations are command-based:
* `InsertImageCommand`
* `MoveImageCommand`
* `DeleteImageCommand`

When a user completes an interactive drag, the `InteractionManager` fires the `onObjectCommitted` callback, which dispatches a `MoveImageCommand` tracking the old and new bounding boxes. Deletions via the Delete key dispatch a `DeleteImageCommand`.

### 5. Copying Images
When an image is selected, the `CopySelection` method extracts the underlying bitmap data and places it on the Windows clipboard using the `CF_DIB` format, enabling cross-application image copying.
