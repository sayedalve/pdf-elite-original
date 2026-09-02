# Current Annotation Architecture Audit

## 1. Current Annotation Types
Currently, the AnnotationType enum supports a variety of types (Text, Link, FreeText, Line, Square, Circle, Polygon, Polyline, Highlight, Underline, Squiggly, StrikeOut, Stamp, Caret, Ink, Popup), but only a few are actually implemented or used.
Tools currently handled in PdfViewer.cpp:
- Highlight, Underline, Strikeout (creates annotations via PdfDocument::AddAnnotation)
- Ink (creates FPDF_ANNOT_INK)
- Rectangle, Ellipse, Line, Arrow (creates shapes)
- StickyNote (creates FPDF_ANNOT_TEXT)
- AddText (creates FreeText)

## 2. Current Data Model
IAnnotation and PdfAnnotation provide a very thin C++ wrapper around PDFium's FPDF_ANNOTATION.
- **Properties Supported:** ID, Type, Bounds, Contents, QuadPoints, Color, FillColor, BorderWidth, LineGeometry, HasAppearance.
- **Weaknesses:** It mixes low-level PDFium pointer access (m_annot) with high-level properties. It does not store uncommitted annotations properly (temporary drawing is just stored as raw vectors of points in PdfViewer). It lacks support for author, creation date, modification date, opacity, stroke properties (dash pattern), and proper flags (e.g. read-only, locked).

## 3. Current Geometry Model
- AnnotationSelectableObject wraps IAnnotation for hit-testing (HitTest checks if px, py falls within GetBounds()).
- Weaknesses: Hit testing is purely bounding box based! A large diagonal line will have a huge square bounding box, and clicking anywhere in that square selects the line.

## 4. Current Interaction Model
- PdfViewer.cpp is a God Object. OnLButtonDown, OnMouseMove, OnLButtonUp, and OnSetCursor are filled with massive switch (m_currentTool) blocks and manual flags (m_isCreatingAnnotation, m_isDrawingInk).
- There is no unified Tool architecture.
- Moving and resizing is handled by InteractionManager, which handles m_drag.active and m_hoverHandle.
- InteractionManager provides an ISelectableObject interface, which is okay, but it mixes hit testing and handle definitions (HandleType::TopLeft, etc.).

## 5. Current Rendering Model
- Committed annotations are rendered by PDFium into the base page bitmap (which is good).
- In-progress annotations (drawing an ink stroke, dragging a rectangle) are rendered manually in PdfViewer::Render via Direct2D using immediate mode logic checking m_isDrawingInk or m_isCreatingAnnotation.
- Selected annotations get their handles rendered by InteractionManager::RenderSelection.
- Weakness: Hardcoded Direct2D rendering inside the viewer for every tool's preview.

## 6. Current Persistence Model
- PDFium automatically saves committed FPDF_ANNOTATIONs.
- SaveAs commits the document.

## 7. Current Undo/Redo Model
- AddAnnotationCommand, MoveAnnotationCommand, DeleteAnnotationCommand.
- Weakness: Modifying an annotation's properties (like color) doesn't have a command. Ink strokes are committed as a single command, but changing ink points after creation isn't supported. MoveAnnotationCommand stores old and new bounds, but modifying points in a polygon/polyline is not supported.

## 8. Missing Functionality
- Proper shape and line geometry hit-testing (not just AABB).
- Tool Handler architecture (Strategy pattern for mouse events).
- Callout, Polygon, Polyline, Stamp, Signature annotations.
- Contextual toolbars for properties.
- Proper handling of Opacity and advanced Stroke styles.
