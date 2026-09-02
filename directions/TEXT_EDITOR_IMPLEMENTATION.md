# Core Text Editor Implementation (Task 15C)

## Overview
The core text editor is implemented using PDFium for modifying existing text objects and creating new text objects. The native application handles input forwarding (WM_KEYDOWN, WM_CHAR) and rendering via DirectWrite.

## Features Implemented
- **Existing Text Edit**: Double-click an existing text object to enter edit mode. Text can be modified in-place.
- **Add Text Tool**: Use `Ctrl+T` to switch to Add Text mode, then click on the page to spawn a new text object.
- **Multiline Editing**: Pressing `Enter` adds a newline `\r\n`. Keyboard navigation (Arrows, Home, End), `Backspace`, and `Delete` are fully supported.
- **Move Text Block**: Click and drag text objects outside of edit mode to move them.
- **Delete Text Block**: Select a text object and press `Delete`.
- **Clipboard Integration**: `Ctrl+C`, `Ctrl+V`, `Ctrl+X` work inside text blocks using standard Windows clipboard (`CF_UNICODETEXT`).
- **Save/Reopen Persistence**: Changes to text are committed back to PDFium's `FPDF_PAGEOBJECT` using `FPDFText_SetText` and saved successfully to the document.
- **Search Indexing**: On save and reopen, new and modified text is properly indexed.
- **Undo / Redo**: Integrated with the app's `CommandStack`. Modifying, moving, deleting, and adding text support Undo and Redo.

## Known Limitations
The following features were deferred from the core text editor implementation to avoid deep architecture rewrites at this stage:
- **Unicode Fallback Rendering**: Arbitrary Unicode insertion (e.g., Bengali, Arabic) is not natively supported if the document font doesn't have the necessary glyphs. Fallback font generation is not implemented.
- **Automatic Paragraph Reflow**: PDFium's `FPDFText_SetText` modifies the single text object but does not perform word-wrapping or paragraph reflow. The text simply flows along the existing object's baseline. Multiline text is split by explicit `\r\n` characters but may overlap other elements.

## Test Validation
Tests have been added to `TextEditingValidation.cpp` to cover:
- Basic Modification
- Save and Reopen Verification
- Add Text, Move Text, Delete Text
- Undo / Redo flows
