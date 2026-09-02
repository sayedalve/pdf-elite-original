# PDF Elite

A high-performance desktop PDF editor powered by a custom C++ Direct2D rendering engine and PDFium.

## Project Overview

PDF Elite is built natively for Windows. It provides a robust suite of tools for annotating, viewing, and manipulating PDF documents without relying on bloated web wrappers.

### Key Features
- **Native Rendering**: Ultra-fast document scrolling and tile-rendering powered by PDFium.
- **Hardware Acceleration**: Smooth, high-fidelity UI overlays drawn via Direct2D on the main thread.
- **Rich Annotations**: Create and edit highlights, underlines, freehand ink, geometric shapes, and text notes.
- **Privacy First**: Fully offline, processing your documents entirely on your local machine.

## Project Structure

\\\
PDF-Elite/
+-- native/              # Core application source
¦   +-- src/ui/          # Windows UI, dialogs, Direct2D canvas overlays, and tool state
¦   +-- src/pdf_engine/  # PDFium wrapper, PDF document model, and annotation serialization
¦   +-- tests/           # Regression and unit tests
+-- resources/           # Application icons (SVG) and static assets used by the UI
+-- tests/               # PDF test fixtures
\\\

## Prerequisites

- **Windows 10/11**
- **Visual Studio 2022** (Desktop development with C++ workload)
- **CMake** (v3.20+)
- **PowerShell** (for build scripts)

## How to Build

We provide a simple PowerShell script to cleanly rebuild the Visual Studio solution via CMake:

\\\powershell
# Compiles the Release build
.\rebuild.ps1
\\\

The compiled executable will be located in \
ative/build/src/app/Release/PDFElite.exe\.

## Development Notes

The rendering pipeline is split into two asynchronous paths to ensure the UI remains responsive:
1. **Background Tiles**: Standard PDF page content is rendered to bitmaps by PDFium on background worker threads.
2. **Foreground Overlay**: Active tools, selections, and newly created uncommitted annotations are rendered synchronously via Direct2D on the main thread until they are committed to the PDF document.
