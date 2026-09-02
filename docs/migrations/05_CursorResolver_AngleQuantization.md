# Subsystem Migration Report: CursorResolver & Dynamic Angle Quantization

## 1. Subsystem Overview
- **Module:** `ui::selection::CursorResolver`
- **Location:** `native/src/ui/include/selection/CursorResolver.h`, `native/src/ui/src/selection/CursorResolver.cpp`
- **Scope:** Dynamic Win32 cursor resolution based on active tool state, handle angle quantization, object hover, text hover, and link hover states.

## 2. Reference Project & Clean-Room License Audit
- **Inspiration Source:** Okular (`CursorManager`) & PDF4QT (`ResizeCursorMapper`)
- **Reference License:** GPL-2.0+ / LGPL-3.0
- **License Decision:** STRICT CLEAN-ROOM REIMPLEMENTATION. Implemented cleanly in native C++20 for Win32 cursor API (`::LoadCursorW`).

## 3. Architecture & Adaptations Made
- **6-Tier Cursor Precedence Hierarchy:**
  1. Transform Handle Drag/Hover (8 angle-mapped resize cursors, rotation `IDC_SIZEALL`, body `IDC_SIZEALL`).
  2. In-Place Text Editing (`IDC_IBEAM`).
  3. Hyperlink Hover (`IDC_HAND`).
  4. Object Hover in Select Tool (`IDC_SIZEALL`).
  5. Text Hover in Select/Text Tools (`IDC_IBEAM`).
  6. Tool Fallback (Pan -> `IDC_HAND`/`IDC_SIZEALL`, Ink/Shapes -> `IDC_CROSS`, Default -> `IDC_ARROW`).
- **Angle Quantization to 4 Bidirectional System Cursors:**
  - $[0^\circ, 22.5^\circ) \cup [157.5^\circ, 180^\circ) \rightarrow$ `IDC_SIZEWE` (Horizontal)
  - $[22.5^\circ, 67.5^\circ) \rightarrow$ `IDC_SIZENWSE` (Diagonal NW-SE)
  - $[67.5^\circ, 112.5^\circ) \rightarrow$ `IDC_SIZENS` (Vertical)
  - $[112.5^\circ, 157.5^\circ) \rightarrow$ `IDC_SIZENESW` (Diagonal NE-SW)

## 4. Old Code Removed / Superseded
- Replaced hardcoded static cursor logic across `PdfViewer::OnSetCursor`.
