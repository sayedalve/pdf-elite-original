/* eslint-disable */
import React, { useState, useCallback, useEffect } from "react";
import { TabBar } from "./TabBar";
import { ViewerLeftRail } from "./ViewerLeftRail";
import { ContextualToolbar } from "./ContextualToolbar";
import { RightUtilityPanel } from "./RightUtilityPanel";
import { OrganizeMode } from "./OrganizeMode";
import { CommentMode } from "./CommentMode";
import { EditMode } from "./EditMode";
import { Search, X } from "lucide-react";
import { useFileState, useFileActions } from "@app/contexts/FileContext";
import { useViewer } from "@app/contexts/ViewerContext";
import {
  useNavigationState,
  useNavigationActions,
} from "@app/contexts/NavigationContext";
import { isPDFEliteFile } from "@app/types/fileContext";
import { useNavigate } from "react-router-dom";

type Props = {
  children?: React.ReactNode;
  onClose: () => void;
};

// Mock document data
const mockDoc = {
  title: "Assignment 3.pdf",
  pages: 8,
};

export const ViewerShell: React.FC<Props> = ({ children, onClose }) => {
  const { selectors } = useFileState();
  const { actions } = useFileActions();
  const activeFiles = selectors.getFiles();
  const {
    activeFileId,
    setActiveFileId,
    zoomActions,
    getZoomState,
    getScrollState,
    scrollActions,
    searchActions,
    getSearchState,
    registerImmediateZoomUpdate,
    registerImmediateScrollUpdate,
  } = useViewer();
  const { workbench: currentView } = useNavigationState();
  const { actions: navActions } = useNavigationActions();
  const navigate = useNavigate();

  // Synchronized state from viewer
  const [zoomPercent, setZoomPercent] = useState(getZoomState().zoomPercent);
  const [zoomScale, setZoomScale] = useState(getZoomState().currentZoom);
  const [currentPage, setCurrentPage] = useState(getScrollState().currentPage);
  const [totalPages, setTotalPages] = useState(getScrollState().totalPages);

  useEffect(() => {
    const unregisterZoom = registerImmediateZoomUpdate((percent) => {
      setZoomPercent(percent);
      setZoomScale(percent / 100);
    });
    const unregisterScroll = registerImmediateScrollUpdate((page, total) => {
      setCurrentPage(page);
      setTotalPages(total);
    });
    return () => {
      unregisterZoom();
      unregisterScroll();
    };
  }, [registerImmediateZoomUpdate, registerImmediateScrollUpdate]);

  // Convert activeFiles to tabs
  const tabs = activeFiles.map((f) => {
    const file = f as any;
    return {
      id: file.fileId || file.name,
      name: file.name,
      path: file.fileId || "",
      active: file.fileId === activeFileId,
      page: currentPage,
      totalPages: totalPages,
      zoom: zoomScale,
    };
  });

  const activeTab = tabs.find((t) => t.active) || tabs[0];

  const [searchQuery, setSearchQuery] = useState("");
  const [rightCollapsed, setRightCollapsed] = useState(false);
  const [selectedPages, setSelectedPages] = useState<number[]>([]);
  const [showSearch, setShowSearch] = useState(false);

  const [mode, setMode] = useState<string>("view");
  const [tempTool, setTempTool] = useState<string | null>("hand");
  const [highlightColor, setHighlightColor] = useState<string>("#fde047");

  // Search logic - Phase 12
  const searchState = getSearchState();
  const searchResults = searchState?.results || [];
  const searchIndex = (searchState?.activeIndex || 1) - 1;

  useEffect(() => {
    if (!searchQuery.trim()) {
      searchActions.clear();
      return;
    }

    // Perform search via viewer context
    // We use a small debounce to avoid spamming the search API
    const timer = setTimeout(() => {
      searchActions.search(searchQuery.trim());
    }, 300);

    return () => clearTimeout(timer);
  }, [searchQuery, searchActions]);

  const handleSearchNext = useCallback(() => {
    searchActions.next();
  }, [searchActions]);

  const handleSearchPrev = useCallback(() => {
    searchActions.previous();
  }, [searchActions]);

  const handleTabSwitch = useCallback(
    (id: string) => {
      setActiveFileId(id as any);
    },
    [setActiveFileId],
  );

  const handleTabClose = useCallback(
    (id: string) => {
      actions.removeFiles([id as any]);
      if (tabs.length === 1) {
        onClose();
      }
    },
    [actions.removeFiles, tabs.length, onClose],
  );

  const handlePageChange = useCallback(
    (page: number) => {
      scrollActions.scrollToPage(page);
    },
    [scrollActions],
  );

  // Ctrl+F handling
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "f") {
        e.preventDefault();
        setShowSearch(true);
        setMode("search");
      }
    };
    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, []);

  // Escape handling for Phase 10
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === "Escape") {
        if (showSearch) {
          setShowSearch(false);
          setSearchQuery("");
          searchActions.clear();
        } else if (tempTool !== "hand" && tempTool !== null) {
          setTempTool("hand");
        } else if (mode !== "view") {
          setMode("view");
        }
      }
    };
    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [
    showSearch,
    tempTool,
    mode,
    setMode,
    setTempTool,
    setShowSearch,
    setSearchQuery,
  ]);

  const isOrganize = mode === "organize";
  const isCommentErrorFixed = true;

  return (
    <div className="viewer-shell">
      {/* Tab bar - single, polished, fully visible */}
      <div className="viewer-tab-strip">
        <div
          className="app-logo-mini"
          onClick={() => navigate("/")}
          style={{ cursor: "pointer" }}
          title="Return to Home"
        >
          <div className="mini-icon" />
          <span>PDF Elite</span>
        </div>
        <TabBar
          tabs={tabs.map((t) => ({
            id: t.id,
            name: t.name,
            path: t.path,
            active: t.active,
          }))}
          onSwitch={handleTabSwitch}
          onClose={handleTabClose}
          onNew={() => {}}
        />
        <div className="window-controls">
          <button className="wc-btn">—</button>
          <button className="wc-btn">□</button>
          <button className="wc-btn close" onClick={onClose}>
            ✕
          </button>
        </div>
      </div>

      {/* Contextual toolbar - changes per left mode */}
      <ContextualToolbar
        mode={showSearch ? "search" : mode}
        tempTool={tempTool}
        onModeChange={setMode}
        onTempTool={setTempTool}
        zoom={{
          percentage: zoomPercent,
          scale: zoomScale,
        }}
        onZoomIn={zoomActions.zoomIn}
        onZoomOut={zoomActions.zoomOut}
        onFitWidth={() => zoomActions.requestZoom("fit-width")}
        onFitPage={() => zoomActions.requestZoom("fit-page")}
        onActualSize={() => zoomActions.requestZoom("actual-size")}
        onZoomSlider={(val) => zoomActions.setZoomLevel(val / 100)}
        highlightColor={highlightColor}
        highlightColors={[
          { id: "1", hex: "#fde047", name: "Yellow" },
          { id: "2", hex: "#fca5a5", name: "Red" },
          { id: "3", hex: "#86efac", name: "Green" },
          { id: "4", hex: "#93c5fd", name: "Blue" },
          { id: "5", hex: "#c084fc", name: "Purple" },
        ]}
        onHighlightColor={setHighlightColor}
        searchQuery={searchQuery}
        searchCount={{
          current: searchResults.length ? Math.max(1, searchIndex + 1) : 0,
          total: searchResults.length,
        }}
        onSearchChange={setSearchQuery}
        onSearchNext={handleSearchNext}
        onSearchPrev={handleSearchPrev}
        onCloseSearch={() => {
          setShowSearch(false);
          setMode("view");
          setSearchQuery("");
          searchActions.clear();
        }}
      />

      <div className="viewer-body">
        <ViewerLeftRail
          activeMode={mode}
          onModeChange={(m) => {
            if (m === "search") {
              setShowSearch(true);
            } else {
              setShowSearch(false);
              setMode(m);
              navActions.setWorkbench("viewer");
            }
          }}
          page={getScrollState().currentPage}
          totalPages={getScrollState().totalPages || 1}
          onPageChange={handlePageChange}
        />

        <div className="viewer-center">
          {mode === "organize" ? (
            <div style={{ flex: 1, overflow: "hidden", display: "flex" }}>
              <OrganizeMode totalPages={getScrollState().totalPages || 1} />
            </div>
          ) : (
            <>
              {children}

              {/* Comment mode fix: no separate error window, stays in workspace */}
              {mode === "comment" && (
                <div
                  style={{
                    position: "absolute",
                    top: 16,
                    right: 16,
                    width: 300,
                    zIndex: 50,
                  }}
                >
                  <CommentMode highlightColor={highlightColor} />
                </div>
              )}

              {mode === "edit" && (
                <div
                  style={{
                    position: "absolute",
                    top: 16,
                    right: 16,
                    zIndex: 50,
                  }}
                >
                  <EditMode />
                </div>
              )}
            </>
          )}
        </div>

        <RightUtilityPanel
          collapsed={rightCollapsed}
          onToggle={() => setRightCollapsed(!rightCollapsed)}
          page={getScrollState().currentPage}
          totalPages={getScrollState().totalPages || 1}
          onPageChange={handlePageChange}
          zoom={{
            percentage: getZoomState().zoomPercent,
            scale: getZoomState().currentZoom,
          }}
          mode={mode}
          searchQuery={searchQuery}
          searchResults={searchResults.map((r, i) => ({
            id: `${i}`,
            page: r.pageIndex + 1,
            preview: "Match on page " + (r.pageIndex + 1),
            active: i === searchIndex,
          }))}
          onSearchResultClick={(r) => searchActions.goToResult(parseInt(r.id))}
          bookmarks={[
            { id: "1", title: "Assignment 03", page: 1, level: 0 },
            { id: "2", title: "Objectives", page: 1, level: 1 },
            { id: "3", title: "Project Description", page: 2, level: 1 },
          ]}
          documentInfo={{
            title: activeTab?.name,
            pageCount: activeTab?.totalPages,
          }}
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
          padding: 0 14px;
          font-size: 12px;
          font-weight: 700;
          color: var(--text-primary);
          border-right: 1px solid var(--border);
          height: 100%;
          flex-shrink: 0;
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
        .comment-thread {
          display: flex;
          gap: 10px;
        }
        .ct-avatar {
          width: 28px;
          height: 28px;
          border-radius: 50%;
          background: var(--accent);
          color: var(--text-inverse);
          display: flex;
          align-items: center;
          justify-content: center;
          font-size: 11px;
          font-weight: 700;
          flex-shrink: 0;
        }
        .ct-author {
          font-size: 12px;
          font-weight: 600;
        }
        .ct-author span {
          font-weight: 400;
          color: var(--text-tertiary);
          margin-left: 6px;
        }
        .ct-text {
          font-size: 12px;
          color: var(--text-secondary);
          margin-top: 2px;
          line-height: 1.4;
        }
        .ann-item {
          display: flex;
          align-items: center;
          gap: 8px;
          padding: 8px;
          background: var(--surface-card);
          border-radius: 8px;
          font-size: 11px;
        }
        .ann-color {
          width: 12px;
          height: 12px;
          border-radius: 3px;
          flex-shrink: 0;
        }
      `}</style>
    </div>
  );
};
