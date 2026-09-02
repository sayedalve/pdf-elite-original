# Subsystem Migration Report: KineticScrollFilter & 60Hz Physics Timer

## 1. Subsystem Overview
- **Module:** `ui::viewport::KineticScrollFilter`
- **Location:** `native/src/ui/include/viewport/KineticScrollFilter.h`, `native/src/ui/src/viewport/KineticScrollFilter.cpp`
- **Scope:** Smooth inertial kinetic scrolling with exponential velocity decay, velocity clamping, and 60Hz `WM_TIMER` animation stepping.

## 2. Reference Project & Clean-Room License Audit
- **Inspiration Source:** Xournal++ (`KineticScrollFilter`, inertial physics simulation)
- **Reference License:** GPL-2.0+
- **License Decision:** STRICT CLEAN-ROOM REIMPLEMENTATION. The differential equations and decay functions were implemented from first principles in C++20 for Win32 timer and Direct2D view rendering.

## 3. Architecture & Adaptations Made
- **Impulse Ingestion:** Mouse wheel notches and trackpad gestures are converted to initial velocity impulses ($v \leftarrow v + \Delta \times 2.5$).
- **Exponential Velocity Decay:** Continuous time-dependent decay ($v(t) = v_0 e^{-\alpha \Delta t}$), defaulting to $35\%$ decay per $16\,\text{ms}$ tick ($\alpha \approx 26.92$).
- **Analytical Closed-Form Displacement Integral:** Computes exact continuous displacement over time step $\Delta t$:
  $$\Delta x = \int_0^{\Delta t} v(t) dt = \frac{v_0}{\alpha} (1 - e^{-\alpha \Delta t})$$
  with numerical stability handling for $\alpha \le 10^{-6}$ ($\Delta x = v_0 \cdot \Delta t$). This guarantees absolute frame-rate invariance across 60Hz, 120Hz, 240Hz, and variable G-Sync/FreeSync refresh displays.
- **Velocity Clamping:** Clamps velocity strictly to $[-8000, 8000]\,\text{px/s}$ to prevent uncontrollable overshoot on rapid wheel spins.
- **Quiescence Threshold:** When speed drops below $0.1\,\text{px/s}$, motion ceases immediately (`IsActive() == false`), terminating repaint invalidations and freeing CPU/GPU cycles.
- **60Hz Timer Dispatch:** `MainWindow` runs a 16ms `WM_TIMER` (`TIMER_PHYSICS`) that steps `PdfViewer::UpdatePhysics()` only while kinetic motion is active.

## 4. Old Code Removed / Superseded
- Replaced discrete, jarring instant-jump scrolling in `PdfViewer::OnMouseWheel`.
- Replaced frame-rate-dependent forward Euler discretization with exact analytical closed-form integral.
