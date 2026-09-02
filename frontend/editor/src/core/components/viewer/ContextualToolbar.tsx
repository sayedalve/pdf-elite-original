/* eslint-disable */
import React from "react";
import {
  ZoomIn,
  ZoomOut,
  Maximize2,
  Maximize,
  Scan,
  RotateCw,
  FileSearch,
  Highlighter,
  StickyNote,
  PenTool,
  Type,
  Image as ImageIcon,
  Link2,
  Trash2,
  Copy,
  Scissors,
  Undo2,
  Redo2,
  Grid3x3,
  FileStack,
  ArrowLeftRight,
  Split,
  Search,
  X,
} from "lucide-react";
import { OpacityControl } from "@app/components/annotation/shared/OpacityControl";

type ToolMode = string;
type TempTool = string | null;

type Props = {
  mode: ToolMode;
  tempTool: TempTool;
  onModeChange: (m: ToolMode) => void;
  onTempTool: (t: TempTool) => void;
  // Zoom props - single canonical control passed from parent
  zoom: { percentage: number; scale: number };
  onZoomIn: () => void;
  onZoomOut: () => void;
  onFitWidth: () => void;
  onFitPage: () => void;
  onActualSize: () => void;
  onZoomSlider: (scale: number) => void;
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
};

export const ContextualToolbar: React.FC<Props> = ({
  mode,
  tempTool,
  onModeChange,
  onTempTool,
  zoom,
  onZoomIn,
  onZoomOut,
  onFitWidth,
  onFitPage,
  onActualSize,
  onZoomSlider,
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
}) => {
  const renderViewTools = () => (
    <>
      <div className="tb-group">
        <button
          className="tb-btn"
          onClick={onZoomOut}
          title="Zoom out (Ctrl+-)"
          aria-label="Zoom out (Ctrl+-)"
        >
          <ZoomOut size={18} />
        </button>
        <div className="zoom-display">
          <input
            type="range"
            min="0.25"
            max="5"
            step="any"
            value={zoom.scale}
            onChange={(e) => onZoomSlider(parseFloat(e.target.value))}
            className="zoom-slider"
          />
          <span className="zoom-pct">{zoom.percentage}%</span>
        </div>
        <button
          className="tb-btn"
          onClick={onZoomIn}
          title="Zoom in (Ctrl++)"
          aria-label="Zoom in (Ctrl++)"
        >
          <ZoomIn size={18} />
        </button>
      </div>
      <div className="tb-sep" />
      <div className="tb-group">
        <button
          className="tb-btn"
          onClick={onFitWidth}
          title="Fit width"
          aria-label="Fit width"
        >
          <Maximize2 size={18} />
        </button>
        <button
          className="tb-btn"
          onClick={onFitPage}
          title="Fit page"
          aria-label="Fit page"
        >
          <Maximize size={18} />
        </button>
        <button
          className="tb-btn"
          onClick={onActualSize}
          title="Actual size (100%)"
          aria-label="Actual size (100%)"
        >
          <Scan size={18} />
        </button>
      </div>
      <div className="tb-sep" />
      <div className="tb-group">
        <button className="tb-btn" title="Rotate" aria-label="Rotate">
          <RotateCw size={18} />
        </button>
      </div>
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
          aria-label="Highlight"
        >
          <Highlighter size={18} />
        </button>
        <button
          className={`tb-btn ${tempTool === "area-highlight" ? "active" : ""}`}
          onClick={() =>
            onTempTool(tempTool === "area-highlight" ? null : "area-highlight")
          }
          title="Area highlight"
          aria-label="Area highlight"
        >
          <div className="area-icon" />
        </button>
        <button
          className={`tb-btn ${tempTool === "note" ? "active" : ""}`}
          onClick={() => onTempTool(tempTool === "note" ? null : "note")}
          title="Sticky note"
          aria-label="Sticky note"
        >
          <StickyNote size={18} />
        </button>
        <button
          className={`tb-btn ${tempTool === "draw" ? "active" : ""}`}
          onClick={() => onTempTool(tempTool === "draw" ? null : "draw")}
          title="Draw"
          aria-label="Draw"
        >
          <PenTool size={18} />
        </button>
      </div>
      {(tempTool === "highlight" || tempTool === "area-highlight") && (
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
                aria-label={c.name}
              />
            ))}
          </div>
          {tempTool === "area-highlight" && (
            <>
              <div className="tb-sep" />
              <div className="tb-group">
                <OpacityControl
                  value={highlightOpacity}
                  onChange={onHighlightOpacity}
                />
              </div>
            </>
          )}
        </>
      )}
      <div className="tb-sep" />
      <div className="tb-group">
        <span className="tb-hint">
          Select text then click Highlight. Color persists.
        </span>
      </div>
      <div className="tb-group ml-auto">
        <button className="tb-btn" onClick={onZoomOut}>
          <ZoomOut size={18} />
        </button>
        <span className="zoom-pct">{zoom.percentage}%</span>
        <button className="tb-btn" onClick={onZoomIn}>
          <ZoomIn size={18} />
        </button>
      </div>
    </>
  );

  const renderEditTools = () => (
    <>
      <div className="tb-group">
        <button className="tb-btn">
          <Undo2 size={18} />
        </button>
        <button className="tb-btn">
          <Redo2 size={18} />
        </button>
      </div>
      <div className="tb-sep" />
      <div className="tb-group">
        <button
          className={`tb-btn ${tempTool === "select" ? "active" : ""}`}
          onClick={() => onTempTool("select")}
        >
          <Type size={18} />
        </button>
        <button className="tb-btn">
          <ImageIcon size={18} />
        </button>
        <button className="tb-btn">
          <Link2 size={18} />
        </button>
      </div>
      <div className="tb-sep" />
      <div className="tb-group">
        <span className="tb-hint">
          Text editing stays in main workspace. Select text to edit.
        </span>
      </div>
    </>
  );

  const renderOrganizeTools = () => (
    <>
      <div className="tb-group">
        <button className="tb-btn">
          <Grid3x3 size={18} /> Thumbnails
        </button>
        <button className="tb-btn">
          <RotateCw size={18} /> Rotate
        </button>
        <button className="tb-btn">
          <Trash2 size={18} /> Delete
        </button>
        <button className="tb-btn">
          <Copy size={18} /> Duplicate
        </button>
      </div>
      <div className="tb-sep" />
      <div className="tb-group">
        <button className="tb-btn">
          <FileStack size={18} /> Extract
        </button>
        <button className="tb-btn">
          <ArrowLeftRight size={18} /> Replace
        </button>
        <button className="tb-btn">
          <Split size={18} /> Split
        </button>
      </div>
      <div className="tb-sep" />
      <div className="tb-group">
        <span className="tb-hint">
          Select pages below to organize. Drag to reorder.
        </span>
      </div>
    </>
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
          aria-label="Previous (Shift+Enter)"
        >
          ↑
        </button>
        <button
          className="tb-btn"
          onClick={onSearchNext}
          title="Next (Enter)"
          aria-label="Next (Enter)"
        >
          ↓
        </button>
        <button
          className="tb-btn"
          onClick={onCloseSearch}
          title="Close (Esc)"
          aria-label="Close (Esc)"
        >
          <X size={16} />
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
          padding: 0 12px;
          gap: 12px;
          flex-shrink: 0;
          overflow-x: auto;
        }
        .contextual-toolbar::-webkit-scrollbar {
          display: none;
        }
        .tb-left {
          display: flex;
          align-items: center;
          gap: 4px;
          flex: 1;
          min-width: 0;
        }
        .tb-group {
          display: flex;
          align-items: center;
          gap: 2px;
        }
        .tb-group.ml-auto {
          margin-left: auto;
        }
        .tb-sep {
          width: 1px;
          height: 20px;
          background: var(--border);
          margin: 0 8px;
          flex-shrink: 0;
        }
        .tb-btn {
          height: 32px;
          min-width: 32px;
          padding: 0 8px;
          border-radius: 8px;
          border: none;
          background: transparent;
          color: var(--text-secondary);
          display: flex;
          align-items: center;
          justify-content: center;
          gap: 6px;
          font-size: 12px;
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
