# Annotation Implementation Details

This document covers the granular details of the native C++ implementation of annotations in PDF Elite.

## 1. Interaction and Event Routing
`InteractionManager` receives mouse and keyboard input from `PdfViewer`. When a tool mode is active (e.g., `ToolMode::Highlight`), dragging operations construct temporary geometrical shapes or perform direct DOM modifications based on HitTesting.
Selection outlines and resize handles are rendered over the base PDF bitmap through Direct2D's `ID2D1HwndRenderTarget`.

## 2. Modification Callbacks
Modifications are persisted using the `onObjectCommitted` and `onDeleteRequested` callbacks exposed by `InteractionManager`. 
- `onObjectCommitted` validates and updates geometry and triggers a partial visual refresh.
- `onDeleteRequested` unlinks the object from the interaction model and commands the `PdfPage` to remove the underlying annotation.

## 3. Tile Invalidation
Changing an annotation property (e.g., color) or deleting it causes `PdfViewer` to call `m_tileCache->InvalidatePage(pageIndex)`. This clears cached textures for that page and forces a re-fetch of the updated PDFium document representation during the next frame, eliminating ghosting artifacts.

## 4. Context Menus
A `WM_RBUTTONUP` event triggers a context menu for the currently selected annotation. Selections can be given varying stroke colors through mapped IDs (`2001`-`2005`).

## 5. Status (Task 11E update)
All interaction hooks, context menus, and visual invalidation logics are implemented and strictly tested in `RegressionSuite.cpp`.
