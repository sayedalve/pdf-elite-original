# Rendering Boundary

The Rendering Boundary enforces that the PdfViewer receives an asynchronous RenderResult via RenderRequest and uses RenderController, strictly separating UI dispatch threads from background PDFium rendering.
