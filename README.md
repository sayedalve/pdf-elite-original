# PDF Elite

A professional, high-performance desktop PDF editor designed for privacy, precision, and speed. PDF Elite combines a powerful native C++ rendering engine with a modern web-based UI and Java backend to deliver a complete PDF manipulation suite directly on your machine.

## Features

- **Native PDF Rendering & Interaction**: Powered by a custom C++ Direct2D and PDFium stack for lightning-fast scrolling, rendering, and interaction.
- **Rich Annotations**: Add highlights, underlines, squiggly lines, strikeouts, shapes (rectangles, ellipses, lines), ink (freehand drawing), and sticky notes. 
- **Full Privacy**: All processing runs locally on your machine.
- **Selection & Interaction**: Advanced text selection, context menus, and contextual property toolbars for seamless editing.

## Technology Stack

PDF Elite is a hybrid desktop application utilizing multiple specialized layers:

1. **Native Client (C++)**: Uses Direct2D for rendering and PDFium for core PDF parsing. Handles raw graphics, annotation layers, and UI interaction natively on Windows.
2. **Backend (Java / Spring Boot)**: Handles heavy PDF processing, orchestrates conversions, and acts as a local proxy for file management.
3. **Frontend (React / TypeScript / Vite)**: Provides the rich graphical user interface, built with Mantine UI and TailwindCSS.

## Project Structure

\\\
PDF-Elite/
+-- native/              # Native C++ application (Direct2D UI + PDFium Engine)
¦   +-- src/ui/          # UI components, tool state machines, and canvas overlay
¦   +-- src/pdf_engine/  # PDFium wrapper, document model, and annotation commands
+-- app/                 # Java Spring Boot backend services
¦   +-- core/            # Core processing logic
¦   +-- common/          # Shared models and utilities
+-- frontend/editor/     # React SPA frontend (Vite, TypeScript, Mantine)
+-- dist/                # Final compiled desktop application and installers
+-- resources/           # Application icons, translations, and static assets
\\\

## Prerequisites

To build and run PDF Elite, you will need:
- **Windows 10/11** (for the native Direct2D client)
- **Visual Studio 2022** (with C++ Desktop Development workload)
- **CMake** (v3.20+)
- **Java JDK 21+**
- **Node.js 18+** & **npm**
- **Task** (https://taskfile.dev) for unified build commands

## How to Build and Run

PDF Elite uses [Task](https://taskfile.dev/) as a unified command runner for the web and backend stacks, and PowerShell scripts for the native Windows client.

### Building the Native C++ Client
The native client must be compiled for Windows. Open a Developer Command Prompt or PowerShell and run:
\\\powershell
# Rebuilds the C++ project using CMake and Visual Studio
.\rebuild.ps1
\\\
The compiled executable will be placed at \
ative/build/src/app/Release/PDFElite.exe\.

### Building the Java Backend and Web Frontend
Use the unified task runner to install dependencies and build:
\\\ash
task install
task build
\\\

### Running the Application
During development, you can run the components concurrently:
- \	ask dev\ (Starts the Java backend and React frontend development servers)
- Run \PDFElite.exe\ for the native desktop experience.

## Dependency Information

PDF Elite relies on the following key open-source technologies:
- **PDFium** (Google/Foxit) - BSD-3-Clause - Core PDF rendering and manipulation.
- **Direct2D** (Microsoft) - Native Windows hardware-accelerated 2D graphics.
- **Spring Boot** - Apache 2.0 - Backend services.
- **React** & **Vite** - MIT - Frontend UI framework.

See \THIRD_PARTY.md\ for full dependency details.

## Development Notes

- **Taskfile**: Always check \	ask --list\ for available development commands.
- **Native Annotations**: The native C++ layer uses a dual-render pipeline. Background tiles are rendered on worker threads, while active annotations and interactions are drawn on an overlay via Direct2D on the main thread.
- **Code Formatting**: Run \	ask format\ to auto-fix formatting across all non-native components.
