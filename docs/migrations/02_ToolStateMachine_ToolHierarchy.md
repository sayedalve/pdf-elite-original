# Subsystem Migration Report: ToolStateMachine & 12-Tool Hierarchy

## 1. Subsystem Overview
- **Module:** `ui::tools::ToolStateMachine`, `ui::tools::ITool`, and 12 Tool Implementations (`PanTool`, `SelectTool`, `TextSelectTool`, `ShapeTool` (Rectangle, Ellipse, Line, Arrow), `MarkupTool` (Highlight, Underline, Strikeout), `InkTool`, `FreeTextTool`, `StickyNoteTool`, `StampTool`, `EraserTool`, `InsertImageTool`)
- **Location:** `native/src/ui/include/tools/`, `native/src/ui/src/tools/`
- **Scope:** State-machine driven tool lifecycle, pointer event handling, overlay rendering, and command dispatch.

## 2. Reference Project & Clean-Room License Audit
- **Inspiration Source:** Xournal++ Tool FSM architecture (`ToolHandler`, finite states `Idle`, `Hovering`, `Dragging`, `Committed`)
- **Reference License:** GPL-2.0+
- **License Decision:** STRICT CLEAN-ROOM REIMPLEMENTATION. Implemented from first principles in C++20. No GPL code incorporated. All 12 tools interface through `ToolContext` using standard C++ functional callbacks and Win32/Direct2D primitives.

## 3. Architecture & Adaptations Made
- **Tool Lifecycle State Machine:** Tools transition cleanly across `Idle`, `Hovering`, `HoveringHandle`, `Dragging`, `InPlaceEditing`, `Committed`, and `Suspended`.
- **Tool Switching Safety:** Switching active tools while a drag is in progress automatically cancels active captures, commits or resets gestures, and cleans up overlays.
- **Direct2D Overlay Rendering:** Each tool renders its active transient geometry (marquee rect, ink strokes, shape outlines, transform bounding boxes) directly onto the Direct2D render target in screen DIP space.
- **Command Integration:** Commits mutations via `core::CommandStack`.

## 4. Old Code Removed / Superseded
- Replaced fragmented annotation handlers and legacy `InteractionManager` tool routing.
- Added all 12 tool implementations cleanly into `pdf_elite_ui` CMake target.
