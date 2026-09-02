# Subsystem Migration Report: InputRouter & PointerCaptureService

## 1. Subsystem Overview
- **Module:** `ui::input::InputRouter`, `ui::input::PointerCaptureService`
- **Location:** `native/src/ui/include/input/`, `native/src/ui/src/input/`
- **Scope:** Centralized input routing, pointer capture lifecycle management, and spatial coordinate transformation (DIPs, physical screen, continuous canvas, PDF page points).

## 2. Reference Project & Clean-Room License Audit
- **Inspiration Source:** Xournal++ (`InputHandler`, `PointerCapture` RAII patterns) & PDF4QT (`InputEventDispatch`)
- **Reference License:** GPL-2.0+ (Xournal++) / LGPL-3.0 (PDF4QT)
- **License Decision:** STRICT CLEAN-ROOM REIMPLEMENTATION. Zero lines of reference source code were copied. The subsystem was written from scratch in native C++20 to integrate directly with Windows Win32 message loop (`::SetCapture`, `::ReleaseCapture`), Direct2D DPI scaling, and PDFium coordinate pipelines.

## 3. Architecture & Adaptations Made
- **Pointer Event Pipeline:** Ingests raw Win32 events and packages them into a structured `ui::input::PointerEvent` containing 4 coordinate tiers (Window DIPs, Physical Pixels, Continuous Canvas Space, and PDF Page Space).
- **Deterministic Pointer Capture:** `PointerCaptureService` manages RAII capture (`PointerCaptureGuard`) ensuring that when dragging starts, mouse capture is guaranteed, and upon pointer release, tool switch, `Escape` key, or window blur, capture is released without leaks.
- **Event Result Hierarchy:** Handled, Ignored, or Consumed event propagation.

## 4. Old Code Removed / Superseded
- Replaced monolithic, ad-hoc `switch` statements across `PdfViewer.cpp` and `MainWindow.cpp`.
- Removed leaky Win32 `SetCapture`/`ReleaseCapture` manual calls scattered across view classes.
