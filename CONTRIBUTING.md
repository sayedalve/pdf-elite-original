# Contributing to PDF Elite

Thank you for your interest in contributing to the PDF Elite native C++ client!

## Development Philosophy

- **Native Performance**: Always prefer native Windows API and Direct2D primitives for rendering rather than integrating heavy frameworks.
- **Separation of Concerns**: 
  - \
ative/src/ui/\: Handles all input routing, UI tool state machines, and Direct2D overlay rendering.
  - \
ative/src/pdf_engine/\: The source of truth for the PDF structure. The UI layer should NOT interact with PDFium directly; it should delegate commands to the engine layer.

## Build and Testing

- Ensure you run \.\rebuild.ps1\ to verify your changes compile cleanly under MSVC.
- Run the regression suite (\RegressionSuite.exe\) located in the build output folder before submitting patches.

## Pull Requests

1. Fork the repository and create a feature branch.
2. Ensure your code adheres to standard modern C++ guidelines (C++17/C++20).
3. Document any complex rendering lifecycle changes, especially regarding the dual-render pipeline.
