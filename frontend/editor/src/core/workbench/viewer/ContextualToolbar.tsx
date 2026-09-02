/* eslint-disable no-restricted-syntax */
import React from "react";
import {
  ZoomIn,
  ZoomOut,
  Maximize2,
  Maximize,
  Scan,
  RotateCw,
  Highlighter,
  StickyNote,
  PenTool,
  Type,
  Underline,
  Strikethrough,
  Search,
  X,
  Contrast,
  Columns2,
  Expand,
  Shrink,
  Image as ImageIcon,
  RefreshCw,
  Trash2,
} from "lucide-react";
import type { ToolMode, TempTool } from "@app/hooks/useToolLifecycle";
import { useViewerZoom } from "@app/contexts/ViewerContext";
import { useAnnotation } from "@app/contexts/AnnotationContext";

type Props = {
  mode: ToolMode;
  tempTool: TempTool;
  onTempTool: (t: TempTool) => void;
  // Rotation
  onRotateRight?: () => void;
  // View Modes
  onCycleViewMode?: () => void;
  // Page layout (single vs facing pages) — Phase 34
  isDualPage?: boolean;
  onTogglePageLayout?: () => void;
  // Fullscreen — Phase 34
  isFullscreen?: boolean;
  onToggleFullscreen?: () => void;
  // Highlight
  highlightColor: string;
  highlightOpacity: number;
  highlightColors: { id: string; hex: string; name: string }[];
  onHighlightColor: (hex: string) => void;
  onHighlightOpacity: (opacity: number) => void;
  // Search
  searchQuery: string;
  searchCount: { current: number; total: number };
  onSearchChange: (q: string) => void;
  onSearchNext: () => void;
  onSearchPrev: () => void;
  onCloseSearch: () => void;
  onToolSelect?: (toolId: string) => void;
};

export const ContextualToolbar: React.FC<Props> = ({
  mode,
  tempTool,
  onTempTool,
  onRotateRight,
  onCycleViewMode,
  isDualPage,
  onTogglePageLayout,
  isFullscreen,
  onToggleFullscreen,
  highlightColor,
  highlightOpacity,
  highlightColors,
  onHighlightColor,
  onHighlightOpacity,
  searchQuery,
  searchCount,
  onSearchChange,
  onSearchNext,
  onSearchPrev,
  onCloseSearch,
  onToolSelect,
}) => {
  const { zoom, zoomActions } = useViewerZoom();
  const { annotationApiRef } = useAnnotation() || {};
  const imageInputRef = React.useRef<HTMLInputElement>(null);

  const handleAddImage = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file && annotationApiRef?.current) {
      const reader = new FileReader();
      reader.onload = (event) => {
        const imageSrc = event.target?.result as string;
        annotationApiRef.current?.activateAnnotationTool("stamp", { imageSrc });
        // After activating stamp with the image, switch to the annotate mode temporarily?
        // Actually, activating the tool directly inside embedpdf works regardless of our top-level state, 
        // but we might want to ensure the temporary mode is set so Escape works.
        onTempTool("stamp" as any);
      };
      reader.readAsDataURL(file);
    }
    // Reset input
    if (imageInputRef.current) {
      imageInputRef.current.value = "";
    }
  };

  const renderViewTools = () => (
    <>
      <div className="tb-group">
        <button
          className="tb-btn"
          onClick={zoomActions.zoomOut}
          title="Zoom out (Ctrl+-)"
        >
          <ZoomOut size={20} />
        </button>
        <div className="zoom-display">
          {/*
            Slider operates in PERCENT units (25–500). The parent handler
            (onZoomSlider) divides by 100 to get the scale factor that
            setZoomLevel expects, matching every other zoom call site.
            value is bound to the live zoom.percentage so the slider never
            goes stale regardless of which source changed the zoom.
          */}
          <input
            type="range"
            min="25"
            max="500"
            step="5"
            value={Math.round(zoom.percentage)}
            onChange={(e) =>
              zoomActions.setZoomLevel(parseFloat(e.target.value) / 100)
            }
            className="zoom-slider"
          />
          <span className="zoom-pct">{Math.round(zoom.percentage)}%</span>
        </div>
        <button
          className="tb-btn"
          onClick={zoomActions.zoomIn}
          title="Zoom in (Ctrl++)"
        >
          <ZoomIn size={20} />
        </button>
      </div>
      <div className="tb-sep" />
      <div className="tb-group">
        <button
          className="tb-btn"
          onClick={() => zoomActions.requestZoom("fit-width")}
          title="Fit width"
        >
          <Maximize2 size={20} />
        </button>
        <button
          className="tb-btn"
          onClick={() => zoomActions.requestZoom("fit-page")}
          title="Fit page"
        >
          <Maximize size={20} />
        </button>
        <button
          className="tb-btn"
          onClick={() => zoomActions.requestZoom("actual-size")}
          title="Actual size"
        >
          <Scan size={20} />
        </button>
      </div>
      <div className="tb-sep" />
      <div className="tb-group">
        <button className="tb-btn" onClick={onRotateRight} title="Rotate">
          <RotateCw size={20} />
        </button>
        {onTogglePageLayout && (
          <button
            className={`tb-btn ${isDualPage ? "active" : ""}`}
            onClick={onTogglePageLayout}
            title={
              isDualPage
                ? "Facing pages — switch to single page"
                : "Single page — switch to facing pages"
            }
          >
            <Columns2 size={20} />
          </button>
        )}
        <button
          className="tb-btn"
          onClick={onCycleViewMode}
          title="Reading color (normal / dark / sepia)"
        >
          <Contrast size={20} />
        </button>
      </div>
      {onToggleFullscreen && (
        <>
          <div className="tb-sep" />
          <div className="tb-group">
            <button
              className={`tb-btn ${isFullscreen ? "active" : ""}`}
              onClick={onToggleFullscreen}
              title={isFullscreen ? "Exit fullscreen" : "Enter fullscreen"}
            >
              {isFullscreen ? <Shrink size={20} /> : <Expand size={20} />}
            </button>
          </div>
        </>
      )}
    </>
  );

  const renderCommentTools = () => (
    <>
      <div className="tb-group">
        <button
          className={`tb-btn ${tempTool === "highlight" ? "active" : ""}`}
          onClick={() =>
            onTempTool(tempTool === "highlight" ? null : "highlight")
          }
          title="Highlight"
        >
          <Highlighter size={20} />
        </button>
        <button
          className={`tb-btn ${tempTool === "area-highlight" ? "active" : ""}`}
          onClick={() =>
            onTempTool(tempTool === "area-highlight" ? null : "area-highlight")
          }
          title="Area highlight"
        >
          <div className="area-icon" />
        </button>
        <button
          className={`tb-btn ${tempTool === "underline" ? "active" : ""}`}
          onClick={() =>
            onTempTool(tempTool === "underline" ? null : "underline")
          }
          title="Underline"
        >
          <Underline size={20} />
        </button>
        <button
          className={`tb-btn ${tempTool === "strikeout" ? "active" : ""}`}
          onClick={() =>
            onTempTool(tempTool === "strikeout" ? null : "strikeout")
          }
          title="Strikethrough"
        >
          <Strikethrough size={20} />
        </button>
        <div className="tb-sep" />
        <button
          className={`tb-btn ${tempTool === "note" ? "active" : ""}`}
          onClick={() => onTempTool(tempTool === "note" ? null : "note")}
          title="Sticky note"
        >
          <StickyNote size={20} />
        </button>
        <button
          className={`tb-btn ${tempTool === "text" ? "active" : ""}`}
          onClick={() => onTempTool(tempTool === "text" ? null : "text")}
          title="Text comment"
        >
          <Type size={20} />
        </button>
        <button
          className={`tb-btn ${tempTool === "draw" ? "active" : ""}`}
          onClick={() => onTempTool(tempTool === "draw" ? null : "draw")}
          title="Draw"
        >
          <PenTool size={20} />
        </button>
      </div>
      {tempTool === "highlight" && (
        <>
          <div className="tb-sep" />
          <div className="tb-group color-group">
            {highlightColors.map((c) => (
              <button
                key={c.id}
                className={`color-dot ${highlightColor === c.hex ? "active" : ""}`}
                style={{ background: c.hex }}
                onClick={() => onHighlightColor(c.hex)}
                title={c.name}
              />
            ))}
          </div>
        </>
      )}
      <div className="tb-sep" />
      <div className="tb-group">
        <span className="tb-hint">
          Select text then click Highlight. Color persists.
        </span>
      </div>
      {/*
        Quick zoom access while commenting. This is NOT a separate zoom system:
        it reads the same synced zoom.percentage and calls the same onZoomIn/
        onZoomOut handlers as the canonical View-mode group, so it can never
        drift out of sync.
      */}
      <div className="tb-group ml-auto">
        <button className="tb-btn" onClick={zoomActions.zoomOut}>
          <ZoomOut size={20} />
        </button>
        <span className="zoom-pct">{Math.round(zoom.percentage)}%</span>
        <button className="tb-btn" onClick={zoomActions.zoomIn}>
          <ZoomIn size={20} />
        </button>
      </div>
    </>
  );

  const renderEditTools = () => (
    <div className="tb-group">
      <button
        className={`tb-btn ${tempTool === "text" ? "active" : ""}`}
        title="Inline Text Edit"
        onClick={() => onTempTool(tempTool === "text" ? null : "text")}
      >
        <Type size={18} />
        <span style={{ fontSize: "12px", marginLeft: "6px", fontWeight: 500 }}>
          Add Text
        </span>
      </button>
      <input
        type="file"
        ref={imageInputRef}
        style={{ display: "none" }}
        accept="image/*"
        onChange={handleAddImage}
      />
      <button
        className={`tb-btn`}
        title="Add Image"
        onClick={() => imageInputRef.current?.click()}
      >
        <ImageIcon size={18} />
        <span style={{ fontSize: "12px", marginLeft: "6px", fontWeight: 500 }}>
          Add Image
        </span>
      </button>
      {/*
        Real image operations on existing document content. Direct in-canvas
        move/resize/rotate of embedded images is NOT supported by the renderer,
        so we deliberately do not fake those controls; instead we surface the
        real, registered image tools (Replace / Remove), which perform genuine
        document operations. Images added via "Add Image" become stamp
        annotations that CAN be moved/resized/deleted through the viewer's
        annotation selection layer.
      */}
      <button
        className={`tb-btn ${tempTool === "replaceImage" ? "active" : ""}`}
        title="Replace Image"
        onClick={() => {
          if (tempTool === "replaceImage") onTempTool(null);
          else onTempTool("replaceImage" as any);
        }}
      >
        <RefreshCw size={18} />
        <span style={{ fontSize: "12px", marginLeft: "6px", fontWeight: 500 }}>
          Replace Image
        </span>
      </button>
      <button
        className="tb-btn"
        title="Remove Images"
        onClick={() => onToolSelect?.("removeImage")}
      >
        <Trash2 size={18} />
        <span style={{ fontSize: "12px", marginLeft: "6px", fontWeight: 500 }}>
          Remove Images
        </span>
      </button>
      <div className="tb-sep" />
      <button
        className={`tb-btn`}
        title="Edit PDF Content (Advanced)"
        onClick={() => onToolSelect?.("pdfTextEditor")}
      >
        <Type size={18} />
        <span style={{ fontSize: "12px", marginLeft: "6px", fontWeight: 500 }}>
          Advanced Edit
        </span>
      </button>
    </div>
  );

  const renderOrganizeTools = () => (
    <div className="tb-group">
      <span className="tb-hint">
        Organize mode is active. Select pages in the viewer and use the toolbar
        below to rotate, duplicate, extract, or delete them.
      </span>
    </div>
  );

  const renderSearch = () => (
    <div className="search-toolbar">
      <div className="search-input-wrap">
        <Search size={16} />
        <input
          autoFocus
          placeholder="Find in document"
          value={searchQuery}
          onChange={(e) => onSearchChange(e.target.value)}
          onKeyDown={(e) => {
            if (e.key === "Enter") {
              if (e.shiftKey) onSearchPrev();
              else onSearchNext();
            }
            if (e.key === "Escape") onCloseSearch();
          }}
        />
        <span className="search-count">
          {searchCount.total > 0
            ? `${searchCount.current}/${searchCount.total}`
            : "0 results"}
        </span>
      </div>
      <div className="tb-group">
        <button
          className="tb-btn"
          onClick={onSearchPrev}
          title="Previous (Shift+Enter)"
        >
          ↑
        </button>
        <button className="tb-btn" onClick={onSearchNext} title="Next (Enter)">
          ↓
        </button>
        <button className="tb-btn" onClick={onCloseSearch} title="Close (Esc)">
          <X size={20} />
        </button>
      </div>
    </div>
  );

  if (mode === "search")
    return <div className="contextual-toolbar">{renderSearch()}</div>;

  return (
    <div className="contextual-toolbar">
      <div className="tb-left">
        {mode === "view" && renderViewTools()}
        {mode === "comment" && renderCommentTools()}
        {mode === "edit" && renderEditTools()}
        {mode === "organize" && renderOrganizeTools()}
      </div>
      <style>{`
        .contextual-toolbar {
          height: var(--toolbar-height);
          background: var(--toolbar-bg);
          border-bottom: 1px solid var(--toolbar-border);
          display: flex;
          align-items: center;
          padding: 0 24px;
          gap: 16px;
          flex-shrink: 0;
        }
        .tb-left {
          display: flex;
          align-items: center;
          gap: 8px;
          flex: 1;
          min-width: 0;
        }
        .tb-group {
          display: flex;
          align-items: center;
          gap: 4px;
        }
        .tb-group.ml-auto {
          margin-left: auto;
        }
        .tb-sep {
          width: 1px;
          height: 24px;
          background: var(--border);
          margin: 0 12px;
          flex-shrink: 0;
        }
        .tb-btn {
          height: 36px;
          min-width: 36px;
          padding: 0 10px;
          border-radius: 8px;
          border: none;
          background: transparent;
          color: var(--text-secondary);
          display: flex;
          align-items: center;
          justify-content: center;
          gap: 6px;
          font-size: 13px;
          font-weight: 500;
          cursor: pointer;
          transition: all var(--duration-fast) var(--ease-out);
          white-space: nowrap;
        }
        .tb-btn:hover {
          background: var(--surface-hover);
          color: var(--text-primary);
        }
        .tb-btn.active {
          background: var(--accent-subtle);
          color: var(--accent);
          border: 1px solid var(--accent-strong);
        }
        .zoom-display {
          display: flex;
          align-items: center;
          gap: 8px;
          padding: 0 4px;
        }
        .zoom-slider {
          width: 100px;
          height: 4px;
          accent-color: var(--accent);
          cursor: pointer;
        }
        .zoom-pct {
          font-size: 12px;
          font-variant-numeric: tabular-nums;
          color: var(--text-secondary);
          min-width: 44px;
          text-align: center;
        }
        .color-group {
          gap: 6px;
          padding: 0 4px;
        }
        .color-dot {
          width: 22px;
          height: 22px;
          border-radius: 50%;
          border: 2px solid transparent;
          cursor: pointer;
          transition: all var(--duration-fast) var(--ease-out);
        }
        .color-dot:hover {
          transform: scale(1.1);
        }
        .color-dot.active {
          border-color: var(--text-primary);
          box-shadow: 0 0 0 2px var(--app-bg);
        }
        .area-icon {
          width: 16px;
          height: 12px;
          border: 1.5px dashed currentColor;
          background: rgba(121,174,255,0.2);
          border-radius: 2px;
        }
        .tb-hint {
          font-size: 11px;
          color: var(--text-tertiary);
          white-space: nowrap;
        }
        .search-toolbar {
          display: flex;
          align-items: center;
          gap: 12px;
          width: 100%;
        }
        .search-input-wrap {
          display: flex;
          align-items: center;
          gap: 8px;
          background: var(--surface-card);
          border: 1px solid var(--border-strong);
          border-radius: 8px;
          padding: 6px 10px;
          flex: 1;
          max-width: 480px;
        }
        .search-input-wrap input {
          flex: 1;
          background: transparent;
          border: none;
          outline: none;
          color: var(--text-primary);
          font-size: 13px;
        }
        .search-count {
          font-size: 11px;
          color: var(--text-tertiary);
          font-variant-numeric: tabular-nums;
        }
      `}</style>
    </div>
  );
};
