import { useState, useEffect, useRef, useCallback } from "react";

type ZoomState = {
  scale: number;
  percentage: number;
};

export function useZoomSync(initialScale = 1.2) {
  const [zoom, setZoom] = useState<ZoomState>({
    scale: initialScale,
    percentage: Math.round(initialScale * 100),
  });

  const containerRef = useRef<HTMLDivElement>(null);
  const isPinching = useRef(false);
  const lastPinchDistance = useRef(0);
  const lastScale = useRef(initialScale);

  // Synchronized setter - single source of truth
  const setScale = useCallback(
    (newScale: number, opts?: { clamp?: boolean }) => {
      const clamped =
        opts?.clamp !== false
          ? Math.min(Math.max(newScale, 0.25), 5.0)
          : newScale;

      const rounded = Math.round(clamped * 100) / 100;
      setZoom({
        scale: rounded,
        percentage: Math.round(rounded * 100),
      });
      lastScale.current = rounded;
    },
    [],
  );

  const zoomIn = useCallback(() => {
    const steps = [0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75, 2, 2.5, 3, 4, 5];
    const current = zoom.scale;
    const next =
      steps.find((s) => s > current + 0.01) || Math.min(current * 1.25, 5);
    setScale(next);
  }, [zoom.scale, setScale]);

  const zoomOut = useCallback(() => {
    const steps = [0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75, 2, 2.5, 3, 4, 5];
    const current = zoom.scale;
    const reversed = [...steps].reverse();
    const next =
      reversed.find((s) => s < current - 0.01) || Math.max(current * 0.8, 0.25);
    setScale(next);
  }, [zoom.scale, setScale]);

  const fitWidth = useCallback(() => {
    // Calculate fit width based on container
    if (containerRef.current) {
      const containerWidth = containerRef.current.clientWidth - 48; // padding
      const pageWidth = 595; // A4 at 72dpi approximation
      const scale = containerWidth / pageWidth;
      setScale(Math.min(scale, 2.5));
    } else {
      setScale(1.2);
    }
  }, [setScale]);

  const fitPage = useCallback(() => {
    if (containerRef.current) {
      const containerHeight = containerRef.current.clientHeight - 48;
      const containerWidth = containerRef.current.clientWidth - 48;
      const pageWidth = 595;
      const pageHeight = 842;
      const scaleW = containerWidth / pageWidth;
      const scaleH = containerHeight / pageHeight;
      setScale(Math.min(scaleW, scaleH, 1.5));
    } else {
      setScale(0.9);
    }
  }, [setScale]);

  const actualSize = useCallback(() => {
    setScale(1);
  }, [setScale]);

  // CRITICAL: Touchpad pinch zoom + Ctrl+wheel handling
  useEffect(() => {
    const container = containerRef.current;
    if (!container) return;

    let wheelTimeout: number | null = null;

    const handleWheel = (e: WheelEvent) => {
      // Detect pinch gesture: ctrlKey + wheel on Windows precision touchpad
      // OR check if it's a pinch (ctrlKey is true for pinch on Windows)
      if (e.ctrlKey) {
        e.preventDefault();

        // Calculate zoom delta - inverted for natural feel
        // deltaY negative = zoom in (pinch out), positive = zoom out (pinch in)
        const delta = -e.deltaY;
        const factor = delta > 0 ? 1.05 : 0.95;
        const newScale = lastScale.current * factor;

        // Throttle slightly for smoothness
        if (wheelTimeout) cancelAnimationFrame(wheelTimeout);
        wheelTimeout = requestAnimationFrame(() => {
          setScale(newScale);
        }) as unknown as number;

        return;
      }

      // Plain wheel = scroll (do not prevent default)
      // Let browser handle scrolling naturally
    };

    // For touch devices - handle gesture events
    const handleTouchStart = (e: TouchEvent) => {
      if (e.touches.length === 2) {
        isPinching.current = true;
        const dx = e.touches[0].clientX - e.touches[1].clientX;
        const dy = e.touches[0].clientY - e.touches[1].clientY;
        lastPinchDistance.current = Math.sqrt(dx * dx + dy * dy);
        e.preventDefault();
      }
    };

    const handleTouchMove = (e: TouchEvent) => {
      if (e.touches.length === 2 && isPinching.current) {
        const dx = e.touches[0].clientX - e.touches[1].clientX;
        const dy = e.touches[0].clientY - e.touches[1].clientY;
        const distance = Math.sqrt(dx * dx + dy * dy);

        if (lastPinchDistance.current > 0) {
          const factor = distance / lastPinchDistance.current;
          const newScale = lastScale.current * factor;
          setScale(newScale);
        }
        lastPinchDistance.current = distance;
        e.preventDefault();
      }
    };

    const handleTouchEnd = () => {
      isPinching.current = false;
      lastPinchDistance.current = 0;
    };

    // Use passive: false for wheel to allow preventDefault when ctrlKey
    container.addEventListener("wheel", handleWheel, { passive: false });
    container.addEventListener("touchstart", handleTouchStart, {
      passive: false,
    });
    container.addEventListener("touchmove", handleTouchMove, {
      passive: false,
    });
    container.addEventListener("touchend", handleTouchEnd);

    return () => {
      container.removeEventListener("wheel", handleWheel);
      container.removeEventListener("touchstart", handleTouchStart);
      container.removeEventListener("touchmove", handleTouchMove);
      container.removeEventListener("touchend", handleTouchEnd);
      if (wheelTimeout) cancelAnimationFrame(wheelTimeout);
    };
  }, [setScale]);

  // Keyboard zoom
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if ((e.ctrlKey || e.metaKey) && (e.key === "+" || e.key === "=")) {
        e.preventDefault();
        zoomIn();
      } else if ((e.ctrlKey || e.metaKey) && e.key === "-") {
        e.preventDefault();
        zoomOut();
      } else if ((e.ctrlKey || e.metaKey) && e.key === "0") {
        e.preventDefault();
        actualSize();
      }
    };
    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [zoomIn, zoomOut, actualSize]);

  return {
    zoom,
    containerRef,
    setScale,
    zoomIn,
    zoomOut,
    fitWidth,
    fitPage,
    actualSize,
    scale: zoom.scale,
    percentage: zoom.percentage,
  };
}
