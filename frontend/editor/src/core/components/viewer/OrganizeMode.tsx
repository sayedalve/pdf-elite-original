/* eslint-disable */
import React, { useState, useRef, useCallback, useMemo } from "react";
import {
  Check,
  RotateCw,
  RotateCcw,
  Trash2,
  Copy,
  FileOutput,
  GripVertical,
  Layers,
  X,
} from "lucide-react";

export type OrganizeModeProps = {
  totalPages?: number;
  selectedPages?: number[];
  onSelect?: (pages: number[]) => void;
  onRotate?: (pages: number[], direction: "cw" | "ccw") => void;
  onDelete?: (pages: number[]) => void;
  onDuplicate?: (pages: number[]) => void;
  onExtract?: (pages: number[]) => void;
  onReorder?: (newOrder: number[]) => void;
  className?: string;
};

type PageItem = {
  id: number;
  rotation: number; // 0 | 90 | 180 | 270
};

const DEFAULT_TOTAL = 12;

export const OrganizeMode: React.FC<OrganizeModeProps> = ({
  totalPages = DEFAULT_TOTAL,
  selectedPages,
  onSelect,
  onRotate,
  onDelete,
  onDuplicate,
  onExtract,
  onReorder,
  className,
}) => {
  const isControlledSelection = selectedPages !== undefined;

  const [internalPages, setInternalPages] = useState<PageItem[]>(() =>
    Array.from({ length: totalPages }, (_, i) => ({
      id: i + 1,
      rotation: 0,
    })),
  );

  const [internalSelected, setInternalSelected] = useState<number[]>([]);
  const [draggedId, setDraggedId] = useState<number | null>(null);
  const [dragOverId, setDragOverId] = useState<number | null>(null);
  const anchorRef = useRef<number | null>(null);
  const [nextId, setNextId] = useState(totalPages + 1);

  // Sync when totalPages prop changes beyond initial (reset for demo)
  React.useEffect(() => {
    if (totalPages !== internalPages.length) {
      // Keep existing rotations for overlap, create new otherwise
      setInternalPages((prev) => {
        if (totalPages > prev.length) {
          const extra = Array.from(
            { length: totalPages - prev.length },
            (_, i) => ({
              id: prev.length + i + 1,
              rotation: 0,
            }),
          );
          return [...prev, ...extra];
        }
        return prev.slice(0, totalPages);
      });
      setNextId(totalPages + 1);
    }
  }, [totalPages, internalPages.length]);

  const effectiveSelected = isControlledSelection
    ? selectedPages!
    : internalSelected;

  const pages = internalPages;
  const selectedSet = useMemo(
    () => new Set(effectiveSelected),
    [effectiveSelected],
  );

  const commitSelection = useCallback(
    (next: number[]) => {
      const sorted = [...next].sort((a, b) => a - b);
      if (!isControlledSelection) {
        setInternalSelected(sorted);
      }
      onSelect?.(sorted);
    },
    [isControlledSelection, onSelect],
  );

  const handlePageClick = useCallback(
    (e: React.MouseEvent, pageId: number) => {
      const idx = pages.findIndex((p) => p.id === pageId);
      if (idx === -1) return;

      if (e.shiftKey && anchorRef.current !== null) {
        const anchorIdx = pages.findIndex((p) => p.id === anchorRef.current);
        if (anchorIdx !== -1) {
          const start = Math.min(anchorIdx, idx);
          const end = Math.max(anchorIdx, idx);
          const range = pages.slice(start, end + 1).map((p) => p.id);
          if (e.metaKey || e.ctrlKey) {
            // add range to existing
            const merged = Array.from(
              new Set([...effectiveSelected, ...range]),
            );
            commitSelection(merged);
          } else {
            commitSelection(range);
          }
          return;
        }
      }

      if (e.metaKey || e.ctrlKey) {
        // toggle
        if (selectedSet.has(pageId)) {
          commitSelection(effectiveSelected.filter((id) => id !== pageId));
        } else {
          commitSelection([...effectiveSelected, pageId]);
          anchorRef.current = pageId;
        }
        return;
      }

      // single select
      anchorRef.current = pageId;
      if (effectiveSelected.length === 1 && effectiveSelected[0] === pageId) {
        // if clicking already single selected, keep it (don't deselect on single click without modifier, but allow deselect via ctrl)
        return;
      }
      commitSelection([pageId]);
    },
    [pages, effectiveSelected, selectedSet, commitSelection],
  );

  const handleClearSelection = useCallback(() => {
    anchorRef.current = null;
    commitSelection([]);
  }, [commitSelection]);

  const handleSelectAll = useCallback(() => {
    commitSelection(pages.map((p) => p.id));
  }, [pages, commitSelection]);

  const handleRotate = useCallback(
    (direction: "cw" | "ccw") => {
      if (effectiveSelected.length === 0) return;
      const delta = direction === "cw" ? 90 : -90;
      setInternalPages((prev) =>
        prev.map((p) =>
          selectedSet.has(p.id)
            ? { ...p, rotation: (p.rotation + delta + 360) % 360 }
            : p,
        ),
      );
      onRotate?.(effectiveSelected, direction);
    },
    [effectiveSelected, selectedSet, onRotate],
  );

  const handleRotateSingle = useCallback(
    (e: React.MouseEvent, pageId: number, direction: "cw" | "ccw") => {
      e.stopPropagation();
      const delta = direction === "cw" ? 90 : -90;
      setInternalPages((prev) =>
        prev.map((p) =>
          p.id === pageId
            ? { ...p, rotation: (p.rotation + delta + 360) % 360 }
            : p,
        ),
      );
      onRotate?.([pageId], direction);
    },
    [onRotate],
  );

  const handleDelete = useCallback(() => {
    if (effectiveSelected.length === 0) return;
    setInternalPages((prev) => prev.filter((p) => !selectedSet.has(p.id)));
    onDelete?.(effectiveSelected);
    commitSelection([]);
  }, [effectiveSelected, selectedSet, onDelete, commitSelection]);

  const handleDuplicate = useCallback(() => {
    if (effectiveSelected.length === 0) return;
    // Duplicate preserves order of selected as they appear in pages
    const selectedInOrder = pages.filter((p) => selectedSet.has(p.id));
    const clones: PageItem[] = selectedInOrder.map((p) => ({
      id: nextId + selectedInOrder.indexOf(p),
      rotation: p.rotation,
    }));
    const newNextId = nextId + clones.length;

    // Insert clones right after last selected occurrence
    setInternalPages((prev) => {
      const lastSelectedIndex = prev.reduce(
        (last, p, i) => (selectedSet.has(p.id) ? i : last),
        -1,
      );
      const copy = [...prev];
      copy.splice(lastSelectedIndex + 1, 0, ...clones);
      return copy;
    });
    setNextId(newNextId);
    onDuplicate?.(effectiveSelected);
    // optionally select clones
    commitSelection(clones.map((c) => c.id));
  }, [
    effectiveSelected,
    selectedSet,
    pages,
    nextId,
    onDuplicate,
    commitSelection,
  ]);

  const handleExtract = useCallback(() => {
    if (effectiveSelected.length === 0) return;
    onExtract?.(effectiveSelected);
  }, [effectiveSelected, onExtract]);

  const handleDragStart = useCallback((e: React.DragEvent, pageId: number) => {
    setDraggedId(pageId);
    e.dataTransfer.effectAllowed = "move";
    e.dataTransfer.setData("text/plain", String(pageId));
    // subtle drag image opacity handling done via CSS class
  }, []);

  const handleDragOver = useCallback(
    (e: React.DragEvent, pageId: number) => {
      e.preventDefault();
      if (pageId !== draggedId) {
        setDragOverId(pageId);
      }
    },
    [draggedId],
  );

  const handleDragLeave = useCallback(() => {
    setDragOverId(null);
  }, []);

  const handleDrop = useCallback(
    (e: React.DragEvent, targetId: number) => {
      e.preventDefault();
      if (draggedId === null || draggedId === targetId) {
        setDraggedId(null);
        setDragOverId(null);
        return;
      }
      setInternalPages((prev) => {
        const fromIdx = prev.findIndex((p) => p.id === draggedId);
        const toIdx = prev.findIndex((p) => p.id === targetId);
        if (fromIdx === -1 || toIdx === -1) return prev;
        const copy = [...prev];
        const [moved] = copy.splice(fromIdx, 1);
        // If dragging forward, targetIdx shifts down by 1 after removal, insert before target if from < to, else before target
        const insertAt =
          fromIdx < toIdx ? toIdx - (fromIdx < toIdx ? 0 : 0) : toIdx;
        // Actually we want to insert before target when dragging, but handle drop position
        copy.splice(insertAt, 0, moved);
        onReorder?.(copy.map((p) => p.id));
        return copy;
      });
      setDraggedId(null);
      setDragOverId(null);
    },
    [draggedId, onReorder],
  );

  const handleDragEnd = useCallback(() => {
    setDraggedId(null);
    setDragOverId(null);
  }, []);

  const hasSelection = effectiveSelected.length > 0;
  const allSelected =
    effectiveSelected.length === pages.length && pages.length > 0;

  return (
    <div className={`organize-root ${className ?? ""}`}>
      {/* Header - document-first hierarchy */}
      <div className="organize-header">
        <div className="organize-header-top">
          <div className="organize-title-group">
            <div className="organize-title-icon">
              <Layers size={16} />
            </div>
            <div>
              <h2 className="organize-title">Organize Pages</h2>
              <p className="organize-subtitle">
                {pages.length} {pages.length === 1 ? "page" : "pages"}
                {hasSelection
                  ? ` • ${effectiveSelected.length} selected`
                  : " • Drag to reorder, click to select"}
              </p>
            </div>
          </div>

          <div className="organize-header-actions">
            {!hasSelection ? (
              <button
                className="org-ghost-btn"
                onClick={handleSelectAll}
                disabled={pages.length === 0}
              >
                Select all
              </button>
            ) : (
              <>
                <span className="selection-pill">
                  <Check size={12} />
                  {effectiveSelected.length}
                </span>
                <button
                  className="org-ghost-btn"
                  onClick={handleClearSelection}
                >
                  <X size={14} />
                  Clear
                </button>
                {!allSelected && (
                  <button className="org-ghost-btn" onClick={handleSelectAll}>
                    Select all
                  </button>
                )}
              </>
            )}
          </div>
        </div>

        {/* Action toolbar - progressive disclosure, only enabled when selection exists */}
        <div className="organize-toolbar">
          <div className="toolbar-group">
            <button
              className="toolbar-btn"
              onClick={() => handleRotate("ccw")}
              disabled={!hasSelection}
              title="Rotate left 90° (selected)"
            >
              <RotateCcw size={16} />
              <span className="btn-label">Rotate left</span>
            </button>
            <button
              className="toolbar-btn"
              onClick={() => handleRotate("cw")}
              disabled={!hasSelection}
              title="Rotate right 90° (selected)"
            >
              <RotateCw size={16} />
              <span className="btn-label">Rotate right</span>
            </button>
          </div>

          <div className="toolbar-divider" />

          <div className="toolbar-group">
            <button
              className="toolbar-btn"
              onClick={handleDuplicate}
              disabled={!hasSelection}
              title="Duplicate selected pages"
            >
              <Copy size={16} />
              <span className="btn-label">Duplicate</span>
            </button>
            <button
              className="toolbar-btn danger"
              onClick={handleDelete}
              disabled={!hasSelection}
              title="Delete selected pages"
            >
              <Trash2 size={16} />
              <span className="btn-label">Delete</span>
            </button>
          </div>

          <div className="toolbar-divider" />

          <div className="toolbar-group">
            <button
              className="toolbar-btn primary"
              onClick={handleExtract}
              disabled={!hasSelection}
              title="Extract selected pages to new PDF"
            >
              <FileOutput size={16} />
              <span className="btn-label">Extract</span>
            </button>
          </div>

          <div className="toolbar-hint">
            <span className="hint-dot" />
            <span>
              Ctrl/Cmd + click for multi-select • Shift + click for range • Drag
              to reorder
            </span>
          </div>
        </div>
      </div>

      {/* Grid */}
      <div className="organize-content">
        {pages.length === 0 ? (
          <div className="empty-state">
            <div className="empty-icon">
              <Layers size={24} />
            </div>
            <h3>No pages</h3>
            <p>
              All pages have been removed. Use duplicate or restore to continue.
            </p>
          </div>
        ) : (
          <div className="pages-grid">
            {pages.map((page) => {
              const isSelected = selectedSet.has(page.id);
              const isDragging = draggedId === page.id;
              const isDragOver = dragOverId === page.id;

              return (
                <div
                  key={page.id}
                  className={`page-card ${isSelected ? "selected" : ""} ${isDragging ? "dragging" : ""} ${
                    isDragOver ? "drag-over" : ""
                  }`}
                  onClick={(e) => handlePageClick(e, page.id)}
                  draggable
                  onDragStart={(e) => handleDragStart(e, page.id)}
                  onDragOver={(e) => handleDragOver(e, page.id)}
                  onDragLeave={handleDragLeave}
                  onDrop={(e) => handleDrop(e, page.id)}
                  onDragEnd={handleDragEnd}
                  tabIndex={0}
                  role="button"
                  aria-selected={isSelected}
                  aria-label={`Page ${page.id}${isSelected ? ", selected" : ""}`}
                  onKeyDown={(e) => {
                    if (e.key === "Enter" || e.key === " ") {
                      e.preventDefault();
                      // @ts-ignore - fake mouse event with no modifiers
                      handlePageClick(
                        e as unknown as React.MouseEvent,
                        page.id,
                      );
                    }
                  }}
                >
                  {/* Selection checkbox */}
                  <div className="selection-box">
                    <div className="check-inner">
                      {isSelected && <Check size={12} strokeWidth={3} />}
                    </div>
                  </div>

                  {/* Drag handle hint */}
                  <div className="drag-handle">
                    <GripVertical size={14} />
                  </div>

                  {/* Thumbnail */}
                  <div className="thumb-area">
                    <div
                      className="page-paper"
                      style={{ transform: `rotate(${page.rotation}deg)` }}
                      data-rotation={page.rotation}
                    >
                      {/* Thumbnail Image or Fallback Mock */}
                      {page.url ? (
                        <div
                          className="pdf-thumbnail-container"
                          style={{
                            width: "100%",
                            height: "100%",
                            display: "flex",
                            alignItems: "center",
                            justifyContent: "center",
                            background: "#fff",
                          }}
                        >
                          <img
                            src={page.url}
                            alt={`Page ${page.id}`}
                            style={{
                              maxWidth: "100%",
                              maxHeight: "100%",
                              objectFit: "contain",
                            }}
                            draggable={false}
                          />
                        </div>
                      ) : (
                        <>
                          <div className="paper-header">
                            <div className="line short" />
                          </div>
                          <div className="paper-body">
                            <div className="line" />
                            <div className="line" />
                            <div className="line medium" />
                            <div className="line" />
                            <div className="line short" />
                            {page.id % 3 === 0 && (
                              <div className="image-block" />
                            )}
                            <div className="line" />
                            <div className="line medium" />
                          </div>
                          <div className="paper-footer">
                            <div className="line tiny" />
                          </div>
                        </>
                      )}

                      {/* Rotation badge */}
                      {page.rotation !== 0 && (
                        <div className="rotation-badge">{page.rotation}°</div>
                      )}
                    </div>
                  </div>

                  {/* Footer */}
                  <div className="card-footer">
                    <span className="page-number">Page {page.id}</span>
                    <div className="card-quick-actions">
                      <button
                        className="quick-action"
                        onClick={(e) => handleRotateSingle(e, page.id, "ccw")}
                        title="Rotate left"
                      >
                        <RotateCcw size={12} />
                      </button>
                      <button
                        className="quick-action"
                        onClick={(e) => handleRotateSingle(e, page.id, "cw")}
                        title="Rotate right"
                      >
                        <RotateCw size={12} />
                      </button>
                    </div>
                  </div>
                </div>
              );
            })}
          </div>
        )}
      </div>

      <style>{`
        .organize-root {
          display: flex;
          flex-direction: column;
          height: 100%;
          width: 100%;
          background: var(--workspace-bg, var(--app-bg));
          overflow: hidden;
          font-family: var(--font-sans);
        }

        /* Header - professional hierarchy */
        .organize-header {
          background: var(--surface-elevated);
          border-bottom: 1px solid var(--border);
          padding: var(--space-lg) var(--space-xl);
          display: flex;
          flex-direction: column;
          gap: var(--space-lg);
          flex-shrink: 0;
        }

        .organize-header-top {
          display: flex;
          align-items: flex-start;
          justify-content: space-between;
          gap: var(--space-lg);
        }

        .organize-title-group {
          display: flex;
          align-items: flex-start;
          gap: var(--space-md);
          min-width: 0;
        }

        .organize-title-icon {
          width: 28px;
          height: 28px;
          border-radius: var(--radius-sm);
          background: var(--accent-subtle);
          color: var(--accent);
          display: flex;
          align-items: center;
          justify-content: center;
          flex-shrink: 0;
          margin-top: 2px;
          border: 1px solid var(--accent-strong);
        }

        .organize-title {
          margin: 0;
          font-size: var(--text-lg);
          font-weight: 600;
          line-height: var(--leading-tight);
          color: var(--text-primary);
          letter-spacing: -0.01em;
        }

        .organize-subtitle {
          margin: 4px 0 0 0;
          font-size: var(--text-sm);
          color: var(--text-secondary);
          line-height: var(--leading-normal);
        }

        .organize-header-actions {
          display: flex;
          align-items: center;
          gap: var(--space-sm);
          flex-shrink: 0;
        }

        .selection-pill {
          display: inline-flex;
          align-items: center;
          gap: 4px;
          padding: 4px 8px;
          border-radius: var(--radius-full);
          background: var(--accent);
          color: #0f172a;
          font-size: var(--text-xs);
          font-weight: 600;
          line-height: 1;
        }

        .org-ghost-btn {
          height: 28px;
          padding: 0 10px;
          border-radius: var(--radius-sm);
          border: 1px solid var(--border);
          background: var(--surface-card);
          color: var(--text-secondary);
          font-size: var(--text-sm);
          font-weight: 500;
          display: inline-flex;
          align-items: center;
          gap: 4px;
          cursor: pointer;
          transition: all var(--duration-fast) var(--ease-out);
        }

        .org-ghost-btn:hover {
          background: var(--surface-hover);
          color: var(--text-primary);
          border-color: var(--border-strong);
        }

        .org-ghost-btn:disabled {
          opacity: 0.5;
          cursor: default;
        }

        /* Toolbar - no duplicate controls, progressive disclosure */
        .organize-toolbar {
          display: flex;
          align-items: center;
          gap: var(--space-sm);
          flex-wrap: wrap;
          padding: 2px 0;
        }

        .toolbar-group {
          display: flex;
          align-items: center;
          gap: 2px;
          background: var(--surface-card);
          border: 1px solid var(--border);
          border-radius: var(--radius-md);
          padding: 2px;
        }

        .toolbar-divider {
          width: 1px;
          height: 20px;
          background: var(--border);
          margin: 0 2px;
          flex-shrink: 0;
        }

        .toolbar-btn {
          height: 32px;
          padding: 0 12px;
          border-radius: var(--radius-sm);
          border: none;
          background: transparent;
          color: var(--text-secondary);
          font-size: var(--text-sm);
          font-weight: 500;
          display: inline-flex;
          align-items: center;
          justify-content: center;
          gap: 6px;
          cursor: pointer;
          transition: all var(--duration-fast) var(--ease-out);
          white-space: nowrap;
        }

        .toolbar-btn:hover:not(:disabled) {
          background: var(--surface-hover);
          color: var(--text-primary);
        }

        .toolbar-btn:focus-visible {
          outline: 2px solid var(--accent);
          outline-offset: 1px;
        }

        .toolbar-btn:disabled {
          opacity: 0.38;
          cursor: default;
        }

        .toolbar-btn.danger:hover:not(:disabled) {
          background: rgba(255,107,107,0.12);
          color: var(--destructive);
        }

        .toolbar-btn.primary {
          background: var(--accent);
          color: #0f172a;
          font-weight: 600;
        }

        .toolbar-btn.primary:hover:not(:disabled) {
          background: var(--accent-hover);
          color: #0f172a;
        }

        .toolbar-btn.primary:disabled {
          background: var(--surface-hover);
          color: var(--text-disabled);
        }

        .btn-label {
          font-size: var(--text-sm);
        }

        .toolbar-hint {
          display: flex;
          align-items: center;
          gap: 8px;
          margin-left: auto;
          font-size: var(--text-xs);
          color: var(--text-tertiary);
          white-space: nowrap;
        }

        .hint-dot {
          width: 6px;
          height: 6px;
          border-radius: 50%;
          background: var(--text-tertiary);
          flex-shrink: 0;
        }

        /* Content */
        .organize-content {
          flex: 1;
          overflow-y: auto;
          overflow-x: hidden;
          padding: var(--space-xl);
          background: var(--workspace-bg);
        }

        .pages-grid {
          display: grid;
          grid-template-columns: repeat(auto-fill, minmax(172px, 1fr));
          gap: var(--space-lg);
          align-items: start;
          max-width: 1400px;
        }

        @media (max-width: 900px) {
          .pages-grid {
            grid-template-columns: repeat(auto-fill, minmax(148px, 1fr));
            gap: var(--space-md);
          }
        }

        @media (max-width: 640px) {
          .pages-grid {
            grid-template-columns: repeat(auto-fill, minmax(128px, 1fr));
          }
          .organize-header {
            padding: var(--space-md) var(--space-lg);
          }
          .toolbar-hint {
            display: none;
          }
        }

        /* Page Card - uses --surface-elevated */
        .page-card {
          position: relative;
          background: var(--surface-elevated);
          border: 1.5px solid var(--border);
          border-radius: var(--radius-lg);
          overflow: hidden;
          cursor: pointer;
          transition: border-color var(--duration-fast) var(--ease-out),
                      background var(--duration-fast) var(--ease-out),
                      box-shadow var(--duration-fast) var(--ease-out),
                      transform var(--duration-fast) var(--ease-out);
          user-select: none;
        }

        .page-card:hover {
          background: var(--surface-card);
          border-color: var(--border-strong);
          transform: translateY(-1px);
          box-shadow: 0 4px 16px rgba(0,0,0,0.22), 0 1px 3px rgba(0,0,0,0.12);
        }

        .page-card:focus-visible {
          outline: 2px solid var(--accent);
          outline-offset: 2px;
        }

        .page-card.selected {
          border-color: var(--accent);
          background: var(--surface-card);
          box-shadow: 0 0 0 2px var(--accent-subtle), 0 8px 24px rgba(0,0,0,0.28);
        }

        .page-card.selected:hover {
          border-color: var(--accent);
          box-shadow: 0 0 0 2px var(--accent-strong), 0 8px 24px rgba(0,0,0,0.32);
        }

        .page-card.dragging {
          opacity: 0.45;
          transform: rotate(2deg) scale(0.97);
        }

        .page-card.drag-over {
          border-color: var(--accent);
          box-shadow: 0 0 0 3px var(--accent-strong), 0 0 0 6px var(--accent-subtle);
          transform: translateY(-2px);
        }

        .selection-box {
          position: absolute;
          left: 8px;
          top: 8px;
          width: 22px;
          height: 22px;
          border-radius: var(--radius-full);
          background: rgba(0,0,0,0.42);
          backdrop-filter: blur(6px);
          border: 1.5px solid rgba(255,255,255,0.18);
          display: flex;
          align-items: center;
          justify-content: center;
          z-index: 2;
          transition: all var(--duration-fast) var(--ease-out);
        }

        .page-card.selected .selection-box {
          background: var(--accent);
          border-color: var(--accent);
          color: #0f172a;
        }

        .check-inner {
          display: flex;
          align-items: center;
          justify-content: center;
          color: white;
          opacity: 1;
        }

        .page-card.selected .check-inner {
          color: #0f172a;
        }

        .drag-handle {
          position: absolute;
          right: 6px;
          top: 6px;
          width: 24px;
          height: 24px;
          border-radius: var(--radius-sm);
          background: rgba(0,0,0,0.38);
          backdrop-filter: blur(6px);
          color: rgba(255,255,255,0.72);
          display: flex;
          align-items: center;
          justify-content: center;
          opacity: 0;
          transform: translateY(-2px);
          transition: all var(--duration-fast) var(--ease-out);
          z-index: 2;
          cursor: grab;
        }

        .page-card:hover .drag-handle,
        .page-card.selected .drag-handle {
          opacity: 1;
          transform: translateY(0);
        }

        .page-card:active .drag-handle {
          cursor: grabbing;
        }

        .thumb-area {
          aspect-ratio: 1 / 1.4142;
          padding: 12px;
          display: flex;
          align-items: center;
          justify-content: center;
          background: linear-gradient(180deg, var(--surface-elevated) 0%, var(--surface-card) 100%);
          overflow: hidden;
        }

        .page-paper {
          width: 86%;
          aspect-ratio: 1 / 1.4142;
          background: #ffffff;
          border-radius: 3px;
          box-shadow: 0 1px 2px rgba(0,0,0,0.10), 0 4px 12px rgba(0,0,0,0.08);
          padding: 14% 12%;
          display: flex;
          flex-direction: column;
          gap: 6px;
          position: relative;
          transition: transform var(--duration-slow) var(--ease-out), box-shadow var(--duration-fast) var(--ease-out);
          transform-origin: center center;
        }

        .page-card:hover .page-paper {
          box-shadow: 0 2px 4px rgba(0,0,0,0.12), 0 8px 20px rgba(0,0,0,0.12);
        }

        .paper-header {
          margin-bottom: 4px;
        }

        .paper-body {
          display: flex;
          flex-direction: column;
          gap: 6px;
          flex: 1;
        }

        .paper-footer {
          margin-top: auto;
          padding-top: 8px;
          border-top: 1px solid #f1f2f4;
        }

        .line {
          height: 4px;
          border-radius: 2px;
          background: #e6e8ee;
          width: 100%;
        }

        .line.medium {
          width: 78%;
        }

        .line.short {
          width: 48%;
          background: #d9dde8;
        }

        .line.tiny {
          width: 32%;
          height: 3px;
          background: #eef0f5;
        }

        .image-block {
          height: 26px;
          border-radius: 2px;
          background: linear-gradient(90deg, #edf1ff 0%, #dbe6ff 100%);
          border: 1px solid #dbe4ff;
          margin: 2px 0;
        }

        .rotation-badge {
          position: absolute;
          bottom: 6px;
          right: 6px;
          background: #1e2130;
          color: white;
          font-size: 10px;
          font-weight: 600;
          padding: 2px 5px;
          border-radius: 4px;
          line-height: 1;
        }

        .card-footer {
          display: flex;
          align-items: center;
          justify-content: space-between;
          padding: 8px 10px 9px;
          border-top: 1px solid var(--border-subtle);
          background: var(--surface-elevated);
          min-height: 36px;
        }

        .page-card.selected .card-footer {
          background: var(--surface-card);
          border-top-color: var(--border);
        }

        .page-number {
          font-size: var(--text-xs);
          font-weight: 500;
          color: var(--text-secondary);
          font-variant-numeric: tabular-nums;
          line-height: 1;
        }

        .page-card.selected .page-number {
          color: var(--text-primary);
          font-weight: 600;
        }

        .card-quick-actions {
          display: flex;
          align-items: center;
          gap: 2px;
          opacity: 0;
          transform: translateX(4px);
          transition: all var(--duration-fast) var(--ease-out);
        }

        .page-card:hover .card-quick-actions,
        .page-card.selected .card-quick-actions,
        .page-card:focus-within .card-quick-actions {
          opacity: 1;
          transform: translateX(0);
        }

        .quick-action {
          width: 22px;
          height: 22px;
          border-radius: var(--radius-xs);
          border: none;
          background: transparent;
          color: var(--text-tertiary);
          display: flex;
          align-items: center;
          justify-content: center;
          cursor: pointer;
          transition: all var(--duration-fast) var(--ease-out);
        }

        .quick-action:hover {
          background: var(--surface-hover);
          color: var(--text-primary);
        }

        /* Empty */
        .empty-state {
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          padding: 80px var(--space-xl);
          text-align: center;
          color: var(--text-secondary);
          max-width: 360px;
          margin: 0 auto;
        }

        .empty-icon {
          width: 48px;
          height: 48px;
          border-radius: var(--radius-lg);
          background: var(--surface-elevated);
          border: 1px solid var(--border);
          display: flex;
          align-items: center;
          justify-content: center;
          color: var(--text-tertiary);
          margin-bottom: var(--space-lg);
        }

        .empty-state h3 {
          margin: 0 0 6px 0;
          font-size: var(--text-lg);
          font-weight: 600;
          color: var(--text-primary);
        }

        .empty-state p {
          margin: 0;
          font-size: var(--text-sm);
          line-height: var(--leading-relaxed);
          color: var(--text-secondary);
        }
      `}</style>
    </div>
  );
};

export default OrganizeMode;
