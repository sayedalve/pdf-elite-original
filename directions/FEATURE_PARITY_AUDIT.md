# Feature Parity Audit (Phase 31)

This document tracks the feature parity between the legacy Tauri/React application and the new Native C++/Direct2D application.

| Feature | Legacy System | Native Implementation | Status |
|---|---|---|---|
| Open PDF | Working | Working (`PdfDocument::Load`) | Complete |
| Save/Save As | Working | Stubs Created | In Progress |
| Render PDF | Working (PDF.js) | Working (PDFium + D2D) | Complete |
| Fast Scrolling | Janky on 100+ pgs | Smooth (RenderWorker + D2D) | Complete |
| Dark UI Theme | CSS Custom | Custom D2D Framework | Complete |
| Left Sidebar Rail | React Components | `components::Sidebar` | Complete |
| Text Selection | Working | Stubs (`TextSelection`) | In Progress |
| Text Search | Working | Stubs (`SearchEngine`) | In Progress |
| Text Editing | Working | Stubs (`TextEditor`) | Pending |
| Add Annotation | Working | Stubs (`AnnotationEngine`) | Pending |
| Undo/Redo | Working | Base Structure (`CommandStack`) | Complete |
| Hot Reload | Vite HMR | Native `ThemeManager` | Complete |
| Installer Size | >250 MB | Target <= 120 MB | Pending |
