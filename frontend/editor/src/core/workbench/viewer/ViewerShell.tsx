import React, {
  useState,
  useCallback,
  useEffect,
  useRef,
  useMemo,
} from "react";
import { TabBar } from "@app/workbench/viewer/TabBar";
import { ViewerLeftRail } from "@app/workbench/viewer/ViewerLeftRail";
import { ContextualToolbar } from "@app/workbench/viewer/ContextualToolbar";
import {
  RightUtilityPanel,
  type BookmarkItem,
  type DocumentInfoData,
  type AttachmentItem,
} from "@app/workbench/viewer/RightUtilityPanel";
import { OrganizeMode } from "@app/workbench/viewer/OrganizeMode";
import { ViewerToolsGrid } from "@app/workbench/viewer/ViewerToolsGrid";
import { useToolLifecycle } from "@app/hooks/useToolLifecycle";
import { ImageSelectionOverlay } from "@app/components/viewer/ImageSelectionOverlay";
import { applyOrganizeChangesLocal } from "@app/services/offlinePageOps";
import { createChildStub } from "@app/contexts/file/fileActions";
import { isPDFEliteFile } from "@app/types/fileContext";
import {
  createPDFEliteFile,
  createFileId,
  FileId,
} from "@app/types/fileContext";
import { useSignature } from "@app/contexts/SignatureContext";
import {
  PdfBookmarkObject,
  PdfActionType,
  type PdfAttachmentObject,
} from "@embedpdf/models";
import { extractPDFMetadata } from "@app/services/pdfMetadataService";
import type { ExtractedPDFMetadata } from "@app/types/metadata";

export type Props = {
  children?: React.ReactNode;
  onClose: () => void; // back to home
  onToolSelect?: (toolId: string) => void;
};

import { useFileState, useFileActions } from "@app/contexts/FileContext";
import { useFileHandler } from "@app/hooks/useFileHandler";
import { useViewer } from "@app/contexts/ViewerContext";
import { useNavigate } from "react-router-dom";

// Phase 30: resolve the 1-based display page for a PDF outline entry. Mirrors
// BookmarkSidebar.resolvePageNumber exactly: embedPDF stores 0-based pageIndex on
// either a "destination" target or a Goto/RemoteGoto "action", so display page =
// pageIndex + 1. Returns null when the entry has no navigable destination.
const resolveBookmarkPage = (bookmark: PdfBookmarkObject): number | null => {
  const target = bookmark.target;
  if (!target) return null;
  if (target.type === "destination") {
    return target.destination.pageIndex + 1;
  }
  if (target.type === "action") {
    const action = target.action;
    if (
      action.type === PdfActionType.Goto ||
      action.type === PdfActionType.RemoteGoto
    ) {
      return action.destination?.pageIndex !== undefined
        ? action.destination.pageIndex + 1
        : null;
    }
  }
  return null;
};

// Phase 30: flatten embedPDF's PdfBookmarkObject tree into the RightUtilityPanel's
// BookmarkItem shape, preserving nesting and depth. A section header with no
// destination of its own borrows its first descendant's page so the row still
// navigates into the section instead of nowhere.
const mapBookmarks = (
  nodes: PdfBookmarkObject[],
  depth = 0,
  prefix = "bm",
): BookmarkItem[] => {
  if (!Array.isArray(nodes)) return [];
  return nodes.map((node, index) => {
    const id = `${prefix}-${depth}-${index}`;
    const children = mapBookmarks(node.children ?? [], depth + 1, id);
    const page = resolveBookmarkPage(node) ?? children[0]?.page ?? 1;
    return {
      id,
      title: node.title ?? "Untitled",
      page,
      level: depth,
      children: children.length ? children : undefined,
    };
  });
};

// Phase 35: human-readable byte size for the Inspector (attachment sizes and
// document file size). Mirrors AttachmentSidebar.formatFileSize.
const formatByteSize = (bytes?: number): string | undefined => {
  if (bytes == null || Number.isNaN(bytes)) return undefined;
  if (bytes < 1024) return `${bytes} B`;
  const units = ["KB", "MB", "GB"];
  let value = bytes / 1024;
  let unitIndex = 0;
  while (value >= 1024 && unitIndex < units.length - 1) {
    value /= 1024;
    unitIndex++;
  }
  return `${value.toFixed(1)} ${units[unitIndex]}`;
};

export const ViewerShell: React.FC<Props> = ({
  children,
  onClose,
  onToolSelect,
}) => {
  const { selectors } = useFileState();
  const { actions } = useFileActions();
  const activeFiles = selectors.getFiles();
  const {
    activeFileId,
    setActiveFileId,
    zoomActions,
    getZoomState,
    scrollActions,
    getScrollState,
    searchActions,
    getSearchState,
    registerImmediateZoomUpdate,
    registerImmediateScrollUpdate,
    registerImmediateSearchUpdate,
    rotationActions,
    cyclePdfRenderMode,
    isThumbnailSidebarVisible,
    toggleThumbnailSidebar,
    isBookmarkSidebarVisible,
    toggleBookmarkSidebar,
    isAttachmentSidebarVisible,
    toggleAttachmentSidebar,
    isLayerSidebarVisible,
    toggleLayerSidebar,
    hasBookmarkSupport,
    bookmarkActions,
    getSpreadState,
    spreadActions,
    registerImmediateSpreadUpdate,
    hasAttachmentSupport,
    attachmentActions,
    getDocumentPermissions,
  } = useViewer();
  const { annotationApiRef, signatureApiRef } = useSignature();

  // Synchronized state from viewer
  const [totalPages, setTotalPages] = useState(getScrollState().totalPages);

  // Phase 34: reader view state mirrored from the viewer so the toolbar shows
  // the true page layout (single vs facing) and fullscreen state, and only
  // exposes controls that actually work.
  const [isDualPage, setIsDualPage] = useState(
    () => getSpreadState().isDualPage,
  );
  const [isFullscreen, setIsFullscreen] = useState(false);

  const viewerCenterRef = useRef<HTMLDivElement>(null);
  // Phase 34: the shell root is the fullscreen target so entering fullscreen
  // keeps the toolbar and rails visible, not just the bare page canvas.
  const shellRef = useRef<HTMLDivElement>(null);

  const fileInputRef = useRef<HTMLInputElement>(null);
  const { addFiles } = useFileHandler();
  const navigate = useNavigate();

  useEffect(() => {
    const unregisterScroll = registerImmediateScrollUpdate((page, total) => {
      setTotalPages(total);
    });
    return () => {
      unregisterScroll();
    };
  }, [registerImmediateScrollUpdate]);

  // Phase 29: the toolbar match count and the results list below both read from
  // getSearchState() during render, but nothing re-rendered this shell when the
  // search bridge published new (deduped) results — so "N results" and the
  // active-match index could stay at 0 or go stale even with a match already
  // visible. Subscribing to the bridge's immediate search notifier and bumping
  // local state forces a re-render so those getSearchState() reads pick up the
  // fresh, accurate values. (The count math itself is correct and untouched.)
  const [, setSearchTick] = useState(0);
  useEffect(() => {
    const unregisterSearch = registerImmediateSearchUpdate(() => {
      setSearchTick((v) => v + 1);
    });
    return () => unregisterSearch();
  }, [registerImmediateSearchUpdate]);

  // Phase 30: the document outline (bookmarks) rendered in the right utility
  // panel. Previously the panel received no bookmarks, so the outline tab always
  // showed the empty state even for PDFs that have one. Here we fetch the real
  // outline from the bookmark bridge whenever the active document changes and map
  // it into the flat BookmarkItem tree the panel renders.
  const [bookmarkItems, setBookmarkItems] = useState<BookmarkItem[]>([]);
  useEffect(() => {
    let cancelled = false;
    let attempts = 0;
    // Clear the prior document's outline right away so a stale tree never lingers
    // while the next document loads.
    setBookmarkItems([]);
    const load = async () => {
      if (cancelled) return;
      // The bookmark bridge only registers once the document is ready, which can
      // land a moment after activeFileId changes; retry briefly before giving up.
      if (!hasBookmarkSupport()) {
        if (attempts++ < 15) {
          window.setTimeout(load, 300);
        }
        return;
      }
      try {
        const raw = await bookmarkActions.fetchBookmarks();
        if (!cancelled) setBookmarkItems(mapBookmarks(raw ?? []));
      } catch {
        if (!cancelled) setBookmarkItems([]);
      }
    };
    load();
    return () => {
      cancelled = true;
    };
    // hasBookmarkSupport/bookmarkActions read from stable context refs; only the
    // active document changing should trigger a re-fetch.
  }, [activeFileId]);

  // Phase 35: document attachments (embedded files) shown in the Inspector's
  // Attachments tab. Mirrors the Phase 30 bookmark fetch: the attachment bridge
  // only registers once the document is ready, and getAttachments() resolves
  // null until the document is fully open, so retry briefly before giving up.
  // The real PdfAttachmentObject[] is kept in a ref so "open" can hand the exact
  // object back to the real downloadAttachment action, while the panel renders
  // the lighter AttachmentItem shape.
  const [attachmentItems, setAttachmentItems] = useState<AttachmentItem[]>([]);
  const rawAttachmentsRef = useRef<PdfAttachmentObject[]>([]);
  useEffect(() => {
    let cancelled = false;
    let attempts = 0;
    setAttachmentItems([]);
    rawAttachmentsRef.current = [];
    const load = async () => {
      if (cancelled) return;
      if (!hasAttachmentSupport()) {
        if (attempts++ < 15) window.setTimeout(load, 300);
        return;
      }
      try {
        const raw = await attachmentActions.getAttachments();
        if (cancelled) return;
        if (raw === null) {
          if (attempts++ < 15) window.setTimeout(load, 200);
          return;
        }
        rawAttachmentsRef.current = raw;
        setAttachmentItems(
          raw.map((a, i) => ({
            id: `att-${i}`,
            name: a.name || "Untitled attachment",
            size:
              typeof a.size === "number" ? formatByteSize(a.size) : undefined,
            description: a.description || undefined,
          })),
        );
      } catch {
        if (!cancelled) {
          rawAttachmentsRef.current = [];
          setAttachmentItems([]);
        }
      }
    };
    load();
    return () => {
      cancelled = true;
    };
  }, [activeFileId]);

  // Phase 35: real document metadata (title/author/dates/producer…) for the
  // Inspector's Info tab, extracted from the actual PDF via the existing
  // metadata service. Page count, file size and permissions are merged in at
  // render time from live viewer state so the panel never shows stale numbers.
  const [pdfMeta, setPdfMeta] = useState<ExtractedPDFMetadata | null>(null);
  useEffect(() => {
    let cancelled = false;
    setPdfMeta(null);
    const file = activeFileId
      ? selectors.getFile(activeFileId as FileId)
      : null;
    if (!file) return;
    extractPDFMetadata(file)
      .then((res) => {
        if (!cancelled) setPdfMeta(res.success ? res.metadata : null);
      })
      .catch(() => {
        if (!cancelled) setPdfMeta(null);
      });
    return () => {
      cancelled = true;
    };
  }, [activeFileId]);

  // Phase 34: keep the toolbar's page-layout indicator in sync with the real
  // spread state (seeded from getSpreadState above for the first render).
  useEffect(() => {
    const unregister = registerImmediateSpreadUpdate((_mode, dual) => {
      setIsDualPage(dual);
    });
    return () => unregister();
  }, [registerImmediateSpreadUpdate]);

  // Phase 34: track real browser fullscreen so the toolbar button reflects the
  // actual state and toggles correctly.
  useEffect(() => {
    const onFsChange = () =>
      setIsFullscreen(Boolean(document.fullscreenElement));
    document.addEventListener("fullscreenchange", onFsChange);
    return () => document.removeEventListener("fullscreenchange", onFsChange);
  }, []);

  const handleToggleFullscreen = useCallback(() => {
    if (!document.fullscreenElement) {
      shellRef.current?.requestFullscreen?.().catch(() => {});
    } else {
      document.exitFullscreen?.().catch(() => {});
    }
  }, []);

  const tabs = activeFiles.map((f) => {
    const file = f;
    return {
      id: file.fileId || file.name,
      name: file.name,
      path: file.fileId || "",
      active: file.fileId === activeFileId,
    };
  });

  const activeTab = tabs.find((t) => t.active) || tabs[0];

  // The real active PDF file, kept referentially stable across renders (keyed on
  // the active file id) so OrganizeMode's thumbnail hook doesn't re-decode the
  // PDF on every render. Passed to OrganizeMode to render real page thumbnails.
  const activeFile = useMemo(
    () =>
      activeFileId ? (selectors.getFile(activeFileId as FileId) ?? null) : null,
    // selectors is stable within the file context; the active file reference
    // should only change when the active file id changes.

    [activeFileId],
  );

  // Phase 35: assemble the Inspector's Info tab from real sources — PDF metadata
  // from the extraction service, plus live page count, file size, and (only when
  // the document is actually encrypted) a readable list of allowed permissions.
  // Page/zoom are intentionally omitted; the panel must not duplicate the
  // toolbar's page and zoom controls. Empty metadata strings collapse to
  // undefined so the panel doesn't render blank rows.
  const documentInfo = useMemo<DocumentInfoData>(() => {
    const info: DocumentInfoData = {
      title: pdfMeta?.title || activeTab?.name,
      author: pdfMeta?.author || undefined,
      subject: pdfMeta?.subject || undefined,
      keywords: pdfMeta?.keywords || undefined,
      creator: pdfMeta?.creator || undefined,
      producer: pdfMeta?.producer || undefined,
      creationDate: pdfMeta?.creationDate || undefined,
      modDate: pdfMeta?.modificationDate || undefined,
      pageCount: totalPages || undefined,
      fileSize: activeFile ? formatByteSize(activeFile.size) : undefined,
    };
    try {
      const perms = getDocumentPermissions();
      if (perms?.isEncrypted) {
        const allowed: string[] = [];
        if (perms.canPrint) allowed.push("Print");
        if (perms.canCopyContents) allowed.push("Copy");
        if (perms.canModifyContents) allowed.push("Modify");
        if (perms.canModifyAnnotations) allowed.push("Annotate");
        if (perms.canFillForms) allowed.push("Fill forms");
        if (perms.canAssembleDocument) allowed.push("Assemble");
        info.permissions = allowed.length ? allowed : ["Restricted"];
      }
    } catch {
      // permissions bridge not ready yet; leave undefined
    }
    return info;
    // getDocumentPermissions reads stable bridge refs; recompute when the doc's
    // metadata or page count changes (both settle after the document loads).
  }, [pdfMeta, activeTab?.name, totalPages, activeFile]);

  const tool = useToolLifecycle();

  const [searchQuery, setSearchQuery] = useState("");
  // Inspector (right utility panel) starts collapsed so the document gets
  // maximum space. Collapsing/expanding never unmounts the viewer, so page,
  // zoom and scroll position are preserved across toggles.
  const [rightCollapsed, setRightCollapsed] = useState(true);
  // (Removed unused selectedPages state that broke OrganizeMode)
  const [showSearch, setShowSearch] = useState(false);

  // Phase 32: document position persistence. The key must be STABLE across
  // close→reopen, so it is derived from the file's identity (name|size|
  // lastModified) rather than the tab/fileId, which is a fresh per-session id
  // every time the file is reopened — that instability is why restoring never
  // worked before. A null key (no active file) disables persistence.
  const positionKey = useMemo(() => {
    if (!activeFile) return null;
    return `pdf-elite:pos:${activeFile.name}|${activeFile.size}|${activeFile.lastModified}`;
  }, [activeFile]);

  // Save the latest page + zoom whenever they change and on window close, so the
  // most recent meaningful position is always captured for this document.
  useEffect(() => {
    if (!positionKey) return;
    const save = () => {
      try {
        const page = getScrollState().currentPage;
        const zoom = getZoomState().currentZoom;
        localStorage.setItem(positionKey, JSON.stringify({ page, zoom }));
      } catch {
        // ignore quota / privacy-mode failures
      }
    };

    const unregisterScroll = registerImmediateScrollUpdate(save);
    const unregisterZoom = registerImmediateZoomUpdate(save);

    window.addEventListener("beforeunload", save);
    return () => {
      save();
      unregisterScroll();
      unregisterZoom();
      window.removeEventListener("beforeunload", save);
    };
  }, [
    positionKey,
    getScrollState,
    getZoomState,
    registerImmediateScrollUpdate,
    registerImmediateZoomUpdate,
  ]);

  // Restore the saved page (and, where possible, zoom) when a document opens.
  // The viewer reports totalPages asynchronously once pages are laid out, so
  // retry until it's ready, then jump to the saved page (clamped to range).
  // Only pages beyond the first are restored — page 1 is already the default,
  // so there is nothing to do and no unnecessary jump.
  useEffect(() => {
    if (!positionKey) return;
    let cancelled = false;
    let attempts = 0;
    let raw: string | null = null;
    try {
      raw = localStorage.getItem(positionKey);
    } catch {
      // ignore
    }
    if (!raw) return;
    let saved: { page?: number; zoom?: number } | null = null;
    try {
      saved = JSON.parse(raw);
    } catch {
      // ignore
    }
    if (!saved || typeof saved.page !== "number") return;
    const savedPage = saved.page;
    const savedZoom = saved.zoom;
    const restore = () => {
      if (cancelled) return;
      const { totalPages: tp } = getScrollState();
      if (!tp || tp <= 0) {
        if (attempts++ < 40) window.setTimeout(restore, 150);
        return;
      }
      const target = Math.min(Math.max(1, Math.round(savedPage)), tp);
      if (target > 1) {
        scrollActions.scrollToPage(target);
      }
      if (typeof savedZoom === "number" && savedZoom > 0) {
        // Best-effort zoom restore, applied after the page jump.
        zoomActions.setZoomLevel(savedZoom);
      }
    };
    // Small delay lets the viewer mount before the first readiness check.
    window.setTimeout(restore, 200);
    return () => {
      cancelled = true;
    };
    // scrollActions/zoomActions/getScrollState are stable context refs; only a
    // new document (or its stable key) should re-run restoration.
  }, [activeFileId, positionKey]);

  // Search logic - real
  const searchState = getSearchState();
  const searchResults = searchState?.results || [];
  const searchIndex = (searchState?.activeIndex || 1) - 1;

  useEffect(() => {
    if (!searchQuery.trim()) {
      searchActions.clear();
      return;
    }

    const timer = setTimeout(() => {
      searchActions.search(searchQuery.trim());
    }, 300);

    return () => clearTimeout(timer);
  }, [searchQuery, searchActions]);

  const handleSearchNext = useCallback(() => {
    searchActions.next();
  }, [searchActions]);

  const handleTabSwitch = useCallback(
    (id: string) => {
      setActiveFileId(id as FileId);
    },
    [setActiveFileId],
  );

  const handleTabClose = useCallback(
    (id: string) => {
      // Phase 31: closing a tab must not eject the user to Home while other
      // documents remain open. The REMOVE_FILES reducer leaves activeFileId
      // untouched, so closing the active document would leave activeFileId
      // pointing at a file that no longer exists and the viewer loses its
      // document. When the closed tab is the active one and siblings remain,
      // move the active selection to a neighbor first (prefer the tab to the
      // right, else the left) so the viewer swaps straight to that document.
      if (id === activeFileId && tabs.length > 1) {
        const idx = tabs.findIndex((t) => t.id === id);
        const next = tabs[idx + 1] ?? tabs[idx - 1];
        if (next) {
          setActiveFileId(next.id as FileId);
        }
      }
      actions.removeFiles([id as FileId]);
      // Only closing the final remaining tab returns to Home.
      if (tabs.length === 1) {
        onClose();
      }
    },
    [activeFileId, tabs, actions, setActiveFileId, onClose],
  );

  const handleSearchPrev = useCallback(() => {
    searchActions.previous();
  }, [searchActions]);

  // Ctrl+F handling - spec: open search, focus field, accurate counts
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "f") {
        e.preventDefault();
        setShowSearch(true);
        tool.setMode("search");
      }
    };
    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [tool]);

  // Auto-exit Add Text mode
  useEffect(() => {
    if (annotationApiRef.current?.onAnnotationEvent) {
      return annotationApiRef.current.onAnnotationEvent((event) => {
        if (event.type === "create" && tool.tempTool === "text") {
          tool.setTempTool(null as any);
        }
      });
    }
  }, [annotationApiRef.current?.onAnnotationEvent, tool.tempTool, tool.setTempTool]);

  useEffect(() => {
    const onKeyDown = (e: KeyboardEvent) => {
      if (e.key === "Escape" && showSearch) {
        // Phase 14: search open/close is ViewerShell-owned state, so ViewerShell
        // owns Escape-to-close-search (this works even when focus is outside the
        // search input; the input's own onKeyDown handles the focused case).
        // Temp-tool cancellation and mode-exit belong SOLELY to
        // useToolLifecycle's capture-phase Escape handler — we no longer also
        // call setTempTool(null) here, because two competing Escape handlers made
        // the behavior order-dependent. Guarding on showSearch means this handler
        // only fires for the one concern it owns.
        e.preventDefault();
        setShowSearch(false);
        tool.setMode("view");
        setSearchQuery("");
      }
    };
    window.addEventListener("keydown", onKeyDown);
    return () => window.removeEventListener("keydown", onKeyDown);
  }, [tool, showSearch]);

  const isOrganize = tool.mode === "organize";

  // Auto-switch back to view mode after placing a click-to-place annotation (like Add Text or Note)
  useEffect(() => {
    if (!annotationApiRef.current) return;

    // Check if the current tool is one of the click-to-place tools
    const toolId = tool.tempTool;
    const isSingleUseTool = toolId === "text" || toolId === "note";

    if (!isSingleUseTool) return;

    const unsubscribe = annotationApiRef.current.onAnnotationEvent?.(
      (event: import("@app/components/viewer/viewerTypes").AnnotationEvent) => {
        // When a new annotation is fully committed, reset the tool to select
        // This prevents the user from accidentally dropping multiple text boxes,
        // while safely keeping the annotation environment active for inline typing.
        if (event.type === "create" && event.committed) {
          // Use setTimeout to avoid interrupting the immediate render cycle
          setTimeout(() => {
            tool.setTempTool("select");
          }, 150);
        }
      },
    );

    return () => {
      if (typeof unsubscribe === "function") unsubscribe();
    };
  }, [tool.tempTool, tool.setTempTool]);

  useEffect(() => {
    if (!annotationApiRef.current) return;
    const t = tool.tempTool;

    // Phase 9 (highlight color): keep the highlight tool's DEFAULT color in
    // sync with the picker at ALL times, without activating the tool.
    // setAnnotationStyle only sets tool defaults (it never consumes the live
    // text selection), so the chosen/persisted color reaches BOTH the toolbar
    // Highlight button AND the embedpdf TextSelectionMenu — the latter applies
    // the highlight tool's CURRENT default to the selection with no color
    // argument of its own. This also fixes an ordering bug: activateAnnotationTool
    // sets the active tool (consuming the selection) before applying defaults,
    // so pre-syncing the default here means a selection-first highlight no
    // longer uses the previous color.
    annotationApiRef.current.setAnnotationStyle?.("highlight", {
      color: tool.highlightColor,
    });

    if (t === "highlight") {
      signatureApiRef.current?.deactivateTools?.();
      annotationApiRef.current.activateAnnotationTool("highlight", {
        color: tool.highlightColor,
      });
    } else if (t === "area-highlight") {
      // Phase 10 (area highlight): a true "drag across an area" highlight is a
      // rectangle, so use the real SQUARE annotation tool (drag to size, live
      // preview while dragging, Escape cancels the in-progress drag, and
      // deactivateToolAfterCreate returns to a sensible tool state) styled as a
      // translucent highlight — NOT the freehand inkHighlighter, which produces
      // a scribble rather than an area. This reuses existing SQUARE support
      // instead of inventing a new tool, and yields a real persistent annotation.
      signatureApiRef.current?.deactivateTools?.();
      annotationApiRef.current.activateAnnotationTool("square", {
        color: tool.highlightColor, // translucent fill
        strokeColor: tool.highlightColor, // matching border
        opacity: tool.highlightOpacity / 100,
        fillOpacity: tool.highlightOpacity / 100,
        strokeOpacity: Math.min(1, tool.highlightOpacity / 100 + 0.3),
        borderWidth: 1,
      });
    } else if (t === "underline") {
      // Phase 11 (underline): apply a real UNDERLINE annotation to the live
      // selection using the tool's own readable default color (amber), which
      // matches what the embedpdf TextSelectionMenu applies — so the two entry
      // points stay consistent. (Previously this forced the highlight color,
      // producing an odd, hard-to-see underline that disagreed with the menu.)
      signatureApiRef.current?.deactivateTools?.();
      annotationApiRef.current.activateAnnotationTool("underline");
    } else if (t === "strikeout") {
      // Phase 11 (strikethrough): apply a real STRIKEOUT annotation to the live
      // selection using the tool's own default color (red), matching the
      // embedpdf TextSelectionMenu for consistency across both entry points.
      signatureApiRef.current?.deactivateTools?.();
      annotationApiRef.current.activateAnnotationTool("strikeout");
    } else if (t === "note") {
      // Phase 12 (sticky note): activate the REAL registered "note" tool — a
      // FREETEXT sticky note with click-to-place (clickBehavior enabled,
      // defaultSize 160x100) and selectAfterCreate, so clicking a spot on the
      // page creates the note and drops straight into its editor. The previous
      // "textComment" id is NOT registered in this viewer's tool set
      // (see LocalEmbedPDF.ensureTool), so activating it did nothing — that was
      // the Comment-mode "note" failure the plan references.
      signatureApiRef.current?.deactivateTools?.();
      annotationApiRef.current.activateAnnotationTool("note");
    } else if (t === "text") {
      // Phase 12 (text comment) / Phase 15 (Add Text): place a real FREETEXT
      // annotation using the tool's own readable default color. Previously it
      // forced tool.highlightColor as the FONT color, so a text comment came
      // out as near-invisible pale yellow text on a white page.
      signatureApiRef.current?.deactivateTools?.();
      annotationApiRef.current.activateAnnotationTool("text");
    } else if (t === "draw") {
      // Phase 13 (pen / drawing): activate the real INK annotation tool
      // ("ink" = "Pen"), which stores freehand strokes in the existing
      // annotation state as an INK subtype and stays armed for consecutive
      // strokes (deactivateToolAfterCreate:false). Do NOT use the signature
      // bridge's activateDrawMode — that activates the deliberately-separate
      // "signatureInk" capture pen and flips the viewer into signature
      // PLACEMENT mode (a different workflow), and it no-ops when the signature
      // layer isn't mounted, which is why the Draw button previously did
      // nothing. Order matters: signature.deactivateTools() sets the annotation
      // active tool to null, so deactivate signature FIRST and activate ink
      // LAST (activate-last wins). Escape cancels an unfinished stroke / exits
      // via the tool lifecycle's capture-phase handler.
      signatureApiRef.current?.deactivateTools?.();
      annotationApiRef.current.activateAnnotationTool("ink");
    } else {
      signatureApiRef.current?.deactivateTools?.();
      annotationApiRef.current.activateAnnotationTool("select");
    }
  }, [tool.tempTool, tool.highlightColor, annotationApiRef, signatureApiRef]);

  const handleHighlightColorChange = useCallback(
    (hex: string) => {
      tool.setHighlightColor(hex);
    },
    [tool],
  );

  return (
    <div className="viewer-shell" ref={shellRef}>
      {/* Hidden file input for opening new PDFs */}
      <input
        type="file"
        ref={fileInputRef}
        style={{ display: "none" }}
        accept=".pdf,application/pdf"
        multiple
        onChange={(e) => {
          if (e.target.files?.length) {
            addFiles(Array.from(e.target.files));
          }
          if (fileInputRef.current) {
            fileInputRef.current.value = "";
          }
        }}
      />
      {/* Tab bar - single, polished, fully visible */}
      <div className="viewer-tab-strip">
        {/* eslint-disable no-restricted-syntax */}
        <div 
          className="app-logo-mini"
          onClick={() => {
            if (onClose) onClose();
            navigate("/");
          }}
          style={{ cursor: "pointer" }}
          title="Return to Home"
        >
          <img
            src="/modern-logo/logo.svg"
            alt="PDF Elite Logo"
            style={{ height: "18px" }}
          />
          <span>PDF Elite</span>
        </div>
        {/* eslint-enable no-restricted-syntax */}
        <div className="tab-strip-divider" />
        <TabBar
          tabs={tabs.map((t) => ({
            id: t.id,
            name: t.name,
            path: t.path,
            active: t.active,
          }))}
          onSwitch={handleTabSwitch}
          onClose={handleTabClose}
          onNew={() => fileInputRef.current?.click()}
        />
        {/* eslint-disable no-restricted-syntax */}
        <div className="window-controls">
          <button type="button" className="wc-btn">
            —
          </button>
          <button type="button" className="wc-btn">
            □
          </button>
          <button type="button" className="wc-btn close" onClick={onClose}>
            ✕
          </button>
        </div>
        {/* eslint-enable no-restricted-syntax */}
      </div>

      {/* Contextual toolbar - changes per left mode */}
      <ContextualToolbar
        mode={showSearch ? "search" : tool.mode}
        tempTool={tool.tempTool}
        onTempTool={tool.setTempTool}
        onRotateRight={() => rotationActions.rotateForward()}
        onCycleViewMode={() => cyclePdfRenderMode()}
        isDualPage={isDualPage}
        onTogglePageLayout={() => spreadActions.toggleSpreadMode()}
        isFullscreen={isFullscreen}
        onToggleFullscreen={handleToggleFullscreen}
        highlightColor={tool.highlightColor}
        highlightOpacity={tool.highlightOpacity}
        highlightColors={tool.highlightColors}
        onHighlightColor={handleHighlightColorChange}
        onHighlightOpacity={tool.setHighlightOpacity}
        searchQuery={searchQuery}
        searchCount={{
          current: searchResults.length ? searchIndex + 1 : 0,
          total: searchResults.length,
        }}
        onSearchChange={setSearchQuery}
        onSearchNext={handleSearchNext}
        onSearchPrev={handleSearchPrev}
        onCloseSearch={() => {
          setShowSearch(false);
          tool.setMode("view");
          setSearchQuery("");
        }}
        onToolSelect={onToolSelect}
      />

      <div className="viewer-body">
        <ViewerLeftRail
          activeMode={tool.mode}
          onModeChange={(m) => {
            if (m === "search") {
              setShowSearch(true);
            } else {
              setShowSearch(false);
              tool.setMode(m);
            }
          }}
          sidebarOpen={
            isThumbnailSidebarVisible
              ? "thumbnails"
              : isBookmarkSidebarVisible
                ? "bookmarks"
                : isAttachmentSidebarVisible
                  ? "attachments"
                  : null
          }
          onToggleSidebar={(sb) => {
            if (sb === "thumbnails") {
              toggleThumbnailSidebar();
              if (isBookmarkSidebarVisible) toggleBookmarkSidebar();
              if (isAttachmentSidebarVisible) toggleAttachmentSidebar();
              if (isLayerSidebarVisible) toggleLayerSidebar();
            }
            if (sb === "bookmarks") {
              toggleBookmarkSidebar();
              if (isThumbnailSidebarVisible) toggleThumbnailSidebar();
              if (isAttachmentSidebarVisible) toggleAttachmentSidebar();
              if (isLayerSidebarVisible) toggleLayerSidebar();
            }
            if (sb === "attachments") {
              toggleAttachmentSidebar();
              if (isThumbnailSidebarVisible) toggleThumbnailSidebar();
              if (isBookmarkSidebarVisible) toggleBookmarkSidebar();
              if (isLayerSidebarVisible) toggleLayerSidebar();
            }
          }}
        />

        <div className="viewer-center" ref={viewerCenterRef}>
          {/*
            Text-selection actions (Highlight / Underline / Strikeout / Comment /
            Copy) are supplied by the REAL embedpdf selection menu
            (components/viewer/TextSelectionMenu, wired into <SelectionLayer> in
            LocalEmbedPDF). That menu applies the action to the actual current
            selection (it calls setActiveTool, which consumes the live
            selection) and is positioned on the selected text. A thinner
            workbench TextSelectionPopover used to render on the same selection,
            producing two competing popovers, so it was removed to leave a
            single, real selection menu. The orphaned component file is left in
            place (see FUNCTIONALITY_INVENTORY.md) rather than blind-deleted.
          */}
          {isOrganize && (
            <OrganizeMode
              file={activeFile ?? null}
              totalPages={totalPages || 0}
              onExtract={async (pagesToExtractString: string) => {
                if (!activeFileId || !pagesToExtractString.trim()) return;
                const fileId = activeFileId as FileId;
                const stub = selectors.getPDFEliteFileStub(fileId);
                const file = selectors.getFile(fileId);
                if (!stub || !file) return;
                try {
                  const { extractPagesLocal } =
                    await import("@app/services/offlinePageOps");
                  const blob = await extractPagesLocal(
                    file,
                    pagesToExtractString,
                  );
                  const newFile = new File([blob], `extracted_${file.name}`, {
                    type: "application/pdf",
                  });
                  const addedFiles = await actions.addFiles([newFile], {
                    selectFiles: true,
                  });
                  if (addedFiles.length > 0) {
                    setActiveFileId(addedFiles[0].fileId);
                  }
                  tool.setMode("view");
                } catch (e) {
                  console.error("Extract failed:", e);
                }
              }}
              onInsert={async (sourceFile, insertAtIndex, workingPages) => {
                if (!activeFileId) return;
                const fileId = activeFileId as FileId;
                const stub = selectors.getPDFEliteFileStub(fileId);
                const file = selectors.getFile(fileId);
                if (!stub || !file) return;
                try {
                  const { insertPagesLocal } =
                    await import("@app/services/offlinePageOps");
                  // Bake any pending working edits (reorder / rotate / delete)
                  // into the base first, so insertAtIndex lines up with what the
                  // user currently sees on screen. If nothing is pending this is
                  // effectively the original file.
                  const baseBlob = workingPages.length
                    ? await applyOrganizeChangesLocal(file, workingPages)
                    : file;
                  const blob = await insertPagesLocal(
                    baseBlob,
                    sourceFile,
                    insertAtIndex,
                  );
                  const newFile = new File([blob], file.name, {
                    type: "application/pdf",
                  });
                  const newPDFEliteFile = createPDFEliteFile(
                    newFile,
                    createFileId(),
                  );
                  const newStub = createChildStub(
                    stub,
                    // eslint-disable-next-line @typescript-eslint/no-explicit-any
                    { toolId: "organizePages" as any, timestamp: Date.now() },
                    newFile,
                  );
                  await actions.consumeFiles(
                    [fileId],
                    [newPDFEliteFile],
                    [newStub],
                  );
                  // Point the viewer at the new version and stay in organize
                  // mode: OrganizeMode's file-sync effect rebuilds the working
                  // set, so the new page count and thumbnails refresh in place
                  // (Phase 25 exit criterion).
                  setActiveFileId(newPDFEliteFile.fileId);
                } catch (e) {
                  console.error("Insert failed:", e);
                }
              }}
              onSplit={async (splits: number[][]) => {
                if (!activeFileId || splits.length === 0) return;
                const fileId = activeFileId as FileId;
                const stub = selectors.getPDFEliteFileStub(fileId);
                const file = selectors.getFile(fileId);
                if (!stub || !file) return;
                try {
                  const { splitAdvancedLocal } =
                    await import("@app/services/offlinePageOps");
                  const blobs = await splitAdvancedLocal(file, splits);

                  const newFiles = blobs.map(
                    (blob, index) =>
                      new File(
                        [blob],
                        `${file.name.replace(/\.pdf$/i, "")}_Part_${index + 1}.pdf`,
                        {
                          type: "application/pdf",
                        },
                      ),
                  );

                  const addedFiles = await actions.addFiles(newFiles, {
                    selectFiles: true,
                  });
                  if (addedFiles.length > 0) {
                    setActiveFileId(addedFiles[0].fileId);
                  }
                  tool.setMode("view");
                } catch (e) {
                  console.error("Split failed:", e);
                }
              }}
              onMerge={async (files: File[]) => {
                try {
                  const { mergePagesLocal } =
                    await import("@app/services/offlinePageOps");
                  const blob = await mergePagesLocal(files);

                  const newFile = new File(
                    [blob],
                    `merged_${files[0].name.replace(/\.pdf$/i, "")}_etc.pdf`,
                    {
                      type: "application/pdf",
                    },
                  );

                  const addedFiles = await actions.addFiles([newFile], {
                    selectFiles: true,
                  });
                  if (addedFiles.length > 0) {
                    setActiveFileId(addedFiles[0].fileId);
                  }
                  tool.setMode("view");
                } catch (e) {
                  console.error("Merge failed:", e);
                }
              }}
              onReplace={async (sourceFile, selectedIndices, workingPages) => {
                if (!activeFileId || selectedIndices.length === 0) return;
                const fileId = activeFileId as FileId;
                const stub = selectors.getPDFEliteFileStub(fileId);
                const file = selectors.getFile(fileId);
                if (!stub || !file) return;
                try {
                  const { replacePagesLocal } =
                    await import("@app/services/offlinePageOps");
                  // Bake pending working edits into the base so selectedIndices
                  // (working-order positions) map to the right pages.
                  const baseBlob = workingPages.length
                    ? await applyOrganizeChangesLocal(file, workingPages)
                    : file;
                  const blob = await replacePagesLocal(
                    baseBlob,
                    sourceFile,
                    selectedIndices,
                  );
                  const newFile = new File([blob], file.name, {
                    type: "application/pdf",
                  });
                  const newPDFEliteFile = createPDFEliteFile(
                    newFile,
                    createFileId(),
                  );
                  const newStub = createChildStub(
                    stub,
                    // eslint-disable-next-line @typescript-eslint/no-explicit-any
                    { toolId: "organizePages" as any, timestamp: Date.now() },
                    newFile,
                  );
                  await actions.consumeFiles(
                    [fileId],
                    [newPDFEliteFile],
                    [newStub],
                  );
                  // Point the viewer at the new version and stay in organize
                  // mode so thumbnails / page state refresh in place (Phase 26
                  // exit criterion).
                  setActiveFileId(newPDFEliteFile.fileId);
                } catch (e) {
                  console.error("Replace failed:", e);
                }
              }}
              onApply={async (pages) => {
                if (!activeFileId) return;
                const fileId = activeFileId as FileId;
                const stub = selectors.getPDFEliteFileStub(fileId);
                const file = selectors.getFile(fileId);
                if (!stub || !file) return;

                try {
                  const blob = await applyOrganizeChangesLocal(file, pages);
                  const newFile = new File([blob], file.name, {
                    type: "application/pdf",
                  });
                  const newPDFEliteFile = createPDFEliteFile(
                    newFile,
                    createFileId(),
                  );

                  const newStub = createChildStub(
                    stub,
                    // eslint-disable-next-line @typescript-eslint/no-explicit-any
                    { toolId: "organizePages" as any, timestamp: Date.now() },
                    newFile,
                  );

                  await actions.consumeFiles(
                    [fileId],
                    [newPDFEliteFile],
                    [newStub],
                  );

                  // Exit organize mode
                  tool.setMode("view");
                } catch (e) {
                  console.error("Organize failed:", e);
                }
              }}
            />
          )}

          <div
            style={{
              flex: 1,
              position: "relative",
              zIndex: 1,
              display: isOrganize ? "none" : "block",
              width: "100%",
              height: "100%",
              // Keep the embedded viewer's <Viewport> the single scroll owner.
              // This wrapper must never become a second scroll surface or chain
              // scroll to the app root, so it clips instead of scrolling.
              overflow: "hidden",
              overscrollBehavior: "contain",
            }}
          >
            {tool.mode === "tools" ? (
              <ViewerToolsGrid onToolSelect={onToolSelect} />
            ) : (
              children
            )}
            
            <ImageSelectionOverlay
              isActive={tool.tempTool === "replaceImage"}
              file={activeFileId ? selectors.getPDFEliteFileStub(activeFileId as any) || null : null}
              mode="replace"
              onImageSelect={(pageNumber, imageIndex) => {
                // Store selection so the backend tools can read them automatically
                sessionStorage.setItem("pdf-elite-selected-image", JSON.stringify({ pageNumber, imageIndex }));
                
                if (tool.tempTool === "replaceImage") {
                  // Trigger replacement flow
                  const input = document.createElement("input");
                  input.type = "file";
                  input.accept = "image/*";
                  input.onchange = async (e) => {
                    const file = (e.target as HTMLInputElement).files?.[0];
                    if (file) {
                      // We need a robust way to pass this file to ReplaceImage.tsx
                      (window as any).__pdfEliteSelectedImageFile = file;
                      import("@app/components/toast").then(({ alert }) => {
                        alert({ title: "Success", body: "Image selected! Opening Replace tool...", alertType: "success" });
                      });
                      if (onToolSelect) onToolSelect("replaceImage");
                      tool.setTempTool(null);
                    }
                  };
                  input.click();
                }
              }}
            />
          </div>

          {/* Phase 12: Comment mode stays entirely inside the viewer (no
              separate window). This panel is an honest guide to the real comment
              tools — it contains NO fabricated author/thread/annotation data.
              The actual comments are real annotations the user places on the
              document via the toolbar tools above; the definitive comment list
              lives in the embedpdf CommentsSidebar, which must render inside the
              annotation provider tree and so cannot be mounted from this shell. */}
        </div>

        <RightUtilityPanel
          collapsed={rightCollapsed}
          onToggle={() => setRightCollapsed(!rightCollapsed)}
          totalPages={totalPages || 1}
          mode={tool.mode}
          searchQuery={searchQuery}
          searchResults={searchResults.map((r, i) => ({
            id: `${i}`,
            page: r.pageIndex + 1,
            preview: "Match on page " + (r.pageIndex + 1),
            active: i === searchIndex,
          }))}
          onSearchResultClick={(r) => searchActions.goToResult(parseInt(r.id))}
          documentInfo={documentInfo}
          attachments={attachmentItems}
          onAttachmentOpen={(item) => {
            const idx = attachmentItems.findIndex((a) => a.id === item.id);
            const original =
              idx >= 0 ? rawAttachmentsRef.current[idx] : undefined;
            if (original) attachmentActions.downloadAttachment(original);
          }}
          bookmarks={bookmarkItems}
          onBookmarkNavigate={(b) => scrollActions.scrollToPage(b.page)}
        />
      </div>

      <style>{`
        .viewer-shell {
          display: flex;
          flex-direction: column;
          height: 100vh;
          background: var(--app-bg);
          overflow: hidden;
          font-family: var(--font-sans);
        }
        .viewer-tab-strip {
          height: var(--tab-bar-height);
          background: var(--tab-bar-bg);
          border-bottom: 1px solid var(--border);
          display: flex;
          align-items: center;
          gap: 0;
          flex-shrink: 0;
          -webkit-app-region: drag;
        }
        .app-logo-mini {
          display: flex;
          align-items: center;
          gap: 8px;
          padding: 0 16px;
          border: none;
          background: transparent;
          font-weight: 600;
          font-size: 13px;
          color: var(--text-primary);
          height: 100%;
        }
        .tab-strip-divider {
          width: 1px;
          height: 24px;
          background: var(--border);
          margin-right: 8px;
        }
        .mini-icon {
          width: 18px;
          height: 18px;
          background: var(--accent);
          border-radius: 4px;
        }
        .window-controls {
          margin-left: auto;
          display: flex;
          height: 100%;
          flex-shrink: 0;
        }
        .wc-btn {
          width: 46px;
          height: 100%;
          border: none;
          background: transparent;
          color: var(--text-secondary);
          cursor: pointer;
          font-size: 14px;
        }
        .wc-btn:hover {
          background: var(--surface-hover);
          color: var(--text-primary);
        }
        .wc-btn.close:hover {
          background: var(--destructive);
          color: white;
        }
        .viewer-body {
          flex: 1;
          display: flex;
          overflow: hidden;
          min-height: 0;
        }
        .viewer-center {
          flex: 1;
          display: flex;
          flex-direction: column;
          min-width: 0;
          position: relative;
          overflow: hidden;
          background: var(--workspace-paper-bg);
        }
        .comment-inline-panel {
          position: absolute;
          right: 12px;
          top: 12px;
          width: 320px;
          background: var(--surface-elevated);
          border: 1px solid var(--border);
          border-radius: 12px;
          box-shadow: var(--page-shadow);
          z-index: 10;
          overflow: hidden;
        }
        .cip-header {
          padding: 12px 14px;
          border-bottom: 1px solid var(--border);
          background: var(--surface-card);
        }
        .cip-header h4 {
          margin: 0;
          font-size: 13px;
          font-weight: 600;
        }
        .cip-subtitle {
          font-size: 11px;
          color: var(--success);
        }
        .cip-content {
          padding: 12px;
          display: flex;
          flex-direction: column;
          gap: 12px;
        }
        .cip-guide {
          margin: 0;
          padding-left: 18px;
          display: flex;
          flex-direction: column;
          gap: 10px;
          font-size: 12px;
          line-height: 1.45;
          color: var(--text-secondary);
        }
        .cip-guide strong {
          color: var(--text-primary);
          font-weight: 600;
        }
        .cip-guide kbd {
          font-family: var(--font-mono, monospace);
          font-size: 10px;
          padding: 1px 5px;
          border: 1px solid var(--border-strong);
          border-radius: 4px;
          background: var(--surface-card);
          color: var(--text-primary);
        }
      `}</style>
    </div>
  );
};
