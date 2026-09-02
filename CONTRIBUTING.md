# Contributing to PDF Elite

Thank you for your interest in contributing to PDF Elite! This document outlines our development guidelines to help you get started.

## Development Workflow

We use [Task](https://taskfile.dev/) as our unified command runner. Familiarize yourself with it:
- \	ask --list\ — See all available commands.
- \	ask install\ — Install all dependencies.
- \	ask dev\ — Start backend and frontend concurrently.
- \	ask build\ — Build all components.
- \	ask check\ — Full quality gate (lint + typecheck + test).

## Native C++ Development

- The native Windows client is located in the \
ative/\ directory.
- Use \.\rebuild.ps1\ to cleanly rebuild the Visual Studio / CMake solution.
- **Rule of Thumb**: Interactions (mouse/keyboard) are handled in the \ui/\ component via \InteractionManager\, which delegates commands to the \pdf_engine/\ component. Always maintain strict separation between UI overlays (Direct2D) and the underlying PDFium document model.

## Backend (Java) Guidelines

- Keep \	ask backend:check\ passing.
- Code style is strictly enforced by Spotless. Run \	ask backend:format\ before committing.
- Ensure all API endpoints are RESTful and documented.

## Frontend (React) Guidelines

- **Imports**: ALWAYS use \@app/*\ for imports. Do not use \@core/*\ or \@proprietary/*\ unless explicitly wrapping or extending a lower layer implementation.
- All \VITE_*\ variables must be declared in the appropriate committed env file (e.g., \.env\, \.env.saas\).
- UI logic should remain strictly separated from global file state (\FileContext.tsx\).

## Submitting Pull Requests

1. Create a feature branch (\eature/your-feature-name\).
2. Run \	ask check\ to ensure all tests, linting, and formatting pass.
3. Keep PRs focused on a single logical change.
4. Provide a clear summary of your changes in the PR description.
