# Deep UX Reference Audit

## PHASE A: Deep Source Analysis

This document records the detailed findings for each major interactive behavior, examining reference implementations and comparing them to PDF Elite's current behavior, followed by a plan for adaptation and integration.

---

### Behavior: Input Routing
* **Reference project:** PDF4QT and Okular
* **Exact source files:** PDF4QT: `pdfdrawwidget.cpp`, Okular: `pageview.cpp`
* **Relevant classes/functions:** PDF4QT: `PDFDrawWidget::mousePressEvent`, `IDrawWidgetInputInterface`. Okular: `PageView::mousePressEvent`
* **Observed behavior:** 
  - **PDF4QT** maintains a list of `m_inputInterfaces` (tools, annotation managers) and iterates through them during `mousePressEvent` using `processEvent` template. If an interface consumes the event, it stops. Fallback is handled by the widget (e.g., autoscroll, translate).
  - **Okular** uses a giant switch statement on `d->mouseMode` in `PageView::mousePressEvent`. It directly manages state for zoom, magnifier, selection, and delegates to `d->annotator` or `d->mouseAnnotation` for specific tools.
* **Current PDF Elite behavior:** `InputRouter` has a 1:1 hardcoded dependency on `ToolStateMachine`. All pointer events (`RoutePointerDown`, etc.) just immediately return `m_stateMachine->RoutePointerDown(event)`.
* **Gap:** PDF Elite's routing is too rigid and doesn't allow multiple input interceptors (like annotation managers, UI overlays, text selection) to easily share or intercept events before the state machine.
* **Direct copy or independent reimplementation:** Independent reimplementation based on PDF4QT's interceptor chain pattern.
* **Adaptation required:** Modify `IInputRouter` to support registering multiple `IInputInterceptor` instances with priorities. The router will pass events down the chain until consumed.
* **License:** PDF4QT is LGPL/GPL (but we are writing independent adapter).
* **Testing status:** Pending

---

### Behavior: Tool State Machine
* **Reference project:** Okular
* **Exact source files:** `pageview.cpp`
* **Relevant classes/functions:** `PageView` and `EnumMouseMode`
* **Observed behavior:** 
  - Tool state is primarily driven by an enum (`Browse`, `Zoom`, `RectSelect`, `TextSelect`, etc.).
  - Transitions between tools immediately update the cursor and clear temporary state (like active selection boxes or temporary overlays).
  - Certain operations (like middle-click) temporarily override the state to enter "continuous zoom" or pan mode, returning to the previous state when released.
* **Current PDF Elite behavior:** `ToolStateMachine` holds a map of `std::unique_ptr<ITool>` and delegates events. `SetActiveTool` calls `OnDeactivate` and `OnActivate`. 
* **Gap:** PDF Elite doesn't appear to have a generic stack or "temporary tool" override mechanism (e.g., holding spacebar to pan while the draw tool is active).
* **Direct copy or independent reimplementation:** Independent reimplementation.
* **Adaptation required:** Add a `PushTool()` and `PopTool()` or `SetTemporaryTool()` mechanism to `ToolStateMachine` to handle modifier-key tool overrides (like Space for Pan).
* **License:** Okular is GPL.
* **Testing status:** Pending

---

### Behavior: Cursor System
* **Reference project:** PDF4QT
* **Exact source files:** `pdfdrawwidget.cpp`
* **Relevant classes/functions:** `PDFDrawWidget::updateCursor`, `IDrawWidgetInputInterface::getCursor`
* **Observed behavior:** 
  - `updateCursor()` asks each input interface in the chain `inputInterface->getCursor()`.
  - The first interface to return a valid cursor wins (e.g. annotation manager might return a resize cursor if hovering over a handle, else a drawing tool returns a crosshair).
  - If no interface returns a cursor, it falls back to the widget's default state (e.g. OpenHand / ClosedHand depending on whether a pan operation is active).
  - The cursor is explicitly updated at the end of input events (`mousePressEvent`, `mouseMoveEvent`, `keyPressEvent`, etc.).
* **Current PDF Elite behavior:** `InputRouter::QueryCursor` delegates exclusively to `m_stateMachine->GetCursor(clientDip)`.
* **Gap:** Does not support hierarchical cursor resolution well (e.g., UI overlays or annotations overriding the tool cursor).
* **Direct copy or independent reimplementation:** Independent reimplementation utilizing the new Interceptor chain.
* **Adaptation required:** `IInputInterceptor` should have `virtual std::optional<HCURSOR> QueryCursor(const PointF& clientDip)`. `InputRouter` will query the chain in order. If none respond, return `IDC_ARROW` or `IDC_HAND`.
* **License:** PDF4QT is LGPL/GPL (adapter pattern).
* **Testing status:** Pending

---

### Behavior: Selection System
* **Reference project:** Okular
* **Exact source files:** `pageview.cpp` (`mouseDoubleClickEvent`)
* **Relevant classes/functions:** `PageView::mouseDoubleClickEvent`, `PageView::textSelectionForItem`
* **Observed behavior:** 
  - Double clicking text delegates to `page()->wordAt()` using normalized coordinates rotated to the page's orientation.
  - Okular detects "Quad-click" (clicking 4 times) which clears the selection and resets, to prevent accidental selection loops.
  - Double clicking an annotation (if not a widget) opens the Annotation Window (text popup editor).
* **Current PDF Elite behavior:** `TextSelectTool` tracks multi-clicks up to Triple Click (line/paragraph). It relies on `SelectionModel::SelectWordAt`. Double-clicking annotations currently does nothing special in `SelectTool`.
* **Gap:** PDF Elite doesn't map double-click on an annotation to "Open Editor / Popup". It also stops at Triple Click (no Quad-click reset).
* **Direct copy or independent reimplementation:** Independent reimplementation to match Okular's annotation editor popup.
* **Adaptation required:** `SelectTool::OnPointerDoubleClick` should do a hit test for annotations and trigger an "Open Popup" command.
* **License:** Okular is GPL (reimplement behavior).
* **Testing status:** Pending

---

### Behavior: Drag/Move/Resize/Rotate
* **Reference project:** Xournal++
* **Exact source files:** `EditSelection.cpp` (`mouseMove`, `getSelectionTypeForPos`, `scaleShift`)
* **Relevant classes/functions:** `EditSelection::mouseMove`
* **Observed behavior:** 
  - Hit testing uses screen-space padding (`btnWidth / 2 + 2`) mapped back through the transformation matrix to ensure the physical grab region on screen remains constant regardless of zoom.
  - Movement supports explicit Grid Snapping when `Alt` is not pressed. It precisely un-projects the snapped rotation center.
  - Resizing calculates a scale factor and anchor offset (`dx`, `dy`) and compensates for the pivot center shifting due to bounding box changes while rotated (`cxRot - cx, cyRot - cy`).
* **Current PDF Elite behavior:** `TransformHandles::HitTest` correctly operates in screen-space DIPs (using `handleSizeDip` + `hitToleranceDip`), which matches the zoom-independent goal.
* **Gap:** PDF Elite does not have full bounding-box snap-to-grid support during drag/resize, nor does it appear to correctly compensate the pivot shift when resizing rotated annotations (this can cause the shape to "jump" when resizing a rotated bounding box).
* **Direct copy or independent reimplementation:** Independent reimplementation based on Xournal++ mathematical transform handling.
* **Adaptation required:** Enhance `TransformHandles` or `TransformAnnotationCommand` with rotation-pivot compensation during resize operations.
* **License:** Xournal++ is GPL (reimplement math).
* **Testing status:** Pending

---

### Behavior: Contextual UI
* **Reference project:** PDF4QT
* **Exact source files:** `pdfwidgetannotation.cpp`
* **Relevant classes/functions:** Context menu handling
* **Observed behavior:** 
  - Floating contextual UI (like property popups or mini toolbars) are triggered on right-click or specific interactions (like double-clicking).
  - Contextual UI positioning ensures it stays within viewport bounds, even if the target object is partially clipped.
  - Contextual menus do not steal focus from the primary view unless they contain text inputs.
* **Current PDF Elite behavior:** Contextual menus / UI might be underdeveloped or standard Win32 menus that steal focus or block execution.
* **Gap:** Missing an overlay-based non-blocking contextual toolbar for quick actions on selected annotations.
* **Direct copy or independent reimplementation:** Independent reimplementation using Direct2D overlay or `WS_POPUP` borderless windows.
* **Adaptation required:** Implement a mini-toolbar mechanism that spawns near `SelectionModel.GetSelectionBounds()`, but clamped to `ViewportEngine` client bounds.
* **License:** Independent architecture.
* **Testing status:** Pending

---

### Behavior: Keyboard/Focus
* **Reference project:** Okular / Xournal++
* **Exact source files:** `pageview.cpp`, `InputRouter.cpp`
* **Relevant classes/functions:** `keyPressEvent`, `Cancel()`
* **Observed behavior:** 
  - The main canvas (`PageView`) aggressively maintains focus. When toolbars or sidebars are clicked, they often do not steal focus, or focus is returned to the canvas so keyboard shortcuts (like `Esc` or spacebar-panning) continue to work.
  - Pressing `Esc` unconditionally aborts the active interaction (e.g. cancels drawing in progress, drops a dragged item back to its origin) and releases mouse capture, without leaving the tool. Pressing `Esc` when idle clears the selection.
* **Current PDF Elite behavior:** `InputRouter::OnCaptureLost()` and `ToolStateMachine::RouteKeyDown` with `VK_ESCAPE` correctly call `Cancel()` on the active tool.
* **Gap:** Must ensure `Cancel()` completely clears ongoing drag offsets and `SelectionModel` state. Also need to ensure Win32 HWND focus isn't stolen by contextual toolbars.
* **Direct copy or independent reimplementation:** Independent reimplementation.
* **Adaptation required:** Audit all `ITool::Cancel` implementations (e.g. `TextSelectTool::Cancel`) to ensure they perfectly revert temporary interaction state (like multi-click states) and ensure overlay windows use `WS_EX_NOACTIVATE` so they don't steal keyboard focus.
* **License:** Independent architecture fix.
* **Testing status:** Pending

---

### Behavior: Undo Grouping
* **Reference project:** Xournal++
* **Exact source files:** `StrokeHandler.cpp`, `EditSelectionContents.cpp`
* **Relevant classes/functions:** `GroupUndoAction`, `InsertUndoAction`, `DeleteUndoAction`
* **Observed behavior:** 
  - Interactive operations that span time (like dragging a mouse to draw a stroke or move an object) are accumulated in temporary state.
  - When the operation commits (e.g., `mouseUp`), a single cohesive UndoAction (like `InsertUndoAction` or `MoveUndoAction`) is instantiated and pushed to the undo stack.
  - For complex edits (like replacing an object), Xournal++ creates a `GroupUndoAction` which bundles a `DeleteUndoAction` and an `InsertUndoAction` so they undo as a single atomic step.
* **Current PDF Elite behavior:** PDF Elite utilizes `core::CommandStack` and specific commands like `TransformAnnotationCommand` which are pushed on completion.
* **Gap:** PDF Elite doesn't seem to have a robust `CommandMacro` or `GroupUndoAction` to bundle multi-step operations (e.g. if dragging an object creates a new duplicate, it should be atomic).
* **Direct copy or independent reimplementation:** Independent reimplementation.
* **Adaptation required:** Add a `CommandGroup` class that implements `ICommand` and contains a `std::vector<std::unique_ptr<ICommand>>`. This will allow atomic multi-step undo operations matching Xournal++'s behavior.
* **License:** Xournal++ is GPL.
* **Testing status:** Pending

---

### Behavior: Document/Tab UX
* **Reference project:** Okular / PDF4QT
* **Exact source files:** `DocumentSession.h`, `TabManager.h` (PDF Elite current implementation)
* **Relevant classes/functions:** `DocumentSession`
* **Observed behavior:** 
  - Mature applications persist the current zoom, scroll offset, selected tool, and active selection state uniquely per tab.
  - When switching from Tab A to Tab B, Tab B restores its own tool (e.g. Text Select vs Draw) and its own selection.
* **Current PDF Elite behavior:** `TabManager` and `DocumentSession` appear to only encapsulate the `IDocumentEngine`. Higher level UI states like `ToolStateMachine`'s active tool, or `SelectionModel` are currently stored at the `InputRouter` or `TextSelectTool` level, which means they are likely global and leak across tabs!
* **Gap:** PDF Elite leaks selection, tool state, and potentially search/scroll state across document tab switches because these are not stored in the `DocumentSession`.
* **Direct copy or independent reimplementation:** Independent reimplementation to move state.
* **Adaptation required:** `DocumentSession` must be expanded to include its own `ToolStateMachine`, `SelectionModel`, `ZoomState`, and `ScrollState`. When `TabManager::ActivateSession` is called, the UI must sync to the newly active session's state.
* **License:** Independent architecture fix.
* **Testing status:** Pending

---

### Behavior: Scroll/Zoom Interaction
* **Reference project:** Okular
* **Exact source files:** `pageview.cpp`
* **Relevant classes/functions:** `PageView::wheelEvent`
* **Observed behavior:** 
  - Zooming via Ctrl+Wheel anchors on the mouse pointer.
  - Crucially, if the user is dragging (left click pressed) while doing Ctrl+Wheel, Okular temporarily fakes an `InputRelease` to the scroller, performs the zoom, and then fakes an `InputPress`. This ensures the scroller doesn't ignore the layout changes and keeps the view centered on the mouse pointer instead of snapping back to the center of the viewport.
* **Current PDF Elite behavior:** Not fully implemented; dragging while zooming might result in unexpected offsets.
* **Gap:** Missing pointer-anchored zoom compensation during active drags.
* **Direct copy or independent reimplementation:** Independent reimplementation.
* **Adaptation required:** In the ViewportEngine or InputRouter, if a zoom occurs during an active capture/drag, we must offset the drag origin by the delta of the document coordinates under the pointer before/after the zoom.
* **License:** Okular is GPL.
* **Testing status:** Pending
