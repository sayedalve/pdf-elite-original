import { useEffect, useState, useCallback, useRef } from "react";

export type ToolMode =
  | "view"
  | "comment"
  | "edit"
  | "organize"
  | "search"
  | "tools";
export type TempTool =
  | "highlight"
  | "note"
  | "draw"
  | "select"
  | "area-highlight"
  | "underline"
  | "strikeout"
  | "text"
  | "replaceImage"
  | "removeImage"
  | null;

type ToolState = {
  mode: ToolMode;
  tempTool: TempTool;
  highlightColor: string;
  highlightOpacity: number;
  lastHighlightColor: string;
};

export const HIGHLIGHT_COLORS = [
  { id: "yellow", hex: "#fef08a", name: "Yellow" },
  { id: "green", hex: "#bbf7d0", name: "Green" },
  { id: "blue", hex: "#bfdbfe", name: "Blue" },
  { id: "pink", hex: "#fbcfe8", name: "Pink" },
  { id: "orange", hex: "#fed7aa", name: "Orange" },
  { id: "red", hex: "#fca5a5", name: "Red" },
];

// Shared with every other highlight-color surface (annotate side panel,
// workbench bar, selection-menu popup) so a chosen color persists and reloads
// consistently. This hook historically used a DIFFERENT key
// ("pdf-elite:highlight-color", colon) than the rest of the app
// ("pdf-elite-highlight-color", hyphen), so its persistence never round-tripped
// with the others.
const HIGHLIGHT_COLOR_STORAGE_KEY = "pdf-elite-highlight-color";
const LEGACY_HIGHLIGHT_COLOR_STORAGE_KEY = "pdf-elite:highlight-color";

export function useToolLifecycle() {
  const [state, setState] = useState<ToolState>({
    mode: "view",
    tempTool: null,
    highlightColor: "#fef08a",
    highlightOpacity: 30,
    lastHighlightColor: "#fef08a",
  });

  const previousModeRef = useRef<ToolMode>("view");

  // Apply a highlight color to this store WITHOUT broadcasting. Used by the
  // event listeners below so an incoming color change (from the selection-menu
  // popup, the annotate side panel, or the workbench bar) updates this store —
  // which is what pushes the embedpdf highlight tool's default via ViewerShell —
  // without echoing the event back out and looping. Guarded so a redundant
  // value is a no-op (no extra render, no loop).
  const applyHighlightColor = useCallback((color: string) => {
    if (!color) return;
    setState((s) =>
      s.highlightColor === color
        ? s
        : { ...s, highlightColor: color, lastHighlightColor: color },
    );
    try {
      localStorage.setItem(HIGHLIGHT_COLOR_STORAGE_KEY, color);
    } catch {
      /* ignore */
    }
  }, []);

  // Public setter used by the top (contextual) toolbar's color picker. Updates
  // this store AND broadcasts the canonical "pdf-elite-color-change" event so
  // the other highlight-color surfaces (side panel, workbench bar, popup
  // swatches) stay in sync.
  const setHighlightColor = useCallback(
    (color: string) => {
      applyHighlightColor(color);
      window.dispatchEvent(
        new CustomEvent("pdf-elite-color-change", {
          detail: { type: "highlight", color },
        }),
      );
    },
    [applyHighlightColor],
  );

  // Apply highlight opacity WITHOUT broadcasting (used by the event listener
  // below to avoid a re-dispatch loop). Guarded so a redundant value is a no-op.
  const applyHighlightOpacity = useCallback((opacity: number) => {
    setState((s) =>
      s.highlightOpacity === opacity ? s : { ...s, highlightOpacity: opacity },
    );
  }, []);

  // Public setter: updates this store AND broadcasts so other surfaces sync.
  const setHighlightOpacity = useCallback(
    (opacity: number) => {
      applyHighlightOpacity(opacity);
      window.dispatchEvent(
        new CustomEvent("set-highlight-color", {
          detail: { opacity },
        }),
      );
    },
    [applyHighlightOpacity],
  );

  // Listen for highlight-color changes from every other surface. The app uses
  // two historical event names: the canonical "pdf-elite-color-change" (fired by
  // the annotate panel, the workbench bar, and the text-markup selection menu)
  // and the legacy "set-highlight-color" (fired by the area-highlight swatches).
  // This hook previously listened ONLY to the legacy name, so recoloring a text
  // highlight from the selection-menu popup never reached this store — and since
  // THIS store is what drives setToolDefaults("highlight", …) through ViewerShell,
  // the next highlight reverted to the old default. Listen to both and apply
  // WITHOUT re-dispatching to avoid a feedback loop.
  useEffect(() => {
    const handleColorChange = (e: Event) => {
      const ce = e as CustomEvent<{ type?: string; color?: string }>;
      if (ce.detail?.type === "highlight" && ce.detail.color) {
        applyHighlightColor(ce.detail.color);
      }
    };
    const handleLegacySetColor = (e: Event) => {
      const ce = e as CustomEvent<{ color?: string; opacity?: number }>;
      if (ce.detail?.color) applyHighlightColor(ce.detail.color);
      // Apply (do NOT call the dispatching setter) — this legacy event is what
      // setHighlightOpacity itself emits, so calling it back here would recurse.
      if (ce.detail?.opacity !== undefined)
        applyHighlightOpacity(ce.detail.opacity);
    };
    window.addEventListener("pdf-elite-color-change", handleColorChange);
    window.addEventListener("set-highlight-color", handleLegacySetColor);
    return () => {
      window.removeEventListener("pdf-elite-color-change", handleColorChange);
      window.removeEventListener("set-highlight-color", handleLegacySetColor);
    };
  }, [applyHighlightColor, applyHighlightOpacity]);

  // Restore persisted color on mount (shared key with the rest of the app;
  // falls back to the legacy colon key for a one-time migration).
  useEffect(() => {
    try {
      const saved =
        localStorage.getItem(HIGHLIGHT_COLOR_STORAGE_KEY) ||
        localStorage.getItem(LEGACY_HIGHLIGHT_COLOR_STORAGE_KEY);
      if (saved) {
        setState((s) => ({
          ...s,
          highlightColor: saved,
          lastHighlightColor: saved,
        }));
      }
    } catch {
      /* ignore */
    }
  }, []);

  const setMode = useCallback(
    (mode: ToolMode) => {
      // When switching major modes, cancel temp tools
      if (mode !== state.mode) {
        previousModeRef.current = state.mode;
        setState((s) => ({ ...s, mode, tempTool: null }));
      }
    },
    [state.mode],
  );

  const setTempTool = useCallback((tool: TempTool) => {
    setState((s) => ({ ...s, tempTool: tool }));
  }, []);

  const cancelTempTool = useCallback(() => {
    setState((s) => ({ ...s, tempTool: null }));
  }, []);

  // CRITICAL: Escape handling - cancels temp tool, closes popovers, exits modes
  useEffect(() => {
    const handleEscape = (e: KeyboardEvent) => {
      if (e.key === "Escape") {
        // Priority: temp tool > search > back to view
        if (state.tempTool) {
          e.preventDefault();
          e.stopPropagation();
          cancelTempTool();
          return;
        }

        if (state.mode === "search") {
          e.preventDefault();
          setMode("view");
          return;
        }

        // If in comment/edit/organize, maybe go back to view? But keep mode, just clear temp
        // Spec says Escape must behave predictably
        if (state.mode !== "view") {
          // For now, clear temp and stay in mode - user can press again to go to view if needed
          // Actually spec: Escape cancels current temporary operation, clear selection, close popovers, exit temp tool state
          // So we don't switch mode on first Escape unless it's search
          // Second Escape could go to view - implement double-escape
          const lastEscape = (window as any).__lastEscape || 0;
          const now = Date.now();
          if (now - lastEscape < 500) {
            setMode("view");
          }
          (window as any).__lastEscape = now;
        }
      }
    };

    // Use capture to ensure we get Escape before other handlers
    window.addEventListener("keydown", handleEscape, true);
    return () => window.removeEventListener("keydown", handleEscape, true);
  }, [state.tempTool, state.mode, cancelTempTool, setMode]);

  // Clicking another major mode must cancel incompatible temp modes (handled in setMode)
  // Also clicking outside should cancel? That's UI-specific

  return {
    ...state,
    setMode,
    setTempTool,
    cancelTempTool,
    setHighlightColor,
    setHighlightOpacity,
    highlightColors: HIGHLIGHT_COLORS,
    isHighlightActive: state.tempTool === "highlight",
  };
}
