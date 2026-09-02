/* eslint-disable */
import React, {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from "react";
import { Highlighter, X } from "lucide-react";
type ToolMode = string;
type TempTool = string;

export interface DocumentWorkspaceProps {
  scale: number;
  page: number;
  totalPages: number;
  onPageChange: (page: number) => void;
  containerRef: React.RefObject<HTMLDivElement>;
  mode: ToolMode;
  tempTool: TempTool;
  highlightColor?: string;
  onHighlight?: (payload: {
    text: string;
    page: number;
    color: string;
  }) => void;
}

type SelectionTooltip = {
  visible: boolean;
  x: number;
  y: number;
  text: string;
  pageIndex: number;
};

const PAGE_WIDTH = 816; // US Letter / A4 approx at 96dpi rendering - width used for fit calc
const PAGE_HEIGHT = 1056;
const PAGE_CONTENT_PADDING = 72;
const FIT_HORIZONTAL_MARGIN = 64; // breathing room on both sides for document-first feel

/**
 * DocumentWorkspace
 * Central PDF reading area - document is dominant, chrome is subordinate.
 * Follows pdf-viewer-ui-ux-skill principles:
 * - document-first, professional hierarchy
 * - progressive disclosure (highlight action only when selection + tempTool)
 * - no duplicate controls
 * - consistent tokens (--workspace-paper-bg, --page-shadow, --accent #79AEFF)
 */
export const DocumentWorkspace: React.FC<DocumentWorkspaceProps> = ({
  scale,
  page,
  totalPages,
  onPageChange,
  containerRef,
  mode,
  tempTool,
  highlightColor = "#fef08a",
  onHighlight,
}) => {
  const pagesWrapperRef = useRef<HTMLDivElement>(null);
  const pageRefs = useRef<Map<number, HTMLDivElement>>(new Map());
  const [selectionTooltip, setSelectionTooltip] = useState<SelectionTooltip>({
    visible: false,
    x: 0,
    y: 0,
    text: "",
    pageIndex: 1,
  });

  const safeTotalPages = Math.max(1, totalPages || 4);

  // Keep page refs map stable
  const setPageRef = useCallback(
    (pageIndex: number, el: HTMLDivElement | null) => {
      if (el) pageRefs.current.set(pageIndex, el);
      else pageRefs.current.delete(pageIndex);
    },
    [],
  );

  // Scroll to active page when `page` prop changes externally (e.g., from left rail / toolbar)
  useEffect(() => {
    const target = pageRefs.current.get(page);
    if (target && containerRef.current) {
      // Use smooth scroll, but respect prefers-reduced-motion via CSS media query fallback
      target.scrollIntoView({
        behavior: "smooth",
        block: "start",
        inline: "nearest",
      });
    }
  }, [page, containerRef]);

  // Intersection observer to sync current visible page -> onPageChange
  // Professional hierarchy: page navigation stays in sync without adding another paginator.
  useEffect(() => {
    const root = containerRef.current;
    if (!root) return;

    const observer = new IntersectionObserver(
      (entries) => {
        // Find most visible page
        const visible = entries
          .filter((e) => e.isIntersecting)
          .sort((a, b) => b.intersectionRatio - a.intersectionRatio);
        if (visible[0]) {
          const pageNum = Number(
            (visible[0].target as HTMLElement).dataset.pageNumber,
          );
          if (pageNum && pageNum !== page) {
            onPageChange(pageNum);
          }
        }
      },
      {
        root,
        // Thresholds for stable detection during smooth scroll
        threshold: [0.2, 0.5, 0.8],
        rootMargin: "-10% 0px -60% 0px",
      },
    );

    pageRefs.current.forEach((el) => observer.observe(el));

    return () => observer.disconnect();
    // page in deps would cause recreation on every page change, we Intentionally
    // only recreates when total pages changes - compare via safeTotalPages
  }, [containerRef, onPageChange, safeTotalPages]);

  // Initial zoom: fit width
  // The source of truth for zoom lives in useZoomSync. This effect documents and ensures
  // the workspace is ready for fit width calculation: it measures container vs page and
  // if scale is still at the initial default (1), the first paint already looks fitted
  // because pages use max-width 100% and centered layout. Parent is expected to call
  // fitWidth() on mount; we re-calculate as safety for direct usage of DocumentWorkspace.
  useEffect(() => {
    const container = containerRef.current;
    if (!container) return;

    const computeFitWidth = () => {
      const available = container.clientWidth - FIT_HORIZONTAL_MARGIN;
      const ideal = available / PAGE_WIDTH;
      // Clamp similar to useZoomSync clamp
      return Math.min(Math.max(ideal, 0.5), 2.5);
    };

    // Store fit value as CSS variable for potential use, and ensure container can scroll smoothly
    const ro = new ResizeObserver(() => {
      const fit = computeFitWidth();
      container.style.setProperty(
        "--initial-fit-scale",
        String(fit.toFixed(3)),
      );
    });
    ro.observe(container);

    // Set initial variable synchronously
    container.style.setProperty(
      "--initial-fit-scale",
      String(computeFitWidth().toFixed(3)),
    );

    return () => ro.disconnect();
  }, [containerRef]);

  // Text selection handling for highlight workflow
  const handleMouseDown = useCallback(() => {
    // Progressive disclosure: hide stale action when starting new selection
    if (selectionTooltip.visible) {
      setSelectionTooltip((s) => ({ ...s, visible: false }));
    }
  }, [selectionTooltip.visible]);

  const handleMouseUp = useCallback(
    (e: React.MouseEvent) => {
      // Only care when highlight tool is active (tempTool highlight) or in comment mode with highlight expectation
      const isHighlightContext =
        tempTool === "highlight" ||
        (mode === "comment" && tempTool === "highlight");

      const sel = window.getSelection();
      if (!sel || sel.isCollapsed) return;

      const selectedText = sel.toString().trim();
      if (!selectedText || selectedText.length < 2) return;

      // Must be inside workspace
      const anchorNode = sel.anchorNode;
      const container = containerRef.current;
      if (!container || !anchorNode) return;

      const anchorEl =
        anchorNode instanceof Element ? anchorNode : anchorNode.parentElement;
      if (!anchorEl || !container.contains(anchorEl)) return;

      // Find which page selection is on
      let pageIndex = page;
      for (const [pNum, el] of pageRefs.current) {
        if (el.contains(anchorEl) || el === anchorEl) {
          pageIndex = pNum;
          break;
        }
      }

      // Only show contextual action if highlight tool is armed (progressive disclosure)
      if (!isHighlightContext) {
        // Still keep native selection available, but don't show highlight tooltip
        return;
      }

      try {
        const range = sel.getRangeAt(0);
        const rect = range.getBoundingClientRect();
        const containerRect = container.getBoundingClientRect();

        // Position tooltip above selection, centered, with viewport safety
        const tooltipWidth = 168; // approximate
        let x = rect.left + rect.width / 2 - tooltipWidth / 2;
        let y = rect.top - 44; // above

        // Clamp inside container horizontal bounds
        x = Math.max(
          containerRect.left + 12,
          Math.min(x, containerRect.right - tooltipWidth - 12),
        );
        // If selection near top, show below
        if (y < containerRect.top + 12) {
          y = rect.bottom + 12;
        }

        setSelectionTooltip({
          visible: true,
          x: x - containerRect.left + container.scrollLeft,
          y: y - containerRect.top + container.scrollTop,
          text: selectedText.slice(0, 320), // limit
          pageIndex,
        });
      } catch {
        // Range errors ignored (complex selections across pages)
        return;
      }
    },
    [tempTool, mode, page, containerRef],
  );

  const clearSelection = useCallback(() => {
    try {
      window.getSelection()?.removeAllRanges();
    } catch {}
    setSelectionTooltip((s) => ({ ...s, visible: false }));
  }, []);

  const handleHighlightConfirm = useCallback(() => {
    const sel = window.getSelection();
    if (!sel || sel.rangeCount === 0) {
      setSelectionTooltip((s) => ({ ...s, visible: false }));
      return;
    }

    try {
      const range = sel.getRangeAt(0);
      // Avoid highlight that splits across multiple block elements - use extract + mark fallback
      const mark = document.createElement("mark");
      mark.className = "pdf-elite-highlight";
      mark.dataset.page = String(selectionTooltip.pageIndex);
      mark.style.backgroundColor = highlightColor;
      mark.style.color = "inherit";
      mark.style.borderRadius = "2px";
      mark.style.padding = "0.5px 1px";
      mark.style.boxShadow = `0 0 0 1px ${highlightColor}`;

      // surroundContents can throw if range partially selects; fallback to wrap
      try {
        range.surroundContents(mark);
      } catch {
        const frag = range.extractContents();
        mark.appendChild(frag);
        range.insertNode(mark);
      }

      if (onHighlight) {
        onHighlight({
          text: selectionTooltip.text,
          page: selectionTooltip.pageIndex,
          color: highlightColor,
        });
      }
    } catch {
      // Silently ignore DOM mutation errors - selection still counts
      if (onHighlight) {
        onHighlight({
          text: selectionTooltip.text,
          page: selectionTooltip.pageIndex,
          color: highlightColor,
        });
      }
    } finally {
      clearSelection();
    }
  }, [selectionTooltip, highlightColor, onHighlight, clearSelection]);

  // Dismiss tooltip on scroll or Escape
  useEffect(() => {
    const root = containerRef.current;
    if (!root) return;

    const onScroll = () => {
      // Hide during scroll to avoid misplaced floating action (professional minimalism)
      if (selectionTooltip.visible) {
        setSelectionTooltip((s) => ({ ...s, visible: false }));
      }
    };

    const onKey = (ev: KeyboardEvent) => {
      if (ev.key === "Escape") {
        setSelectionTooltip((s) => ({ ...s, visible: false }));
      }
    };

    root.addEventListener("scroll", onScroll, { passive: true });
    window.addEventListener("keydown", onKey);
    return () => {
      root.removeEventListener("scroll", onScroll);
      window.removeEventListener("keydown", onKey);
    };
  }, [containerRef, selectionTooltip.visible]);

  const pageNumbers = useMemo(
    () => Array.from({ length: safeTotalPages }, (_, i) => i + 1),
    [safeTotalPages],
  );

  return (
    <div
      ref={containerRef}
      className="doc-workspace"
      data-mode={mode}
      data-temp-tool={tempTool || "none"}
      onMouseDown={handleMouseDown}
      onMouseUp={handleMouseUp}
      role="region"
      aria-label="PDF document workspace"
      tabIndex={-1}
    >
      {/* Scalable layer - zoom via CSS transform, centered */}
      <div
        ref={pagesWrapperRef}
        className="pages-scaler"
        style={{
          transform: `scale(${scale})`,
          transformOrigin: "top center",
          // Width compensation so scaled content doesn't collapse: keep intrinsic page width
          willChange: "transform",
        }}
      >
        <div className="pages-stack">
          {pageNumbers.map((pNum) => (
            <div
              key={pNum}
              data-page-number={pNum}
              ref={(el) => setPageRef(pNum, el)}
              className={`pdf-page ${pNum === page ? "is-active" : ""}`}
              role="group"
              aria-label={`Page ${pNum} of ${safeTotalPages}`}
            >
              {pNum === 1 ? (
                <PageOne />
              ) : pNum === 2 ? (
                <PageTwo />
              ) : (
                <PageContinuation pageNumber={pNum} />
              )}
              <div className="page-number-badge">{pNum}</div>
            </div>
          ))}
        </div>
      </div>

      {/* Contextual highlight action - progressive disclosure only when selection + highlight tool */}
      {selectionTooltip.visible && tempTool === "highlight" && (
        <div
          className="highlight-action"
          role="toolbar"
          aria-label="Highlight actions"
          style={{
            left: `${selectionTooltip.x}px`,
            top: `${selectionTooltip.y}px`,
          }}
          onMouseDown={(e) => e.preventDefault()} // prevent losing selection when clicking toolbar
        >
          <button
            className="highlight-action-btn primary"
            onClick={handleHighlightConfirm}
            title="Apply highlight"
          >
            <Highlighter size={14} />
            <span>Highlight</span>
          </button>
          <button
            className="highlight-action-btn ghost"
            onClick={clearSelection}
            aria-label="Dismiss"
          >
            <X size={14} />
          </button>
          <span
            className="highlight-color-preview"
            style={{ background: highlightColor }}
            aria-hidden
          />
        </div>
      )}

      <style>{`
        .doc-workspace {
          position: relative;
          flex: 1 1 auto;
          min-width: 0;
          min-height: 0;
          width: 100%;
          height: 100%;
          overflow: auto;
          background: var(--workspace-paper-bg);
          background-image: radial-gradient(rgba(255,255,255,0.02) 1px, transparent 1px);
          background-size: 24px 24px;
          scroll-behavior: smooth;
          -webkit-overflow-scrolling: touch;
          overscroll-behavior: contain;
          display: flex;
          justify-content: center;
          align-items: flex-start;
          padding: 32px 32px 96px;
          /* Professional focus ring handled by containerRef owner */
        }

        .doc-workspace:focus-visible {
          outline: 2px solid var(--focus-ring);
          outline-offset: -2px;
        }

        /* Keep scrollbar subtle - tokens from design system */
        .doc-workspace::-webkit-scrollbar {
          width: 10px;
          height: 10px;
        }
        .doc-workspace::-webkit-scrollbar-thumb {
          background: var(--scrollbar-thumb);
          border-radius: var(--radius-full);
          border: 2px solid var(--workspace-paper-bg);
        }
        .doc-workspace::-webkit-scrollbar-thumb:hover {
          background: var(--scrollbar-hover);
        }

        .pages-scaler {
          display: flex;
          justify-content: center;
          width: 100%;
          /* Prevent scaled content from affecting center calc via transform only */
          transition: transform var(--duration-slow) var(--ease-out);
        }

        @media (prefers-reduced-motion: reduce) {
          .pages-scaler,
          .doc-workspace {
            scroll-behavior: auto;
            transition-duration: 0ms !important;
          }
        }

        .pages-stack {
          display: flex;
          flex-direction: column;
          align-items: center;
          gap: var(--page-gap);
          width: 100%;
          max-width: ${PAGE_WIDTH}px;
          /* The stack itself is not scaled, only its parent */
        }

        .pdf-page {
          position: relative;
          width: 100%;
          max-width: ${PAGE_WIDTH}px;
          min-height: ${PAGE_HEIGHT}px;
          background: var(--page-bg);
          color: #111827;
          box-shadow: var(--page-shadow);
          border-radius: 2px;
          padding: ${PAGE_CONTENT_PADDING}px ${PAGE_CONTENT_PADDING}px 56px;
          text-align: left;
          line-height: 1.6;
          font-family: "Times New Roman", Times, Georgia, serif;
          font-size: 15px;
          transform: translateZ(0);
          transition: box-shadow var(--duration-normal) var(--ease-out);
          cursor: text;
          /* Selection should feel native */
          user-select: text;
        }

        .pdf-page.is-active {
          box-shadow: var(--page-shadow-hover, 0 8px 32px rgba(0,0,0,0.4)), 0 1px 3px rgba(0,0,0,0.2);
        }

        .pdf-page:hover {
          box-shadow: var(--page-shadow-hover, 0 8px 32px rgba(0,0,0,0.35));
        }

        /* Page content styling - professional document hierarchy */
        .pdf-page .assignment-label {
          font-family: var(--font-sans);
          font-weight: 800;
          font-size: 32px;
          letter-spacing: -0.02em;
          line-height: 1.1;
          color: #0a0a0a;
          margin: 0 0 28px 0;
        }

        .pdf-page .doc-title {
          font-family: var(--font-sans);
          font-weight: 800;
          font-size: 26px;
          line-height: 1.2;
          letter-spacing: -0.01em;
          color: #111;
          margin: 0 0 32px 0;
        }

        .pdf-page .section-heading {
          font-family: var(--font-sans);
          font-weight: 700;
          font-size: 22px;
          line-height: 1.3;
          color: #111;
          margin: 32px 0 14px;
        }

        .pdf-page .body-text {
          font-size: 17.5px;
          line-height: 1.65;
          color: #1d1d1f;
          margin: 0 0 14px;
          font-family: "Spectral", "Times New Roman", Georgia, serif;
        }

        .pdf-page .divider {
          height: 2px;
          background: #c5c5c5;
          margin: 28px 0 32px;
          border: none;
        }

        .pdf-page .meta-line {
          font-family: var(--font-sans);
          font-size: 11px;
          letter-spacing: 0.04em;
          text-transform: uppercase;
          color: #6b7280;
          margin-bottom: 18px;
        }

        .pdf-page .highlight-mock {
          background: #fef08a;
          padding: 1px 2px;
          border-radius: 2px;
        }

        .page-number-badge {
          position: absolute;
          bottom: 12px;
          right: 20px;
          font-family: var(--font-sans);
          font-size: 10px;
          font-weight: 500;
          letter-spacing: 0.04em;
          color: #9ca3af;
          font-variant-numeric: tabular-nums;
          user-select: none;
          pointer-events: none;
        }

        /* Inline highlight style injected */
        .pdf-page ::selection {
          background: var(--accent-strong);
        }

        .pdf-page .pdf-elite-highlight {
          background: #fef08a;
          border-radius: 2px;
          box-decoration-break: clone;
          -webkit-box-decoration-break: clone;
        }

        /* Contextual highlight toolbar */
        .highlight-action {
          position: absolute;
          z-index: var(--z-tooltip);
          display: inline-flex;
          align-items: center;
          gap: 4px;
          background: var(--surface-elevated);
          border: 1px solid var(--border-strong);
          box-shadow: 0 8px 24px rgba(0,0,0,0.32), 0 2px 6px rgba(0,0,0,0.2);
          border-radius: var(--radius-md);
          padding: 4px;
          min-width: max-content;
          animation: highlightActionIn var(--duration-fast) var(--ease-out);
          pointer-events: auto;
        }

        @keyframes highlightActionIn {
          from { opacity: 0; transform: translateY(4px) scale(0.98); }
          to { opacity: 1; transform: translateY(0) scale(1); }
        }

        @media (prefers-reduced-motion: reduce) {
          .highlight-action {
            animation: none;
          }
        }

        .highlight-action-btn {
          display: inline-flex;
          align-items: center;
          gap: 6px;
          height: 28px;
          padding: 0 10px;
          border-radius: var(--radius-sm);
          border: 1px solid transparent;
          font-family: var(--font-sans);
          font-size: 12px;
          font-weight: 600;
          line-height: 1;
          cursor: pointer;
          transition: all var(--duration-fast) var(--ease-out);
          white-space: nowrap;
        }

        .highlight-action-btn.primary {
          background: var(--accent);
          color: var(--text-inverse);
          border-color: var(--accent);
        }
        .highlight-action-btn.primary:hover {
          background: var(--accent-hover);
          border-color: var(--accent-hover);
        }
        .highlight-action-btn.primary:active {
          background: var(--accent-pressed);
        }

        .highlight-action-btn.ghost {
          background: transparent;
          color: var(--text-secondary);
          width: 28px;
          padding: 0;
          justify-content: center;
        }
        .highlight-action-btn.ghost:hover {
          background: var(--surface-hover);
          color: var(--text-primary);
        }

        .highlight-color-preview {
          width: 12px;
          height: 12px;
          border-radius: 50%;
          border: 1px solid rgba(0,0,0,0.12);
          margin-left: 2px;
        }

        /* Document workspace dominates - large empty state not needed,
           but handle very narrow containers gracefully */
        @media (max-width: 960px) {
          .doc-workspace {
            padding: 16px 16px 64px;
          }
          .pdf-page {
            padding: 36px 28px 48px;
            min-height: auto;
          }
          .pdf-page .assignment-label {
            font-size: 26px;
          }
          .pdf-page .doc-title {
            font-size: 20px;
          }
        }
      `}</style>
    </div>
  );
};

/* --- Mock PDF Page Content: Assignment 03 as styled divs --- */

const PageOne: React.FC = () => {
  return (
    <div className="page-content">
      <div className="meta-line">CE 431 · Structural Modeling · Fall 2024</div>
      <h1 className="assignment-label">Assignment 03</h1>
      <h2 className="doc-title">
        Modeling and Verification of a Multi-Story Reinforced Concrete Building
        Using ETABS
      </h2>

      <h3 className="section-heading">Objectives</h3>
      <p className="body-text">
        The objective of this assignment is to develop a complete
        three-dimensional analytical model of a multi-story reinforced concrete
        building using ETABS. Students will define project settings, material
        properties, frame and shell sections, structural sections, and
        structural elements, verify the analytical model, and prepare it for
        seismic and gravity analysis in the subsequent chapters.
      </p>

      <hr className="divider" />

      <h3 className="section-heading">Project Description</h3>
      <p className="body-text">
        The building considered for this assignment is a 6-story reinforced
        concrete moment frame structure located in Seismic Zone III. The floor
        plan is rectangular 22.5 m × 16.0 m with a typical story height of 3.2 m
        and a ground story height of 4.0 m. All columns are 400 mm × 500 mm,
        beams are 300 mm × 550 mm, and the slab is 150 mm thick. The structure
        uses M30 concrete and Fe500 reinforcement.
      </p>
      <p className="body-text">
        Students are required to model the structure with fixed base supports,
        rigid diaphragms at each floor level, and appropriate load patterns (DL,
        LL, EQX, EQY). The model must be checked for geometry, connectivity, and
        stability before proceeding to analysis.
      </p>

      <h3 className="section-heading">Deliverables</h3>
      <ul className="body-list">
        <li>ETABS model file (.edb) with all definitions</li>
        <li>Screenshots of material, section, and load definitions</li>
        <li>3D view and plan verification</li>
        <li>Model check report</li>
      </ul>

      <style>{`
        .body-list {
          margin: 8px 0 0 20px;
          padding: 0;
          font-size: 16px;
          line-height: 1.7;
          color: #1d1d1f;
        }
        .body-list li {
          margin-bottom: 4px;
        }
      `}</style>
    </div>
  );
};

const PageTwo: React.FC = () => {
  return (
    <div className="page-content">
      <h3 className="section-heading">Workflow Overview</h3>
      <p className="body-text">
        1. Initialize new model with Indian codes IS 456:2000 and IS 1893:2016.
        Set units to kN, m, C.
      </p>
      <p className="body-text">
        2. Define grid system: 4 bays @ 5.0 m + 1 bay @ 2.5 m in X direction
        (22.5 m) and 3 bays @ 5.0 m + 1 bay @ 1.0 m in Y. Include construction
        grid for lift core.
      </p>
      <p className="body-text">
        3. Define materials M30 (E = 27386 MPa) and Fe500 (Fy = 500 MPa). Note
        to check weight and mass source for seismic mass.
      </p>

      <h3 className="section-heading">Modeling Checks</h3>
      <p className="body-text">
        Use <span className="highlight-mock">Analyze → Check Model</span> to
        verify overlapping objects, unsupported joints, and unmeshed walls. The
        stability of the structure should be confirmed by checking for isolated
        joints and mechanisms. All floor diaphragms must be assigned and
        verified in the deformed shape viewer.
      </p>

      <hr className="divider" />

      <h3 className="section-heading">Load Patterns and Cases</h3>
      <p className="body-text">
        Define dead load (self-weight multiplier = 1), super dead (1.0 kN/m²
        floor finish, 1.5 kN/m² wall on beams), live load 3.0 kN/m² (typical)
        and 1.5 kN/m² (roof). Seismic definitions as per IS 1893 with Z=0.16,
        I=1.0, R=5.0, soil type II.
      </p>

      <div className="callout">
        <strong>Important:</strong> Do not run analysis in this assignment. This
        chapter limits to geometric and definition verification only.
      </div>

      <style>{`
        .callout {
          margin-top: 20px;
          padding: 12px 14px;
          background: #f8fafc;
          border: 1px solid #e2e8f0;
          border-left: 3px solid var(--accent, #79AEFF);
          border-radius: 4px;
          font-family: var(--font-sans);
          font-size: 13px;
          line-height: 1.5;
          color: #334155;
        }
      `}</style>
    </div>
  );
};

const PageContinuation: React.FC<{ pageNumber: number }> = ({ pageNumber }) => {
  return (
    <div className="page-content">
      <h3 className="section-heading">
        Appendix {pageNumber - 2}: Supplementary Details
      </h3>
      <p className="body-text">
        This page is a mock continuation to demonstrate multi-page scroll, gap
        handling with <code>var(--page-gap)</code>, and page shadow{" "}
        <code>var(--page-shadow)</code> stacking under zoom transform. The
        workspace uses <code>var(--workspace-paper-bg)</code> as paper
        background to keep document dominant.
      </p>
      <p className="body-text">
        Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur
        lacinia, nunc ut dictum tincidunt, dolor massa tincidunt lacus, ac
        feugiat ligula arcu non lorem. Fusce feugiat nibh sit amet orci blandit,
        sed faucibus tortor consequat. Pellentesque habitant morbi tristique
        senectus et netus et malesuada fames ac turpis egestas.
      </p>
      <p className="body-text">
        Reinforced concrete sections: beams require effective flange width
        consideration, cracked section modifiers per IS 16700:2017 – 0.35Ig for
        beams, 0.70Ig for columns. Ensure end offsets and insertion points are
        verified in ETABS for frame action.
      </p>
      <p className="body-text">
        Page {pageNumber} intentionally contains selectable text to test the
        highlight workflow: when tempTool is
        <strong> highlight</strong>, selecting text shows a contextual action to
        apply highlight. The action is progressively disclosed and does not
        duplicate toolbar controls.
      </p>
    </div>
  );
};

export default DocumentWorkspace;
