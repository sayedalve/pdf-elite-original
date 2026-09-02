# World Class UX Audit

## 1. Drag and Drop Experience
* **Current**: Dropping PDFs onto the window did nothing.
* **Why it feels poor**: Forces users into file picker workflows.
* **Expected**: Dropping opens files in new tabs.
* **Proposed**: Implemented WM_DROPFILES handler in MainWindow.

## 2. Dirty Tab and App State
* **Current**: 'Modified' state was hardcoded alse on tabs. Close operations silently quit.
* **Why it feels poor**: Data loss risk and lack of confidence.
* **Expected**: * suffix on dirty tabs. Prompt on close.
* **Proposed**: Synced 	ab->isModified with CommandStack().IsDirty(). Implemented PromptSaveChanges() and wired into WM_CLOSE and CloseTab.

## 3. Zooming (Item 11)
* **Current**: Ctrl+Wheel zoomed to the vertical center of the view.
* **Why it feels poor**: User loses context if zooming near the edges.
* **Expected**: Zoom anchors to mouse pointer.
* **Proposed**: Overhauled PdfViewer::OnZoom to accept mouse coordinates and re-center the anchor dynamically.

## 4. Search UX (Item 12 & 46)
* **Current**: Good visual bar, but keyboard mapping was disconnected from MainWindow.
* **Why it feels poor**: Esc didn't close properly from parent focus.
* **Expected**: Esc clears, Enter steps forward.
* **Proposed**: Verified SearchBar implements strict subclass overrides for VK_RETURN (Next), Shift+VK_RETURN (Prev), and VK_ESCAPE (Close/Clear).

## 5. Cut/Paste (Item 20)
* **Current**: Only Undo/Redo/Copy implemented globally in viewer.
* **Why it feels poor**: Users naturally try to Cut (Ctrl+X) selected objects.
* **Expected**: Cut removes and copies.
* **Proposed**: Added Ctrl+X explicitly triggering CopySelection() followed by OnKeyDown(VK_DELETE). Ctrl+V injected into Text Editor context.

## 6. Home Transition (Item 26/27)
* **Current**: Clicking Home blindly swapped the view pointer without verifying state.
* **Why it feels poor**: Leaves hidden active documents holding locks, or risks silent discard.
* **Expected**: Home asks to save modified tabs, then unloads the workspace.
* **Proposed**: onHomeRequest now safely prompts all dirty tabs via PromptSaveChanges before clearing the active workspace and returning to HomeView.
