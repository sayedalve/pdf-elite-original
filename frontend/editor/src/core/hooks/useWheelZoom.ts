import { RefObject, useEffect } from "react";

interface UseWheelZoomOptions {
  /**
   * Element the wheel listener should be bound to.
   */
  ref: RefObject<Element | null>;
  /**
   * Optional container to apply CSS transform to during active pinch gesture for Phase 6 two-stage zoom.
   */
  zoomContainerRef?: RefObject<HTMLElement | null>;
  /**
   * Callback executed when the hook decides to zoom in (discrete).
   * Used as a fallback when setZoomLevel is not provided.
   */
  onZoomIn: () => void;
  /**
   * Callback executed when the hook decides to zoom out (discrete).
   * Used as a fallback when setZoomLevel is not provided.
   */
  onZoomOut: () => void;
  /**
   * Optional callback to set the zoom to an exact factor (e.g. 1.5 = 150%).
   * When provided, pinch/wheel zoom is smooth and continuous instead of discrete.
   */
  setZoomLevel?: (factor: number, center?: { vx: number; vy: number }) => void;
  /**
   * Optional callback to get the current zoom factor (e.g. 1.0 = 100%).
   * Required for smooth zoom to work correctly.
   */
  getZoomFactor?: () => number;
  /**
   * Whether the wheel listener should be active.
   */
  enabled?: boolean;
  /**
   * How much delta needs to accumulate before a zoom action is triggered
   * when using the discrete (non-smooth) fallback path.
   * Defaults to 10.
   */
  threshold?: number;
  /**
   * Whether a Ctrl/Cmd modifier is required for zooming. Defaults to true so
   * we only react to pinch gestures and intentional ctrl+wheel zooming.
   */
  requireModifierKey?: boolean;
}

/**
 * Shared hook for handling wheel-based zoom across components.
 *
 * Key fixes over the previous implementation:
 * 1. Binds in CAPTURE phase so events are intercepted before @embedpdf's inner
 *    interaction manager can call stopPropagation() on them.
 * 2. When setZoomLevel + getZoomFactor are provided, applies smooth continuous
 *    zoom using fractional deltas from the trackpad pinch gesture, rather than
 *    accumulating to an arbitrary threshold and then doing a discrete jump.
 * 3. Normal vertical scrolling (no ctrlKey) is untouched — only pinch or
 *    Ctrl+wheel triggers zoom.
 */
export function useWheelZoom({
  ref,
  zoomContainerRef,
  onZoomIn,
  onZoomOut,
  setZoomLevel,
  getZoomFactor,
  enabled = true,
  threshold = 10,
  requireModifierKey = true,
}: UseWheelZoomOptions) {
  useEffect(() => {
    if (!enabled) {
      return;
    }

    const element = ref.current;
    if (!element) {
      return;
    }

    // Discrete fallback accumulator (used when smooth zoom isn't wired up)
    let accumulator = 0;

    // Smooth zoom state to prevent lagging behind async PDF.js state updates
    let targetZoom = 0;

    // Two-stage zoom state (Phase 6)
    let isGesturing = false;
    let gestureTimeout: ReturnType<typeof setTimeout> | null = null;
    let startZoomFactor = 1;
    let startVx = 0;
    let startVy = 0;

    const handleWheel = (event: Event) => {
      const wheelEvent = event as WheelEvent;
      // Trackpad pinch-to-zoom fires with ctrlKey=true on both Mac and Windows.
      // Cmd+scroll (metaKey) is used for OS-level page zoom and shouldn't trigger internal canvas zoom.
      const hasModifier = wheelEvent.ctrlKey;

      if (requireModifierKey && !hasModifier) {
        // Plain scroll — do not interfere.
        return;
      }

      // We are handling the zoom gesture — prevent the browser's native
      // pinch-to-zoom / page-zoom from firing as well.
      wheelEvent.preventDefault();
      wheelEvent.stopPropagation();

      // ── Smooth continuous zoom path ─────────────────────────────────────────
      // Trackpad pinch gestures produce small, fractional deltaY values at high
      // frequency.  Map them directly to a zoom factor change so the experience
      // feels native and fluid, exactly like a modern PDF reader.
      if (setZoomLevel && getZoomFactor) {
        if (!isGesturing) {
          isGesturing = true;
          startZoomFactor = getZoomFactor();
          targetZoom = startZoomFactor;

          // Calculate origin once for the entire gesture to avoid shifting when the rect transforms
          const elementToMeasure = zoomContainerRef?.current || element;
          const rect = elementToMeasure.getBoundingClientRect();
          const originX = wheelEvent.clientX - rect.left;
          const originY = wheelEvent.clientY - rect.top;

          startVx = originX / rect.width;
          startVy = originY / rect.height;

          if (zoomContainerRef?.current) {
            zoomContainerRef.current.style.transformOrigin = `${originX}px ${originY}px`;
            zoomContainerRef.current.style.transition = "none";
            zoomContainerRef.current.style.willChange = "transform";
          }
        }

        // Sensitivity: 0.01 is a typical multiplier for deltaY.
        const sensitivity = 0.01;
        const delta = wheelEvent.deltaY;

        // Zoom direction: negative deltaY = pinch-open = zoom in.
        // Math.exp ensures symmetric zooming in both directions.
        const newFactor = targetZoom * Math.exp(-delta * sensitivity);

        // Clamp to reasonable bounds (20% – 500%)
        const clamped = Math.min(Math.max(newFactor, 0.2), 5.0);
        targetZoom = clamped;

        if (zoomContainerRef?.current) {
          // Stage 1: Temporary CSS transform for immediate 60fps feedback without PDF.js rerenders
          const scaleRatio = targetZoom / startZoomFactor;
          zoomContainerRef.current.style.transform = `scale(${scaleRatio})`;
        } else {
          // Fallback if no container provided: spam PDF.js with updates
          setZoomLevel(clamped, { vx: startVx, vy: startVy });
        }

        if (gestureTimeout) {
          clearTimeout(gestureTimeout);
        }

        // Stage 2: When gesture stops, remove CSS transform and trigger full high-quality render
        gestureTimeout = setTimeout(() => {
          isGesturing = false;
          if (zoomContainerRef?.current) {
            zoomContainerRef.current.style.transform = "";
            zoomContainerRef.current.style.willChange = "auto";
            setZoomLevel(targetZoom, { vx: startVx, vy: startVy });
          }
        }, 150);

        return;
      }

      // ── Discrete fallback (no smooth zoom wired) ────────────────────────────
      accumulator += wheelEvent.deltaY;

      if (accumulator <= -threshold) {
        onZoomIn();
        accumulator = 0;
      } else if (accumulator >= threshold) {
        onZoomOut();
        accumulator = 0;
      }
    };

    // CRITICAL: use { capture: true } so our handler fires BEFORE the
    // @embedpdf inner canvas interaction manager, which calls stopPropagation()
    // on wheel events in the bubble phase.  Without capture mode, pinch gestures
    // are swallowed and never reach this handler.
    element.addEventListener("wheel", handleWheel, {
      passive: false,
      capture: true,
    });

    return () => {
      element.removeEventListener("wheel", handleWheel, { capture: true });
      if (gestureTimeout) clearTimeout(gestureTimeout);
    };
  }, [
    ref,
    zoomContainerRef,
    onZoomIn,
    onZoomOut,
    setZoomLevel,
    getZoomFactor,
    enabled,
    threshold,
    requireModifierKey,
  ]);
}
