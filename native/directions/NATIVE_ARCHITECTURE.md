# Native Architecture

The Native Architecture of PDF Elite separates UI, Application, and Engine to prevent UI code from directly using PDFium internals. All communications flow from UI -> Core (DocumentSession/DocumentController) -> Engine (EngineAdapter) -> PDFium.
