/* eslint-disable no-restricted-syntax */
import React, { useState, useRef, useCallback, useMemo } from "react";
import {
  Check,
  RotateCw,
  RotateCcw,
  Trash2,
  Copy,
  GripVertical,
  Layers,
  X,
  FileStack,
  Plus,
  RefreshCw,
  SplitSquareHorizontal,
} from "lucide-react";
import { useProgressivePagePreviews } from "@app/hooks/useProgressivePagePreviews";
import { ExtractPagesModal } from "@app/components/viewer/modals/ExtractPagesModal";
import { SplitModal } from "@app/components/viewer/modals/SplitModal";
import { MergeWorkspaceModal } from "@app/components/viewer/modals/MergeWorkspaceModal";
import {
  BlankPageModal,
  BlankPageLocation,
  BlankPageSize,
} from "@app/components/viewer/modals/BlankPageModal";

export type OrganizeModeProps = {
  totalPages?: number;
  /**
   * The real active PDF file. When provided, OrganizeMode renders actual
   * per-page thumbnails (via pdfjs, independent of the embedpdf provider) so
   * opening Organize immediately shows the true document pages instead of
   * placeholder graphics. When null, it falls back to lightweight page
   * placeholders (e.g. before a file is available).
   */
  file?: File | null;
  selectedPages?: string[];
  onApply?: (
    pages: {
      originalId: number;
      rotation: number;
      isBlank?: boolean;
      blankDimensions?: [number, number];
    }[],
  ) => void;
  onExtract?: (pageNumbersString: string) => void;
  /**
   * Insert every page of a locally-chosen PDF into the document. The picker is
   * owned here; the parent performs the real commit. `insertAtIndex` is the
   * 0-indexed position in the current working order (after the last selected
   * page, or the end if nothing is selected). `workingPages` is the full
   * working set so the parent can honour any pending reorder/rotate/delete.
   */
  onInsert?: (
    sourceFile: File,
    insertAtIndex: number,
    workingPages: { originalId: number; rotation: number }[],
  ) => void;
  /**
   * Replace the selected pages with every page of a locally-chosen PDF.
   * `selectedIndices` are 0-indexed positions in the current working order.
   */
  onReplace?: (
    sourceFile: File,
    selectedIndices: number[],
    workingPages: { originalId: number; rotation: number }[],
  ) => void;
  onSplit?: (splits: number[][]) => void;
  onMerge?: (files: File[]) => void;
  onPageVisible?: (pageIndex: number) => void;
  className?: string;
};

type PageItem = {
  id: string; // unique key for react
  originalId: number; // 1-indexed original page number, -1 if blank
  rotation: number; // cumulative delta (0, 90, 180, 270) relative to document
  isBlank?: boolean;
  blankDimensions?: [number, number];
};

const DEFAULT_TOTAL = 12;

export const OrganizeMode: React.FC<OrganizeModeProps> = ({
  totalPages = DEFAULT_TOTAL,
  file,
  selectedPages,
  onApply,
  onExtract,
  onInsert,
  onReplace,
  onSplit,
  onMerge,
  onPageVisible,
  className,
}) => {
  const isControlledSelection = selectedPages !== undefined;

  // Real per-page thumbnails. useProgressivePagePreviews renders the actual PDF
  // pages with pdfjs directly (through pdfWorkerManager), so it works even
  // though OrganizeMode lives OUTSIDE the embedpdf provider tree. It reports the
  // true page count and streams page images in as they finish rendering.
  const [extractModalOpened, setExtractModalOpened] = useState(false);
  const [splitModalOpened, setSplitModalOpened] = useState(false);
  const [mergeModalOpened, setMergeModalOpened] = useState(false);
  const [blankPageModalOpened, setBlankPageModalOpened] = useState(false);
  const [visibleRange, setVisibleRange] = useState<
    { start: number; end: number } | undefined
  >(undefined);
  const previews = useProgressivePagePreviews({
    file: file ?? null,
    enabled: !!file,
    cacheKey: null,
    visiblePageRange: visibleRange,
  });

  // Once the real page count is known, request thumbnails for every page so the
  // whole grid fills in (the hook batches internally and skips loaded pages).
  React.useEffect(() => {
    if (previews.totalPages > 0) {
      setVisibleRange({ start: 0, end: previews.totalPages - 1 });
    }
  }, [previews.totalPages]);

  // The authoritative page count is the real document's, when we have a file.
  const resolvedTotal =
    file && previews.totalPages > 0 ? previews.totalPages : totalPages;

  // Map 1-based page number -> rendered thumbnail data URL.
  const previewByPage = useMemo(() => {
    const m = new Map<number, string>();
    for (const p of previews.pages) {
      if (p.url) m.set(p.pageNumber, p.url);
    }
    return m;
  }, [previews.pages]);

  const [internalPages, setInternalPages] = useState<PageItem[]>(() =>
    Array.from({ length: resolvedTotal }, (_, i) => ({
      id: `orig-${i + 1}`,
      originalId: i + 1,
      rotation: 0,
    })),
  );

  const [internalSelected, setInternalSelected] = useState<string[]>([]);
  const [draggedId, setDraggedId] = useState<string | null>(null);
  const [dragOverId, setDragOverId] = useState<string | null>(null);
  const anchorRef = useRef<string | null>(null);
  const [nextId, setNextId] = useState(resolvedTotal + 1);

  // Rebuild the working page list only when the underlying document changes —
  // a different file, or the real page count first becoming known — NEVER in
  // response to the user's own edits (delete / duplicate / reorder), which also
  // change internalPages.length. Tracking the last-synced file + total via refs
  // keeps user edits from being clobbered.
  const lastSyncedTotalRef = useRef<number>(resolvedTotal);
  const lastFileRef = useRef<File | null | undefined>(file);
  React.useEffect(() => {
    const fileChanged = file !== lastFileRef.current;
    if (
      resolvedTotal > 0 &&
      (fileChanged || resolvedTotal !== lastSyncedTotalRef.current)
    ) {
      lastFileRef.current = file;
      lastSyncedTotalRef.current = resolvedTotal;
      setInternalPages(
        Array.from({ length: resolvedTotal }, (_, i) => ({
          id: `orig-${i + 1}`,
          originalId: i + 1,
          rotation: 0,
        })),
      );
      setNextId(resolvedTotal + 1);
      setInternalSelected([]);
      anchorRef.current = null;
    }
  }, [resolvedTotal, file]);

  const effectiveSelected = isControlledSelection
    ? selectedPages!
    : internalSelected;

  const pages = internalPages;
  const selectedSet = useMemo(
    () => new Set(effectiveSelected),
    [effectiveSelected],
  );

  // Set up an intersection observer to track which page is currently most visible
  const observerRef = useRef<IntersectionObserver | null>(null);
  const elementsRef = useRef<Map<string, HTMLDivElement>>(new Map());

  React.useEffect(() => {
    if (!onPageVisible) return;

    const callback = (entries: IntersectionObserverEntry[]) => {
      // Find the currently intersecting element with the highest intersection ratio
      let bestMatch: IntersectionObserverEntry | null = null;
      for (const entry of entries) {
        if (entry.isIntersecting) {
          if (
            !bestMatch ||
            entry.intersectionRatio > bestMatch.intersectionRatio
          ) {
            bestMatch = entry;
          }
        }
      }

      if (bestMatch) {
        const id = bestMatch.target.getAttribute("data-page-id");
        if (id) {
          const idx = internalPages.findIndex((p) => p.id === id);
          if (idx !== -1) {
            onPageVisible(idx + 1); // 1-indexed for the global counter
          }
        }
      }
    };

    const observer = new IntersectionObserver(callback, {
      root: null, // viewport
      rootMargin: "0px",
      threshold: [0.1, 0.5, 0.9],
    });

    observerRef.current = observer;

    // Observe all current elements
    elementsRef.current.forEach((el) => observer.observe(el));

    return () => {
      observer.disconnect();
      observerRef.current = null;
    };
  }, [onPageVisible, internalPages]);

  const setPageRef = useCallback((id: string, el: HTMLDivElement | null) => {
    if (el) {
      elementsRef.current.set(id, el);
      observerRef.current?.observe(el);
    } else {
      const oldEl = elementsRef.current.get(id);
      if (oldEl) observerRef.current?.unobserve(oldEl);
      elementsRef.current.delete(id);
    }
  }, []);

  const commitSelection = useCallback((next: string[]) => {
    setInternalSelected(next);
  }, []);

  const handlePageClick = useCallback(
    (e: React.MouseEvent, pageId: string) => {
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
      if (effectiveSelected.length === 1 && effectiveSelected[0] === pageId) {
        // if clicking already single selected, deselect it
        anchorRef.current = null;
        commitSelection([]);
        return;
      }
      anchorRef.current = pageId;
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
    },
    [effectiveSelected, selectedSet],
  );

  const handleRotateSingle = useCallback(
    (e: React.MouseEvent, pageId: string, direction: "cw" | "ccw") => {
      e.stopPropagation();
      const delta = direction === "cw" ? 90 : -90;
      setInternalPages((prev) =>
        prev.map((p) =>
          p.id === pageId
            ? { ...p, rotation: (p.rotation + delta + 360) % 360 }
            : p,
        ),
      );
    },
    [],
  );

  const handleDelete = useCallback(() => {
    if (effectiveSelected.length === 0) return;
    if (effectiveSelected.length === pages.length) {
      window.alert("You cannot delete all pages from the document.");
      return;
    }
    const ok = window.confirm(
      `Are you sure you want to delete ${effectiveSelected.length} page${effectiveSelected.length === 1 ? "" : "s"}?`,
    );
    if (!ok) return;
    setInternalPages((prev) => prev.filter((p) => !selectedSet.has(p.id)));
    commitSelection([]);
  }, [effectiveSelected, selectedSet, pages.length, commitSelection]);

  const handleDuplicate = useCallback(() => {
    if (effectiveSelected.length === 0) return;
    // Duplicate preserves order of selected as they appear in pages
    const selectedInOrder = pages.filter((p) => selectedSet.has(p.id));
    const clones: PageItem[] = selectedInOrder.map((p, i) => ({
      id: `clone-${nextId}-${i}`,
      originalId: p.originalId,
      rotation: p.rotation,
    }));
    const newNextId = nextId + 1;

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
    // optionally select clones
    commitSelection(clones.map((c) => c.id));
  }, [effectiveSelected, selectedSet, pages, nextId, commitSelection]);

  const handleDragStart = useCallback((e: React.DragEvent, pageId: string) => {
    setDraggedId(pageId);
    e.dataTransfer.effectAllowed = "move";
    e.dataTransfer.setData("text/plain", pageId);
  }, []);

  const handleDragOver = useCallback(
    (e: React.DragEvent, pageId: string) => {
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
    (e: React.DragEvent, targetId: string) => {
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
        const insertAt = toIdx;
        copy.splice(insertAt, 0, moved);
        return copy;
      });
      setDraggedId(null);
      setDragOverId(null);
    },
    [draggedId],
  );

  const handleDragEnd = useCallback(() => {
    setDraggedId(null);
    setDragOverId(null);
  }, []);

  // --- Insert / Replace: pull pages from a locally-chosen PDF -------------
  // Both actions read a second PDF from disk. A single hidden <input type=file>
  // is reused; a ref records which action opened it so onChange dispatches to
  // the correct callback. The parent (ViewerShell) performs the real, fully
  // local pdf-lib commit — no cloud endpoint and no separate window, per
  // Phases 25/26.
  const sourceInputRef = useRef<HTMLInputElement | null>(null);
  const pendingSourceAction = useRef<"insert" | "replace" | null>(null);

  const workingPagesSnapshot = useCallback(
    () =>
      internalPages.map((p) => ({
        originalId: p.originalId,
        rotation: p.rotation,
        ...(p.isBlank
          ? { isBlank: p.isBlank, blankDimensions: p.blankDimensions }
          : {}),
      })),
    [internalPages],
  );

  const openSourcePicker = useCallback((action: "insert" | "replace") => {
    pendingSourceAction.current = action;
    // Clear the value first so choosing the same file twice still fires change.
    if (sourceInputRef.current) sourceInputRef.current.value = "";
    sourceInputRef.current?.click();
  }, []);

  const handleSourceFileChosen = useCallback(
    (e: React.ChangeEvent<HTMLInputElement>) => {
      const chosen = e.target.files?.[0];
      const action = pendingSourceAction.current;
      pendingSourceAction.current = null;
      if (!chosen || !action) return;

      if (action === "insert") {
        // Insert after the last selected page in the current working order; if
        // nothing is selected, append at the end. insertAt is 0-indexed and is
        // the slot the source pages will occupy once inserted.
        let insertAt = internalPages.length;
        const lastSelectedIndex = internalPages.reduce(
          (last, p, i) => (selectedSet.has(p.id) ? i : last),
          -1,
        );
        if (lastSelectedIndex >= 0) insertAt = lastSelectedIndex + 1;
        onInsert?.(chosen, insertAt, workingPagesSnapshot());
      } else {
        // Replace: 0-indexed positions of the selected pages in working order.
        const selectedIndices = internalPages
          .map((p, i) => (selectedSet.has(p.id) ? i : -1))
          .filter((i) => i >= 0);
        if (selectedIndices.length === 0) return;
        onReplace?.(chosen, selectedIndices, workingPagesSnapshot());
      }
    },
    [internalPages, selectedSet, onInsert, onReplace, workingPagesSnapshot],
  );

  // Dirty tracking. The working set (internalPages) starts as the pristine
  // document — pages 1..N in order, every rotation 0. Reorder / rotate / delete
  // / duplicate all mutate internalPages, and NONE of them touch the real PDF
  // until "Apply Changes" runs onApply -> applyOrganizeChangesLocal (which
  // rebuilds the document, honouring order, per-page rotation, and omissions).
  // Comparing the current signature to the pristine one tells us whether there
  // is anything to commit, so Apply can advertise that real work is pending
  // instead of silently letting edits look applied when the document is
  // untouched.
  const pristineSignature = useMemo(
    () =>
      Array.from({ length: resolvedTotal }, (_, i) => `${i + 1}:0`).join(","),
    [resolvedTotal],
  );
  const currentSignature = useMemo(
    () => internalPages.map((p) => `${p.originalId}:${p.rotation}`).join(","),
    [internalPages],
  );
  const isDirty = currentSignature !== pristineSignature;
  const isEmpty = internalPages.length === 0;
  // How many of the original pages are absent from the working set (i.e. would
  // be permanently dropped when the reorganized document is written).
  const removedCount = useMemo(() => {
    const present = new Set(internalPages.map((p) => p.originalId));
    let n = 0;
    for (let i = 1; i <= resolvedTotal; i++) if (!present.has(i)) n++;
    return n;
  }, [internalPages, resolvedTotal]);

  const handleApply = useCallback(() => {
    if (!isDirty || isEmpty) return;
    // Confirm before a destructive commit that removes pages (Phase 23:
    // "confirm where appropriate"). Reorder/rotate-only commits apply directly.
    if (removedCount > 0) {
      const ok = window.confirm(
        `Apply changes?\n\nThis rebuilds the document and permanently removes ${removedCount} page${
          removedCount === 1 ? "" : "s"
        }. It cannot be undone from here.`,
      );
      if (!ok) return;
    }
    onApply?.(
      internalPages.map((p) => ({
        originalId: p.originalId,
        rotation: p.rotation,
        ...(p.isBlank
          ? { isBlank: p.isBlank, blankDimensions: p.blankDimensions }
          : {}),
      })),
    );
  }, [isDirty, isEmpty, removedCount, internalPages, onApply]);

  const hasSelection = effectiveSelected.length > 0;
  const allSelected =
    effectiveSelected.length === pages.length && pages.length > 0;

  return (
    <div className={`organize-root ${className ?? ""}`}>
      {/* Hidden picker reused by Insert and Replace (fully local, no cloud). */}
      <input
        ref={sourceInputRef}
        type="file"
        accept="application/pdf,.pdf"
        style={{ display: "none" }}
        onChange={handleSourceFileChosen}
      />
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
                {isDirty && !isEmpty && (
                  <span
                    className="dirty-flag"
                    title="You have changes that haven't been written to the document yet — click Apply Changes"
                  >
                    Unsaved changes
                  </span>
                )}
              </p>
            </div>
          </div>

          <div className="organize-header-actions">
            {!hasSelection ? (
              <>
                <button
                  className={`org-ghost-btn apply-btn ${
                    isDirty && !isEmpty ? "has-changes" : ""
                  }`}
                  onClick={handleApply}
                  disabled={!isDirty || isEmpty}
                  style={{ marginRight: "8px" }}
                  title={
                    isEmpty
                      ? "Keep at least one page to apply"
                      : isDirty
                        ? "Write the current page order, rotations and deletions to the document"
                        : "No changes to apply yet"
                  }
                >
                  <Check size={16} />
                  Apply Changes
                </button>
                <button
                  className="org-ghost-btn"
                  onClick={handleSelectAll}
                  disabled={pages.length === 0}
                >
                  Select all
                </button>
              </>
            ) : (
              <>
                <button
                  className={`org-ghost-btn apply-btn ${
                    isDirty && !isEmpty ? "has-changes" : ""
                  }`}
                  onClick={handleApply}
                  disabled={!isDirty || isEmpty}
                  style={{ marginRight: "8px" }}
                  title={
                    isEmpty
                      ? "Keep at least one page to apply"
                      : isDirty
                        ? "Write the current page order, rotations and deletions to the document"
                        : "No changes to apply yet"
                  }
                >
                  <Check size={16} />
                  Apply Changes
                </button>
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
          </div>
          <div className="toolbar-group">
            <button
              className="toolbar-btn"
              onClick={() => openSourcePicker("insert")}
              disabled={!file}
              title="Insert pages from another PDF (after the selected page, or at the end)"
            >
              <Plus size={16} />
              <span className="btn-label">Insert</span>
            </button>
            <button
              className="toolbar-btn"
              onClick={() => setBlankPageModalOpened(true)}
              title="Insert a blank page"
            >
              <FileStack size={16} /> {/* Or FileText/FilePlus */}
              <span className="btn-label">Blank</span>
            </button>
            <button
              className="toolbar-btn"
              onClick={() => openSourcePicker("replace")}
              disabled={!hasSelection}
              title="Replace the selected pages with pages from another PDF"
            >
              <RefreshCw size={16} />
              <span className="btn-label">Replace</span>
            </button>
          </div>
          <div className="toolbar-divider" />
          <div className="toolbar-group">
            <button
              className="toolbar-btn"
              onClick={() => setSplitModalOpened(true)}
              disabled={pages.length < 2}
              title="Split this document into multiple PDFs"
            >
              <SplitSquareHorizontal size={16} />
              <span className="btn-label">Split</span>
            </button>
            <button
              className="toolbar-btn"
              onClick={() => setExtractModalOpened(true)}
              disabled={!hasSelection}
              title="Extract selected pages to a new file"
            >
              <FileStack size={16} />
              <span className="btn-label">Extract</span>
            </button>
            <button
              className="toolbar-btn"
              onClick={() => setMergeModalOpened(true)}
              title="Merge multiple PDFs together"
            >
              <Layers size={16} />
              <span className="btn-label">Merge</span>
            </button>
            <button
              className="toolbar-btn danger"
              onClick={handleDelete}
              disabled={!hasSelection || allSelected}
              title={
                allSelected
                  ? "Cannot delete all pages"
                  : "Delete selected pages"
              }
            >
              <Trash2 size={16} />
              <span className="btn-label">Delete</span>
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
          file && (previews.loading || previews.totalPages === 0) ? (
            <div className="empty-state">
              <div className="empty-icon">
                <div className="thumb-spinner" />
              </div>
              <h3>Loading pages…</h3>
              <p>Rendering the document’s pages.</p>
            </div>
          ) : (
            <div className="empty-state">
              <div className="empty-icon">
                <Layers size={24} />
              </div>
              <h3>No pages</h3>
              <p>
                All pages have been removed. Use duplicate or restore to
                continue.
              </p>
            </div>
          )
        ) : (
          <div className="pages-grid">
            {pages.map((page, index) => {
              const isSelected = selectedSet.has(page.id);
              const isDragging = draggedId === page.id;
              const isDragOver = dragOverId === page.id;
              const thumbUrl = previewByPage.get(page.originalId);
              // Position label reflects the page's place in the *working order*
              // (1-based), so labels renumber live as pages are reordered,
              // duplicated, or deleted (Phase 21: "page labels update"). The
              // original source page is surfaced as provenance only when it
              // differs from the current position.
              const positionLabel = index + 1;
              const movedFromSource = page.originalId !== positionLabel;

              return (
                <div
                  key={page.id}
                  ref={(el) => setPageRef(page.id, el)}
                  data-page-id={page.id}
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
                  aria-label={`Page ${positionLabel}${
                    movedFromSource
                      ? ` (from original page ${page.originalId})`
                      : ""
                  }${isSelected ? ", selected" : ""}`}
                  onKeyDown={(e) => {
                    if (e.key === "Enter" || e.key === " ") {
                      e.preventDefault();

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

                  {/* Thumbnail — real rendered PDF page (falls back to a
                      lightweight loading state until the page has rendered). */}
                  <div className="thumb-area">
                    <div
                      className="page-paper"
                      style={{ transform: `rotate(${page.rotation}deg)` }}
                      data-rotation={page.rotation}
                    >
                      {page.isBlank ? (
                        <div
                          style={{
                            background: "white",
                            width: "100%",
                            height: "100%",
                          }}
                          aria-hidden="true"
                        />
                      ) : thumbUrl ? (
                        <img
                          className="page-thumb-img"
                          src={thumbUrl}
                          alt={`Page ${positionLabel}`}
                          draggable={false}
                        />
                      ) : (
                        <div className="page-thumb-loading" aria-hidden="true">
                          <div className="thumb-spinner" />
                        </div>
                      )}

                      {/* Rotation badge */}
                      {page.rotation !== 0 && (
                        <div className="rotation-badge">{page.rotation}°</div>
                      )}
                    </div>
                  </div>

                  {/* Footer */}
                  <div className="card-footer">
                    <span className="page-label">
                      <span className="page-number">Page {positionLabel}</span>
                      {movedFromSource && (
                        <span
                          className="page-source"
                          title={`Originally page ${page.originalId}`}
                        >
                          from {page.originalId}
                        </span>
                      )}
                    </span>
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

      <ExtractPagesModal
        opened={extractModalOpened}
        onClose={() => setExtractModalOpened(false)}
        totalPages={totalPages || pages.length}
        initialSelection={pages
          .filter((p) => selectedSet.has(p.id))
          .map((p) => p.originalId)}
        onExtract={(pagesString: string) => onExtract?.(pagesString)}
      />

      <SplitModal
        opened={splitModalOpened}
        onClose={() => setSplitModalOpened(false)}
        totalPages={pages.length}
        selectedIndices={pages
          .map((p, index) => (selectedSet.has(p.id) ? index : -1))
          .filter((i) => i !== -1)}
        onSplit={(splits: number[][]) => onSplit?.(splits)}
      />

      <MergeWorkspaceModal
        opened={mergeModalOpened}
        onClose={() => setMergeModalOpened(false)}
        onMerge={(files: File[]) => onMerge?.(files)}
        initialFile={file}
      />

      <BlankPageModal
        opened={blankPageModalOpened}
        onClose={() => setBlankPageModalOpened(false)}
        hasSelection={hasSelection}
        totalPages={pages.length}
        onInsert={(count, location, pageRef, size, customDims) => {
          const newPages = [...pages];

          let insertIndex = newPages.length;
          if (location === "beginning") insertIndex = 0;
          else if (location === "end") insertIndex = newPages.length;
          else if (location === "after_page")
            insertIndex = Math.min(pageRef, newPages.length);
          else if (location === "before_selected" && hasSelection) {
            insertIndex = newPages.findIndex((p) => selectedSet.has(p.id));
            if (insertIndex === -1) insertIndex = 0;
          } else if (location === "after_selected" && hasSelection) {
            let lastSelectedIndex = -1;
            for (let i = newPages.length - 1; i >= 0; i--) {
              if (selectedSet.has(newPages[i].id)) {
                lastSelectedIndex = i;
                break;
              }
            }
            insertIndex = lastSelectedIndex + 1;
          }

          let dims: [number, number] | undefined = undefined;
          if (size === "A4") dims = [595.28, 841.89];
          else if (size === "Letter") dims = [612, 792];
          else if (size === "custom") dims = customDims;

          const blankItems: PageItem[] = Array.from({ length: count }, () => ({
            id: crypto.randomUUID(),
            originalId: -1,
            rotation: 0,
            isBlank: true,
            blankDimensions: dims,
          }));

          newPages.splice(insertIndex, 0, ...blankItems);
          setInternalPages(newPages);
        }}
      />

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

        /* Apply Changes: the single bridge from the working copy to the real
           PDF. When there are pending edits it goes solid-accent to make the
           "commit" affordance obvious; when clean/empty it stays a muted ghost
           button (via :disabled) so users aren't misled into thinking an
           unchanged doc needs saving. */
        .apply-btn {
          transition: all var(--duration-fast) var(--ease-out);
        }
        .apply-btn.has-changes {
          background: var(--accent);
          border-color: var(--accent);
          color: #0f172a;
          font-weight: 600;
          box-shadow: 0 0 0 3px var(--accent-subtle);
        }
        .apply-btn.has-changes:hover {
          background: var(--accent-hover);
          border-color: var(--accent-hover);
        }

        .dirty-flag {
          display: inline-flex;
          align-items: center;
          margin-left: 8px;
          padding: 1px 8px;
          border-radius: var(--radius-full, 999px);
          background: var(--accent-subtle);
          color: var(--accent);
          border: 1px solid var(--accent-strong);
          font-size: var(--text-xs);
          font-weight: 600;
          vertical-align: middle;
          white-space: nowrap;
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
          overflow: hidden;
          display: flex;
          align-items: center;
          justify-content: center;
          position: relative;
          transition: transform var(--duration-slow) var(--ease-out), box-shadow var(--duration-fast) var(--ease-out);
          transform-origin: center center;
        }

        .page-card:hover .page-paper {
          box-shadow: 0 2px 4px rgba(0,0,0,0.12), 0 8px 20px rgba(0,0,0,0.12);
        }

        /* Real rendered page image. object-fit: contain letterboxes any page
           aspect ratio cleanly inside the white paper frame. */
        .page-thumb-img {
          width: 100%;
          height: 100%;
          object-fit: contain;
          display: block;
          background: #ffffff;
        }

        .page-thumb-loading {
          width: 100%;
          height: 100%;
          display: flex;
          align-items: center;
          justify-content: center;
          background: linear-gradient(180deg, #fafbfc 0%, #f1f3f6 100%);
        }

        .thumb-spinner {
          width: 22px;
          height: 22px;
          border-radius: 50%;
          border: 2.5px solid var(--border, #d9dde8);
          border-top-color: var(--accent, #4f7cff);
          animation: organize-thumb-spin 0.8s linear infinite;
        }

        @keyframes organize-thumb-spin {
          to { transform: rotate(360deg); }
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

        .page-label {
          display: inline-flex;
          align-items: baseline;
          gap: 6px;
          min-width: 0;
        }
        /* Provenance hint: shown only when a page's working position differs
           from its original source page (i.e. after reorder/duplicate), so the
           user can still trace where a page came from once labels renumber. */
        .page-source {
          font-size: 10px;
          font-weight: 500;
          color: var(--text-tertiary);
          font-variant-numeric: tabular-nums;
          line-height: 1;
          white-space: nowrap;
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
