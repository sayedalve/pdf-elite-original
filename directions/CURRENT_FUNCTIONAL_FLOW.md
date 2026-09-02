# Current Functional Flow

## 1. Highlight Execution and Invalidation
- **Workflow:** Select text -> Click Highlight -> Command -> Document Mutation -> Invalidation -> Render -> Visible.
- **Smoking Gun:** 
  1. AddAnnotationCommand properly mutates the main document and increments m_generation.
  2. PdfViewer::Render loops tiles and tries m_tileCache->Get(key) with the new generation.
  3. It misses, and correctly falls back to m_generation - 1 to prevent flickering.
  4. *CRITICAL FLAW:* Because the fallback successfully returns an old bitmap, the code executes the if (bitmap) branch and **completely skips** the else branch which contains core::RenderController::Instance().EnqueueRequest(req).
  5. The new tile is never requested from the background worker. It relies on a future zoom/scroll to force a completely new tile key that has no m_generation - 1 fallback.

## 2. Right Click Context Menu Lifecycle
- **Workflow:** Right click on PDF -> Identify Target -> Construct Menu -> Show Menu.
- **Smoking Gun:** 
  1. PdfViewer::OnRButtonUp performs a hit test.
  2. If an object is found under the cursor, it modifies the active document selection state (m_interactionManager.GetSelectionModel().AddSelect(hitObj)).
  3. This addition synchronously fires OnSelectionChanged, bubbling up to MainWindow.
  4. MainWindow reacts to the selection change by calling m_propertiesPanel->SetVisible(true).
  5. *CRITICAL FLAW:* A simple right-click structurally modifies the selection model. It should only temporarily identify the context target for the menu.

## 3. Global Undo/Redo
- **Workflow:** User triggers Ctrl+Z / Ctrl+Y or clicks Toolbar Undo/Redo.
- **Smoking Gun:** 
  1. Ctrl+Z correctly hits MainWindow::PreTranslateMessage, invokes 	ab->viewer->OnUndo(), which updates m_generation, invalidates cache, and redraws.
  2. *CRITICAL FLAW:* Clicking the Toolbar Undo button triggers MainWindow::OnCommand with ction == L"Undo". This directly calls 	ab->document->GetCommandStack().Undo(), entirely bypassing 	ab->viewer->OnUndo().
  3. As a result, the document changes, but m_generation is never incremented, the cache is not invalidated, and the screen is never redrawn.

## 4. State Consistency (Selection)
- **Workflow:** Select All, Text Selection, Object Selection.
- **Smoking Gun:** 
  1. There are competing selection states: PdfViewer::m_selection (for old marquee text selection), PdfViewer::SelectAllText() which interacts with TextSelectTool's model, and InteractionManager's object selection.
  2. OnCommand IDM_PAGE_SELECT_ALL sets PdfViewer::m_selection. But other tools look at different selection models.

## 5. Recent Files
- **Workflow:** Click Recent File in Home view -> Open PDF.
- **Smoking Gun:** 
  1. The pipeline likely bypasses the safe OpenLocalPDF logic which registers the document with the RenderWorker, initializes layout, and pushes it to AppShell.

## 6. Worker Thread Document Cloning
- **Workflow:** Modify document on UI thread -> worker renders tiles.
- **Smoking Gun:**
  1. RenderWorker::RegisterDocument clones the document exactly once when the file opens.
  2. PdfDocument::Clone returns 
ullptr to force a shared instance, which is thread-safe for reading.
  3. However, if it didn't share the instance, modifications would be invisible. The issue here is purely the missed EnqueueRequest during the fallback loop.
