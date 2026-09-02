# UI Engine Boundary

The UI is completely decoupled from the PDF engine. The UI only uses interfaces provided by the core::interfaces::dom namespace, which abstracts IDocument, IPage, IAnnotation, ITextPage, etc.
