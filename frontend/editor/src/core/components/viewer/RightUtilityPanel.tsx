/* eslint-disable */
import React, { useState, useEffect, useMemo, useCallback } from "react";
import {
  PanelRightClose,
  PanelRightOpen,
  ChevronLeft,
  ChevronRight,
  ChevronsLeft,
  ChevronsRight,
  FileText,
  Bookmark,
  Search,
  Info,
  Paperclip,
  Hash,
  Clock3,
  User,
  Layers,
  FileType2,
  ExternalLink,
} from "lucide-react";

// Types
export type BookmarkItem = {
  id: string;
  title: string;
  page: number;
  level?: number;
  children?: BookmarkItem[];
  active?: boolean;
};

export type SearchResultItem = {
  id: string;
  page: number;
  preview: string;
  matchStart?: number;
  matchEnd?: number;
};

export type DocumentInfoData = {
  title?: string;
  author?: string;
  subject?: string;
  keywords?: string;
  creator?: string;
  producer?: string;
  creationDate?: string;
  modDate?: string;
  pageSize?: string;
  pageCount?: number;
  fileSize?: string;
  version?: string;
  isLinearized?: boolean;
  permissions?: string[];
};

export type AttachmentItem = {
  id: string;
  name: string;
  size?: string;
  description?: string;
  type?: string;
};

export type RightPanelTab =
  | "pages"
  | "bookmarks"
  | "search"
  | "info"
  | "attachments";
export type RightPanelMode =
  | "view"
  | "comment"
  | "edit"
  | "organize"
  | "search"
  | string;

type ZoomProp = { percentage: number; scale: number } | number;

export type RightUtilityPanelProps = {
  collapsed: boolean;
  onToggle: () => void;
  page: number;
  totalPages: number;
  onPageChange?: (page: number) => void;
  zoom?: ZoomProp;
  mode?: RightPanelMode;
  bookmarks?: BookmarkItem[];
  searchQuery?: string;
  searchResults?: SearchResultItem[];
  searchActiveIndex?: number;
  onSearchResultClick?: (result: SearchResultItem, index: number) => void;
  documentInfo?: DocumentInfoData;
  attachments?: AttachmentItem[];
  defaultTab?: RightPanelTab;
  hidePageNavigation?: boolean;
  onBookmarkNavigate?: (bookmark: BookmarkItem) => void;
  onAttachmentOpen?: (attachment: AttachmentItem) => void;
  className?: string;
};

const getZoomPct = (zoom?: ZoomProp): number | undefined => {
  if (zoom == null) return undefined;
  if (typeof zoom === "number") return Math.round(zoom * 100);
  return zoom.percentage;
};

export const RightUtilityPanel: React.FC<RightUtilityPanelProps> = ({
  collapsed,
  onToggle,
  page,
  totalPages,
  onPageChange,
  zoom,
  mode,
  bookmarks = [],
  searchQuery,
  searchResults = [],
  searchActiveIndex = -1,
  onSearchResultClick,
  documentInfo,
  attachments = [],
  defaultTab = "pages",
  hidePageNavigation = false,
  onBookmarkNavigate,
  onAttachmentOpen,
}) => {
  const [activeTab, setActiveTab] = useState<RightPanelTab>(defaultTab);
  const [pageInput, setPageInput] = useState(String(page));
  const [expandedBookmarks, setExpandedBookmarks] = useState<Set<string>>(
    new Set(),
  );
  const [infoExpanded, setInfoExpanded] = useState(true);

  const zoomPct = useMemo(() => getZoomPct(zoom), [zoom]);
  const hasSearch = !!(searchQuery && searchQuery.trim().length > 0);
  const hasBookmarks = bookmarks.length > 0;
  const hasAttachments = attachments.length > 0;

  // Auto switch to search tab when query appears, progressive disclosure
  useEffect(() => {
    if (hasSearch && searchResults.length > 0) {
      setActiveTab("search");
    }
  }, [hasSearch, searchResults.length]);

  useEffect(() => {
    setPageInput(String(page));
  }, [page]);

  const commitPage = useCallback(() => {
    const n = parseInt(pageInput, 10);
    if (!Number.isNaN(n) && onPageChange) {
      const clamped = Math.min(totalPages, Math.max(1, n));
      onPageChange(clamped);
      setPageInput(String(clamped));
    } else {
      setPageInput(String(page));
    }
  }, [pageInput, totalPages, page, onPageChange]);

  const handlePageKey = (e: React.KeyboardEvent) => {
    if (e.key === "Enter") commitPage();
    if (e.key === "Escape") setPageInput(String(page));
  };

  const toggleBookmark = (id: string) => {
    setExpandedBookmarks((prev) => {
      const next = new Set(prev);
      if (next.has(id)) next.delete(id);
      else next.add(id);
      return next;
    });
  };

  const renderBookmarkTree = (
    items: BookmarkItem[],
    depth = 0,
  ): React.ReactNode => {
    return items.map((b) => {
      const hasChildren = !!(b.children && b.children.length > 0);
      const isExpanded = expandedBookmarks.has(b.id);
      const isActive = b.page === page || b.active;
      return (
        <div key={b.id} className="bm-node">
          <button
            className={`bm-row ${isActive ? "active" : ""}`}
            style={{ paddingLeft: `${12 + depth * 16}px` }}
            onClick={() => {
              if (onBookmarkNavigate) onBookmarkNavigate(b);
              else if (onPageChange) onPageChange(b.page);
            }}
            title={`${b.title} — Page ${b.page}`}
            aria-current={isActive ? "true" : undefined}
          >
            {hasChildren ? (
              <span
                className="bm-chevron"
                onClick={(e) => {
                  e.stopPropagation();
                  toggleBookmark(b.id);
                }}
                role="button"
                tabIndex={0}
                onKeyDown={(e) => {
                  if (e.key === "Enter" || e.key === " ") {
                    e.preventDefault();
                    e.stopPropagation();
                    toggleBookmark(b.id);
                  }
                }}
                aria-expanded={isExpanded}
              >
                <ChevronRight
                  size={12}
                  className={isExpanded ? "rotated" : ""}
                />
              </span>
            ) : (
              <span className="bm-chevron placeholder" />
            )}
            <span className="bm-title">{b.title}</span>
            <span className="bm-page">{b.page}</span>
          </button>
          {hasChildren && isExpanded && (
            <div className="bm-children">
              {renderBookmarkTree(b.children!, depth + 1)}
            </div>
          )}
        </div>
      );
    });
  };

  const tabs: {
    id: RightPanelTab;
    label: string;
    icon: React.ReactNode;
    count?: number;
    hidden?: boolean;
  }[] = [
    { id: "pages", label: "Pages", icon: <Hash size={14} /> },
    {
      id: "bookmarks",
      label: "Outline",
      icon: <Bookmark size={14} />,
      count: bookmarks.length,
      hidden: !hasBookmarks,
    },
    {
      id: "search",
      label: "Search",
      icon: <Search size={14} />,
      count: searchResults.length,
      hidden: !hasSearch,
    },
    {
      id: "attachments",
      label: "Files",
      icon: <Paperclip size={14} />,
      count: attachments.length,
      hidden: !hasAttachments,
    },
    { id: "info", label: "Info", icon: <Info size={14} /> },
  ];

  const visibleTabs = tabs.filter((t) => !t.hidden);

  // Collapsed state — PDF gains space
  if (collapsed) {
    return (
      <div
        className="rup-collapsed-rail"
        role="complementary"
        aria-label="Utility panel collapsed"
      >
        <button
          className="rup-expand-btn"
          onClick={onToggle}
          title="Expand utility panel"
          aria-label="Expand utility panel"
        >
          <PanelRightOpen size={16} />
        </button>
        <div className="rup-collapsed-divider" />
        <div className="rup-collapsed-icons">
          <button
            className="rup-collapsed-icon"
            onClick={() => {
              onToggle();
              setActiveTab("pages");
            }}
            title="Pages"
          >
            <Hash size={14} />
          </button>
          {hasBookmarks && (
            <button
              className="rup-collapsed-icon"
              onClick={() => {
                onToggle();
                setActiveTab("bookmarks");
              }}
              title="Bookmarks"
            >
              <Bookmark size={14} />
            </button>
          )}
          {hasSearch && (
            <button
              className="rup-collapsed-icon has-badge"
              onClick={() => {
                onToggle();
                setActiveTab("search");
              }}
              title={`Search results (${searchResults.length})`}
            >
              <Search size={14} />
              <span className="badge-dot" />
            </button>
          )}
          {hasAttachments && (
            <button
              className="rup-collapsed-icon has-badge"
              onClick={() => {
                onToggle();
                setActiveTab("attachments");
              }}
              title="Attachments"
            >
              <Paperclip size={14} />
            </button>
          )}
        </div>
        <style>{`
          .rup-collapsed-rail {
            width: 36px;
            background: var(--viewer-right-rail-bg, var(--app-bg, #1e2130));
            border-left: 1px solid var(--border, #2f334a);
            display: flex;
            flex-direction: column;
            align-items: center;
            padding: 10px 0;
            gap: 8px;
            flex-shrink: 0;
            height: 100%;
          }
          .rup-expand-btn {
            width: 28px;
            height: 28px;
            border-radius: 8px;
            border: 1px solid var(--border, #2f334a);
            background: var(--surface-elevated, #2a2d3f);
            color: var(--text-secondary, #9aa0b8);
            display: flex;
            align-items: center;
            justify-content: center;
            cursor: pointer;
            transition: all 150ms var(--ease-out, cubic-bezier(0,0,0.2,1));
          }
          .rup-expand-btn:hover {
            background: var(--surface-hover, #343a56);
            color: var(--text-primary, #e8e9f0);
          }
          .rup-collapsed-divider {
            width: 20px;
            height: 1px;
            background: var(--border, #2f334a);
            margin: 4px 0;
          }
          .rup-collapsed-icons {
            display: flex;
            flex-direction: column;
            gap: 6px;
          }
          .rup-collapsed-icon {
            width: 28px;
            height: 28px;
            border-radius: 8px;
            border: none;
            background: transparent;
            color: var(--text-tertiary, #6b7190);
            display: flex;
            align-items: center;
            justify-content: center;
            cursor: pointer;
            position: relative;
            transition: all 150ms var(--ease-out, cubic-bezier(0,0,0.2,1));
          }
          .rup-collapsed-icon:hover {
            background: var(--surface-hover, #343a56);
            color: var(--text-primary, #e8e9f0);
          }
          .rup-collapsed-icon.has-badge .badge-dot {
            position: absolute;
            top: 4px;
            right: 5px;
            width: 6px;
            height: 6px;
            border-radius: 50%;
            background: var(--accent, #79AEFF);
          }
        `}</style>
      </div>
    );
  }

  return (
    <aside
      className="right-utility-panel"
      role="complementary"
      aria-label="Viewer utilities"
    >
      {/* Header */}
      <div className="rup-header">
        <div className="rup-header-left">
          <h2 className="rup-title">Inspector</h2>
          {zoomPct != null && <span className="rup-zoom-pill">{zoomPct}%</span>}
          {mode && mode !== "view" && (
            <span className="rup-mode-pill">{mode}</span>
          )}
        </div>
        <div className="rup-header-actions">
          <button
            className="rup-icon-btn"
            onClick={onToggle}
            title="Collapse panel (PDF gains space)"
            aria-label="Collapse utility panel"
          >
            <PanelRightClose size={16} />
          </button>
        </div>
      </div>

      {/* Tabs */}
      <div className="rup-tabs" role="tablist" aria-label="Utility sections">
        {visibleTabs.map((t) => (
          <button
            key={t.id}
            role="tab"
            aria-selected={activeTab === t.id}
            className={`rup-tab ${activeTab === t.id ? "active" : ""}`}
            onClick={() => setActiveTab(t.id)}
            title={t.label}
          >
            <span className="rup-tab-icon">{t.icon}</span>
            <span className="rup-tab-label">{t.label}</span>
            {typeof t.count === "number" && t.count > 0 && (
              <span className="rup-tab-count">
                {t.count > 99 ? "99+" : t.count}
              </span>
            )}
          </button>
        ))}
      </div>

      {/* Content */}
      <div className="rup-content">
        {/* PAGES */}
        {activeTab === "pages" && (
          <div className="rup-section" data-section="pages">
            {!hidePageNavigation ? (
              <>
                <div className="rup-section-header">
                  <span className="rup-section-label">Page Navigation</span>
                  <span className="rup-section-meta">
                    {page} / {totalPages}
                  </span>
                </div>

                <div className="rup-page-control">
                  <div className="rup-page-row">
                    <button
                      className="rup-nav-btn"
                      onClick={() => onPageChange?.(1)}
                      disabled={page <= 1}
                      title="First page (Home)"
                      aria-label="First page"
                    >
                      <ChevronsLeft size={16} />
                    </button>
                    <button
                      className="rup-nav-btn"
                      onClick={() => onPageChange?.(Math.max(1, page - 1))}
                      disabled={page <= 1}
                      title="Previous page"
                      aria-label="Previous page"
                    >
                      <ChevronLeft size={16} />
                    </button>

                    <div className="rup-page-input-group">
                      <input
                        className="rup-page-input"
                        value={pageInput}
                        onChange={(e) =>
                          setPageInput(e.target.value.replace(/[^0-9]/g, ""))
                        }
                        onBlur={commitPage}
                        onKeyDown={handlePageKey}
                        aria-label="Current page number"
                        inputMode="numeric"
                        pattern="[0-9]*"
                      />
                      <span className="rup-page-sep">/</span>
                      <span className="rup-page-total">{totalPages}</span>
                    </div>

                    <button
                      className="rup-nav-btn"
                      onClick={() =>
                        onPageChange?.(Math.min(totalPages, page + 1))
                      }
                      disabled={page >= totalPages}
                      title="Next page"
                      aria-label="Next page"
                    >
                      <ChevronRight size={16} />
                    </button>
                    <button
                      className="rup-nav-btn"
                      onClick={() => onPageChange?.(totalPages)}
                      disabled={page >= totalPages}
                      title="Last page (End)"
                      aria-label="Last page"
                    >
                      <ChevronsRight size={16} />
                    </button>
                  </div>

                  <div className="rup-page-slider-wrap">
                    <input
                      type="range"
                      min={1}
                      max={totalPages}
                      value={page}
                      onChange={(e) =>
                        onPageChange?.(parseInt(e.target.value, 10))
                      }
                      className="rup-page-slider"
                      aria-label="Scrub through pages"
                    />
                    <div className="rup-slider-labels">
                      <span>1</span>
                      <span>{totalPages}</span>
                    </div>
                  </div>
                </div>

                <div className="rup-divider" />
              </>
            ) : (
              <div className="rup-muted-note">
                <FileText size={14} />
                <span>
                  Page navigation is shown in the main toolbar to keep this
                  panel focused.
                </span>
              </div>
            )}

            <div className="rup-mini-stats">
              <div className="rup-stat">
                <Layers size={12} />
                <span className="rup-stat-label">Pages</span>
                <span className="rup-stat-value">{totalPages}</span>
              </div>
              {zoomPct != null && (
                <div className="rup-stat">
                  <Search size={12} />
                  <span className="rup-stat-label">Zoom</span>
                  <span className="rup-stat-value">{zoomPct}%</span>
                </div>
              )}
              {documentInfo?.fileSize && (
                <div className="rup-stat">
                  <FileType2 size={12} />
                  <span className="rup-stat-label">Size</span>
                  <span className="rup-stat-value">
                    {documentInfo.fileSize}
                  </span>
                </div>
              )}
            </div>

            {/* Quick outline preview when bookmarks exist */}
            {hasBookmarks && (
              <>
                <div className="rup-section-header sm">
                  <span className="rup-section-label">On this document</span>
                  <button
                    className="rup-link-btn"
                    onClick={() => setActiveTab("bookmarks")}
                  >
                    View outline
                  </button>
                </div>
                <div className="rup-compact-bookmarks">
                  {bookmarks.slice(0, 6).map((b) => (
                    <button
                      key={b.id}
                      className="rup-compact-bm"
                      onClick={() =>
                        onBookmarkNavigate
                          ? onBookmarkNavigate(b)
                          : onPageChange?.(b.page)
                      }
                    >
                      <span className="rup-compact-bm-title">{b.title}</span>
                      <span className="rup-compact-bm-page">{b.page}</span>
                    </button>
                  ))}
                  {bookmarks.length > 6 && (
                    <div className="rup-more-hint">
                      +{bookmarks.length - 6} more sections
                    </div>
                  )}
                </div>
              </>
            )}
          </div>
        )}

        {/* BOOKMARKS */}
        {activeTab === "bookmarks" && (
          <div className="rup-section" data-section="bookmarks">
            <div className="rup-section-header">
              <span className="rup-section-label">Outline</span>
              <span className="rup-section-meta">{bookmarks.length} items</span>
            </div>
            {hasBookmarks ? (
              <div
                className="rup-bookmarks-list"
                role="tree"
                aria-label="Document outline"
              >
                {renderBookmarkTree(bookmarks)}
              </div>
            ) : (
              <div className="rup-empty">
                <Bookmark size={20} />
                <p className="rup-empty-title">No outline</p>
                <p className="rup-empty-desc">
                  This document has no bookmarks or table of contents.
                </p>
              </div>
            )}
          </div>
        )}

        {/* SEARCH */}
        {activeTab === "search" && (
          <div className="rup-section" data-section="search">
            <div className="rup-section-header">
              <div className="rup-section-header-left">
                <span className="rup-section-label">Find results</span>
                {searchQuery && (
                  <span className="rup-search-q">“{searchQuery}”</span>
                )}
              </div>
              <span className="rup-section-meta">
                {searchResults.length > 0
                  ? `${searchActiveIndex + 1 >= 1 ? searchActiveIndex + 1 : 1} of ${searchResults.length}`
                  : "No results"}
              </span>
            </div>

            {searchResults.length > 0 ? (
              <div className="rup-search-list">
                {searchResults.map((r, idx) => {
                  const isActive = idx === searchActiveIndex;
                  return (
                    <button
                      key={r.id}
                      className={`rup-search-item ${isActive ? "active" : ""}`}
                      onClick={() => onSearchResultClick?.(r, idx)}
                      title={`Go to page ${r.page}`}
                    >
                      <div className="rup-search-item-top">
                        <span className="rup-search-page-badge">
                          <FileText size={10} />
                          {r.page}
                        </span>
                        {isActive && (
                          <span className="rup-search-current-dot" />
                        )}
                      </div>
                      <div className="rup-search-preview">
                        {r.matchStart != null && r.matchEnd != null ? (
                          <>
                            {r.preview.slice(0, r.matchStart)}
                            <mark>
                              {r.preview.slice(r.matchStart, r.matchEnd)}
                            </mark>
                            {r.preview.slice(r.matchEnd)}
                          </>
                        ) : (
                          r.preview
                        )}
                      </div>
                    </button>
                  );
                })}
              </div>
            ) : (
              <div className="rup-empty">
                <Search size={20} />
                <p className="rup-empty-title">
                  {searchQuery ? "No matches" : "No search"}
                </p>
                <p className="rup-empty-desc">
                  {searchQuery
                    ? `No results for “${searchQuery}”.`
                    : "Use Ctrl+F to search this document."}
                </p>
              </div>
            )}
          </div>
        )}

        {/* ATTACHMENTS */}
        {activeTab === "attachments" && (
          <div className="rup-section" data-section="attachments">
            <div className="rup-section-header">
              <span className="rup-section-label">Attachments</span>
              <span className="rup-section-meta">{attachments.length}</span>
            </div>
            {hasAttachments ? (
              <div className="rup-attach-list">
                {attachments.map((a) => (
                  <div key={a.id} className="rup-attach-item">
                    <div className="rup-attach-icon">
                      <Paperclip size={14} />
                    </div>
                    <div className="rup-attach-main">
                      <span className="rup-attach-name" title={a.name}>
                        {a.name}
                      </span>
                      <span className="rup-attach-meta">
                        {a.type ? `${a.type}` : "File"}
                        {a.size ? ` • ${a.size}` : ""}
                      </span>
                      {a.description && (
                        <span className="rup-attach-desc">{a.description}</span>
                      )}
                    </div>
                    <button
                      className="rup-attach-action"
                      onClick={() => onAttachmentOpen?.(a)}
                      title={`Open ${a.name}`}
                      aria-label={`Open ${a.name}`}
                    >
                      <ExternalLink size={14} />
                    </button>
                  </div>
                ))}
              </div>
            ) : (
              <div className="rup-empty">
                <Paperclip size={20} />
                <p className="rup-empty-title">No attachments</p>
                <p className="rup-empty-desc">
                  Embedded files will appear here.
                </p>
              </div>
            )}
          </div>
        )}

        {/* INFO */}
        {activeTab === "info" && (
          <div className="rup-section" data-section="info">
            <div
              className="rup-section-header clickable"
              onClick={() => setInfoExpanded((v) => !v)}
              role="button"
              tabIndex={0}
              onKeyDown={(e) => {
                if (e.key === "Enter" || e.key === " ") {
                  e.preventDefault();
                  setInfoExpanded((v) => !v);
                }
              }}
              aria-expanded={infoExpanded}
            >
              <span className="rup-section-label">Document Information</span>
              <ChevronRight
                size={14}
                className={infoExpanded ? "rotated-90" : ""}
              />
            </div>

            {infoExpanded ? (
              <div className="rup-info-grid">
                {[
                  {
                    label: "Title",
                    value: documentInfo?.title,
                    icon: <FileText size={12} />,
                  },
                  {
                    label: "Author",
                    value: documentInfo?.author,
                    icon: <User size={12} />,
                  },
                  {
                    label: "Subject",
                    value: documentInfo?.subject,
                    icon: <Info size={12} />,
                  },
                  {
                    label: "Keywords",
                    value: documentInfo?.keywords,
                    icon: <Hash size={12} />,
                  },
                  { label: "Creator", value: documentInfo?.creator },
                  { label: "Producer", value: documentInfo?.producer },
                  {
                    label: "Created",
                    value: documentInfo?.creationDate,
                    icon: <Clock3 size={12} />,
                  },
                  {
                    label: "Modified",
                    value: documentInfo?.modDate,
                    icon: <Clock3 size={12} />,
                  },
                  { label: "Page size", value: documentInfo?.pageSize },
                  {
                    label: "Pages",
                    value: documentInfo?.pageCount ?? totalPages,
                    icon: <Layers size={12} />,
                  },
                  { label: "File size", value: documentInfo?.fileSize },
                  { label: "PDF version", value: documentInfo?.version },
                  {
                    label: "Fast web view",
                    value:
                      documentInfo?.isLinearized != null
                        ? documentInfo.isLinearized
                          ? "Yes"
                          : "No"
                        : undefined,
                  },
                ]
                  .filter(
                    (f) => f.value != null && String(f.value).trim() !== "",
                  )
                  .map((f) => (
                    <div key={f.label} className="rup-info-row">
                      <div className="rup-info-label">
                        {f.icon && (
                          <span className="rup-info-icon">{f.icon}</span>
                        )}
                        {f.label}
                      </div>
                      <div className="rup-info-value" title={String(f.value)}>
                        {String(f.value)}
                      </div>
                    </div>
                  ))}

                {(!documentInfo || Object.keys(documentInfo).length === 0) && (
                  <div className="rup-info-row">
                    <div className="rup-info-label">Pages</div>
                    <div className="rup-info-value">{totalPages}</div>
                  </div>
                )}

                {documentInfo?.permissions &&
                  documentInfo.permissions.length > 0 && (
                    <>
                      <div className="rup-divider" />
                      <div className="rup-section-header sm">
                        <span className="rup-section-label">Permissions</span>
                      </div>
                      <div className="rup-perm-list">
                        {documentInfo.permissions.map((p) => (
                          <span key={p} className="rup-perm-pill">
                            {p}
                          </span>
                        ))}
                      </div>
                    </>
                  )}
              </div>
            ) : (
              <div className="rup-muted-note">
                <Info size={14} />
                <span>Metadata hidden to save space.</span>
              </div>
            )}

            <div className="rup-divider" />
            <div className="rup-actions-footer">
              <button
                className="rup-footer-btn"
                title="Copy document info"
                onClick={() => {
                  if (!documentInfo) return;
                  const text = Object.entries(documentInfo)
                    .map(([k, v]) => `${k}: ${v}`)
                    .join("\n");
                  navigator.clipboard?.writeText(text);
                }}
              >
                <FileType2 size={14} />
                Copy info
              </button>
            </div>
          </div>
        )}
      </div>

      {/* Footer / status */}
      <div className="rup-footer">
        <span className="rup-footer-page">
          Page {page} of {totalPages}
        </span>
        <div className="rup-footer-right">
          {hasSearch && (
            <span
              className="rup-footer-dot"
              title={`${searchResults.length} results`}
            />
          )}
          {hasAttachments && (
            <span title={`${attachments.length} attachments`}>
              <Paperclip size={12} />
            </span>
          )}
        </div>
      </div>

      <style>{`
        .right-utility-panel {
          width: 320px;
          min-width: 320px;
          max-width: 320px;
          height: 100%;
          background: var(--viewer-right-rail-bg, var(--surface-elevated, #252836));
          border-left: 1px solid var(--border, #2f334a);
          display: flex;
          flex-direction: column;
          flex-shrink: 0;
          overflow: hidden;
          font-family: var(--font-sans, "Inter", "Segoe UI", sans-serif);
          color: var(--text-primary, #e8e9f0);
          user-select: none;
        }

        /* Header */
        .rup-header {
          height: 44px;
          min-height: 44px;
          padding: 0 12px;
          display: flex;
          align-items: center;
          justify-content: space-between;
          border-bottom: 1px solid var(--border, #2f334a);
          background: var(--surface-elevated, #252836);
          gap: 8px;
        }
        .rup-header-left {
          display: flex;
          align-items: center;
          gap: 8px;
          min-width: 0;
        }
        .rup-title {
          font-size: 12px;
          font-weight: 600;
          letter-spacing: 0.04em;
          text-transform: uppercase;
          color: var(--text-primary, #e8e9f0);
          margin: 0;
          line-height: 1;
        }
        .rup-zoom-pill, .rup-mode-pill {
          font-size: 10px;
          font-weight: 500;
          padding: 2px 6px;
          border-radius: 9999px;
          background: var(--surface-card, #2f334a);
          border: 1px solid var(--border, #2f334a);
          color: var(--text-secondary, #9aa0b8);
          font-variant-numeric: tabular-nums;
          line-height: 1;
        }
        .rup-mode-pill {
          background: var(--accent-subtle, rgba(121,174,255,0.12));
          color: var(--accent, #79AEFF);
          border-color: var(--accent-strong, rgba(121,174,255,0.2));
          text-transform: capitalize;
        }
        .rup-header-actions {
          display: flex;
          align-items: center;
          gap: 4px;
        }
        .rup-icon-btn {
          width: 28px;
          height: 28px;
          border-radius: 8px;
          border: none;
          background: transparent;
          color: var(--text-secondary, #9aa0b8);
          display: flex;
          align-items: center;
          justify-content: center;
          cursor: pointer;
          transition: all 150ms var(--ease-out, cubic-bezier(0,0,0.2,1));
        }
        .rup-icon-btn:hover {
          background: var(--surface-hover, #343a56);
          color: var(--text-primary, #e8e9f0);
        }
        .rup-icon-btn:focus-visible {
          outline: 2px solid var(--focus-ring, #79AEFF);
          outline-offset: 2px;
        }

        /* Tabs */
        .rup-tabs {
          display: flex;
          align-items: center;
          gap: 2px;
          padding: 8px 8px 0 8px;
          flex-shrink: 0;
          overflow-x: auto;
          scrollbar-width: none;
        }
        .rup-tabs::-webkit-scrollbar { display: none; }
        .rup-tab {
          height: 32px;
          padding: 0 10px;
          border-radius: 8px;
          border: 1px solid transparent;
          background: transparent;
          color: var(--text-secondary, #9aa0b8);
          display: flex;
          align-items: center;
          gap: 6px;
          font-size: 12px;
          font-weight: 500;
          white-space: nowrap;
          cursor: pointer;
          transition: all 120ms var(--ease-out, cubic-bezier(0,0,0.2,1));
          flex-shrink: 0;
        }
        .rup-tab:hover {
          background: var(--surface-hover, #343a56);
          color: var(--text-primary, #e8e9f0);
        }
        .rup-tab.active {
          background: var(--surface-selected, #3a4064);
          border-color: var(--border-strong, #3a4060);
          color: var(--text-primary, #e8e9f0);
        }
        .rup-tab.active .rup-tab-icon {
          color: var(--accent, #79AEFF);
        }
        .rup-tab-icon { display: flex; align-items: center; }
        .rup-tab-label { line-height: 1; }
        .rup-tab-count {
          font-size: 10px;
          font-weight: 600;
          min-width: 16px;
          height: 16px;
          padding: 0 4px;
          border-radius: 9999px;
          background: var(--surface-card, #2f334a);
          border: 1px solid var(--border, #2f334a);
          display: inline-flex;
          align-items: center;
          justify-content: center;
          color: var(--text-secondary, #9aa0b8);
          font-variant-numeric: tabular-nums;
        }
        .rup-tab.active .rup-tab-count {
          background: var(--accent-subtle, rgba(121,174,255,0.12));
          color: var(--accent, #79AEFF);
          border-color: var(--accent-strong, rgba(121,174,255,0.2));
        }

        /* Content */
        .rup-content {
          flex: 1;
          overflow-y: auto;
          overflow-x: hidden;
          padding: 12px 0;
          display: flex;
          flex-direction: column;
          gap: 0;
        }
        .rup-content::-webkit-scrollbar { width: 6px; }
        .rup-content::-webkit-scrollbar-thumb { background: var(--scrollbar, #3a4060); border-radius: 9999px; }

        .rup-section {
          padding: 0 12px;
          display: flex;
          flex-direction: column;
          gap: 12px;
          animation: rupFadeIn 150ms var(--ease-out, cubic-bezier(0,0,0.2,1));
        }
        @keyframes rupFadeIn {
          from { opacity: 0; transform: translateY(2px); }
          to { opacity: 1; transform: translateY(0); }
        }
        .rup-section-header {
          display: flex;
          align-items: center;
          justify-content: space-between;
          gap: 8px;
          padding: 0 2px;
        }
        .rup-section-header.sm { margin-top: 4px; }
        .rup-section-header.clickable { cursor: pointer; border-radius: 6px; padding: 4px 2px; margin: -4px -2px; }
        .rup-section-header.clickable:hover { background: var(--surface-hover, #343a56); }
        .rup-section-label {
          font-size: 11px;
          font-weight: 600;
          letter-spacing: 0.04em;
          text-transform: uppercase;
          color: var(--text-secondary, #9aa0b8);
          line-height: 1.2;
        }
        .rup-section-meta {
          font-size: 11px;
          font-weight: 500;
          color: var(--text-tertiary, #6b7190);
          font-variant-numeric: tabular-nums;
          white-space: nowrap;
        }
        .rup-section-header-left {
          display: flex;
          align-items: center;
          gap: 8px;
          min-width: 0;
        }
        .rup-search-q {
          font-size: 11px;
          color: var(--text-primary, #e8e9f0);
          font-style: italic;
          overflow: hidden;
          text-overflow: ellipsis;
          white-space: nowrap;
          max-width: 140px;
        }
        .rup-link-btn {
          font-size: 11px;
          font-weight: 500;
          color: var(--accent, #79AEFF);
          background: transparent;
          border: none;
          cursor: pointer;
          padding: 0;
        }
        .rup-link-btn:hover { text-decoration: underline; }

        /* Page navigation */
        .rup-page-control {
          background: var(--surface-card, #2f334a);
          border: 1px solid var(--border, #2f334a);
          border-radius: 12px;
          padding: 10px;
          display: flex;
          flex-direction: column;
          gap: 10px;
        }
        .rup-page-row {
          display: flex;
          align-items: center;
          gap: 4px;
        }
        .rup-nav-btn {
          width: 28px;
          height: 28px;
          border-radius: 8px;
          border: 1px solid var(--border, #2f334a);
          background: var(--surface-elevated, #2a2d3f);
          color: var(--text-secondary, #9aa0b8);
          display: flex;
          align-items: center;
          justify-content: center;
          cursor: pointer;
          transition: all 120ms var(--ease-out, cubic-bezier(0,0,0.2,1));
          flex-shrink: 0;
        }
        .rup-nav-btn:hover:not(:disabled) {
          background: var(--surface-hover, #343a56);
          color: var(--text-primary, #e8e9f0);
          border-color: var(--border-strong, #3a4060);
        }
        .rup-nav-btn:disabled {
          opacity: 0.4;
          cursor: default;
        }
        .rup-nav-btn:focus-visible {
          outline: 2px solid var(--focus-ring, #79AEFF);
          outline-offset: 1px;
        }
        .rup-page-input-group {
          flex: 1;
          display: flex;
          align-items: center;
          justify-content: center;
          gap: 6px;
          background: var(--app-bg, #1e2130);
          border: 1px solid var(--border, #2f334a);
          border-radius: 8px;
          height: 28px;
          padding: 0 8px;
          min-width: 0;
        }
        .rup-page-input {
          width: 36px;
          background: transparent;
          border: none;
          outline: none;
          color: var(--text-primary, #e8e9f0);
          font-size: 12px;
          font-weight: 600;
          text-align: center;
          font-variant-numeric: tabular-nums;
        }
        .rup-page-sep, .rup-page-total {
          font-size: 11px;
          color: var(--text-tertiary, #6b7190);
          font-variant-numeric: tabular-nums;
        }
        .rup-page-total { font-weight: 500; min-width: 24px; }
        .rup-page-slider-wrap {
          display: flex;
          flex-direction: column;
          gap: 4px;
        }
        .rup-page-slider {
          width: 100%;
          height: 4px;
          accent-color: var(--accent, #79AEFF);
          cursor: pointer;
          margin: 0;
        }
        .rup-slider-labels {
          display: flex;
          justify-content: space-between;
          font-size: 10px;
          color: var(--text-tertiary, #6b7190);
          font-variant-numeric: tabular-nums;
          padding: 0 2px;
        }
        .rup-divider {
          height: 1px;
          background: var(--border, #2f334a);
          margin: 4px 0;
        }
        .rup-mini-stats {
          display: flex;
          gap: 8px;
          flex-wrap: wrap;
        }
        .rup-stat {
          display: flex;
          align-items: center;
          gap: 6px;
          background: var(--surface-elevated, #2a2d3f);
          border: 1px solid var(--border, #2f334a);
          border-radius: 9999px;
          padding: 4px 8px;
          font-size: 11px;
        }
        .rup-stat-label { color: var(--text-tertiary, #6b7190); }
        .rup-stat-value { color: var(--text-primary, #e8e9f0); font-weight: 600; font-variant-numeric: tabular-nums; }

        .rup-muted-note {
          display: flex;
          gap: 8px;
          align-items: flex-start;
          background: var(--surface-elevated, #2a2d3f);
          border: 1px dashed var(--border, #2f334a);
          border-radius: 8px;
          padding: 8px 10px;
          font-size: 11px;
          line-height: 1.4;
          color: var(--text-tertiary, #6b7190);
        }
        .rup-compact-bookmarks {
          display: flex;
          flex-direction: column;
          gap: 2px;
        }
        .rup-compact-bm {
          display: flex;
          align-items: center;
          justify-content: space-between;
          gap: 8px;
          padding: 6px 8px;
          border-radius: 8px;
          border: none;
          background: transparent;
          color: var(--text-secondary, #9aa0b8);
          font-size: 12px;
          text-align: left;
          cursor: pointer;
          transition: all 100ms var(--ease-out, cubic-bezier(0,0,0.2,1));
        }
        .rup-compact-bm:hover {
          background: var(--surface-hover, #343a56);
          color: var(--text-primary, #e8e9f0);
        }
        .rup-compact-bm-title { flex: 1; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
        .rup-compact-bm-page { font-size: 10px; color: var(--text-tertiary, #6b7190); font-variant-numeric: tabular-nums; }
        .rup-more-hint { font-size: 11px; color: var(--text-tertiary, #6b7190); padding: 2px 8px; }

        /* Bookmarks */
        .rup-bookmarks-list {
          display: flex;
          flex-direction: column;
          gap: 1px;
        }
        .bm-node { display: flex; flex-direction: column; }
        .bm-row {
          display: flex;
          align-items: center;
          gap: 6px;
          width: 100%;
          min-height: 30px;
          padding: 6px 8px 6px 12px;
          border-radius: 8px;
          border: none;
          background: transparent;
          color: var(--text-secondary, #9aa0b8);
          text-align: left;
          cursor: pointer;
          transition: all 100ms var(--ease-out, cubic-bezier(0,0,0.2,1));
        }
        .bm-row:hover { background: var(--surface-hover, #343a56); color: var(--text-primary, #e8e9f0); }
        .bm-row.active { background: var(--accent-subtle, rgba(121,174,255,0.12)); color: var(--text-primary, #e8e9f0); }
        .bm-row.active .bm-title { font-weight: 600; }
        .bm-chevron {
          width: 16px;
          height: 16px;
          display: flex;
          align-items: center;
          justify-content: center;
          border-radius: 4px;
          flex-shrink: 0;
          cursor: pointer;
        }
        .bm-chevron:hover { background: var(--surface-selected, #3a4064); }
        .bm-chevron.placeholder { width: 16px; }
        .bm-chevron .rotated { transform: rotate(90deg); }
        .bm-title {
          flex: 1;
          font-size: 12px;
          line-height: 1.35;
          overflow: hidden;
          text-overflow: ellipsis;
          white-space: nowrap;
        }
        .bm-page {
          font-size: 10px;
          color: var(--text-tertiary, #6b7190);
          font-variant-numeric: tabular-nums;
          flex-shrink: 0;
          background: var(--surface-elevated, #2a2d3f);
          border: 1px solid var(--border, #2f334a);
          border-radius: 6px;
          padding: 1px 5px;
          min-width: 20px;
          text-align: center;
        }
        .bm-row.active .bm-page {
          background: var(--accent-strong, rgba(121,174,255,0.2));
          color: var(--accent, #79AEFF);
          border-color: transparent;
        }
        .bm-children { display: flex; flex-direction: column; gap: 1px; }

        /* Search */
        .rup-search-list {
          display: flex;
          flex-direction: column;
          gap: 6px;
        }
        .rup-search-item {
          display: flex;
          flex-direction: column;
          gap: 6px;
          padding: 8px 10px;
          border-radius: 10px;
          border: 1px solid var(--border, #2f334a);
          background: var(--surface-card, #2f334a);
          text-align: left;
          cursor: pointer;
          transition: all 120ms var(--ease-out, cubic-bezier(0,0,0.2,1));
        }
        .rup-search-item:hover {
          background: var(--surface-hover, #343a56);
          border-color: var(--border-strong, #3a4060);
        }
        .rup-search-item.active {
          background: var(--accent-subtle, rgba(121,174,255,0.12));
          border-color: var(--accent-strong, rgba(121,174,255,0.2));
        }
        .rup-search-item-top {
          display: flex;
          align-items: center;
          justify-content: space-between;
        }
        .rup-search-page-badge {
          display: inline-flex;
          align-items: center;
          gap: 4px;
          font-size: 10px;
          font-weight: 600;
          color: var(--text-tertiary, #6b7190);
          background: var(--surface-elevated, #2a2d3f);
          border: 1px solid var(--border, #2f334a);
          border-radius: 6px;
          padding: 2px 6px;
          font-variant-numeric: tabular-nums;
        }
        .rup-search-item.active .rup-search-page-badge {
          background: var(--accent-strong, rgba(121,174,255,0.2));
          color: var(--accent, #79AEFF);
          border-color: transparent;
        }
        .rup-search-current-dot {
          width: 6px;
          height: 6px;
          border-radius: 50%;
          background: var(--accent, #79AEFF);
        }
        .rup-search-preview {
          font-size: 12px;
          line-height: 1.45;
          color: var(--text-secondary, #9aa0b8);
          display: -webkit-box;
          -webkit-line-clamp: 3;
          -webkit-box-orient: vertical;
          overflow: hidden;
        }
        .rup-search-preview mark {
          background: var(--accent-subtle, rgba(121,174,255,0.25));
          color: var(--text-primary, #e8e9f0);
          border-radius: 3px;
          padding: 0 2px;
        }

        /* Attachments */
        .rup-attach-list {
          display: flex;
          flex-direction: column;
          gap: 6px;
        }
        .rup-attach-item {
          display: flex;
          gap: 10px;
          align-items: flex-start;
          padding: 10px;
          border-radius: 10px;
          border: 1px solid var(--border, #2f334a);
          background: var(--surface-card, #2f334a);
          transition: all 120ms var(--ease-out, cubic-bezier(0,0,0.2,1));
        }
        .rup-attach-item:hover {
          background: var(--surface-hover, #343a56);
          border-color: var(--border-strong, #3a4060);
        }
        .rup-attach-icon {
          width: 28px;
          height: 28px;
          border-radius: 8px;
          background: var(--surface-elevated, #2a2d3f);
          border: 1px solid var(--border, #2f334a);
          display: flex;
          align-items: center;
          justify-content: center;
          color: var(--text-secondary, #9aa0b8);
          flex-shrink: 0;
        }
        .rup-attach-main {
          flex: 1;
          min-width: 0;
          display: flex;
          flex-direction: column;
          gap: 2px;
        }
        .rup-attach-name {
          font-size: 12px;
          font-weight: 500;
          color: var(--text-primary, #e8e9f0);
          overflow: hidden;
          text-overflow: ellipsis;
          white-space: nowrap;
        }
        .rup-attach-meta {
          font-size: 10px;
          color: var(--text-tertiary, #6b7190);
          font-variant-numeric: tabular-nums;
        }
        .rup-attach-desc {
          font-size: 11px;
          color: var(--text-tertiary, #6b7190);
          line-height: 1.3;
          margin-top: 2px;
        }
        .rup-attach-action {
          width: 26px;
          height: 26px;
          border-radius: 8px;
          border: 1px solid transparent;
          background: transparent;
          color: var(--text-tertiary, #6b7190);
          display: flex;
          align-items: center;
          justify-content: center;
          cursor: pointer;
          flex-shrink: 0;
          transition: all 120ms var(--ease-out, cubic-bezier(0,0,0.2,1));
        }
        .rup-attach-action:hover {
          background: var(--surface-elevated, #2a2d3f);
          color: var(--text-primary, #e8e9f0);
          border-color: var(--border, #2f334a);
        }

        /* Info */
        .rup-info-grid {
          display: flex;
          flex-direction: column;
          gap: 1px;
          background: var(--border, #2f334a);
          border: 1px solid var(--border, #2f334a);
          border-radius: 10px;
          overflow: hidden;
        }
        .rup-info-row {
          display: flex;
          flex-direction: column;
          gap: 2px;
          padding: 8px 10px;
          background: var(--surface-card, #2f334a);
        }
        .rup-info-row:nth-child(even) {
          background: var(--surface-elevated, #2a2d3f);
        }
        .rup-info-label {
          display: flex;
          align-items: center;
          gap: 6px;
          font-size: 10px;
          font-weight: 600;
          letter-spacing: 0.03em;
          text-transform: uppercase;
          color: var(--text-tertiary, #6b7190);
          line-height: 1;
        }
        .rup-info-icon { display: flex; opacity: 0.8; }
        .rup-info-value {
          font-size: 12px;
          color: var(--text-primary, #e8e9f0);
          line-height: 1.4;
          word-break: break-word;
        }
        .rup-perm-list {
          display: flex;
          flex-wrap: wrap;
          gap: 6px;
          padding: 0 2px 4px 2px;
        }
        .rup-perm-pill {
          font-size: 10px;
          padding: 3px 7px;
          border-radius: 9999px;
          background: var(--surface-card, #2f334a);
          border: 1px solid var(--border, #2f334a);
          color: var(--text-secondary, #9aa0b8);
        }
        .rup-actions-footer {
          display: flex;
          gap: 6px;
          padding: 0 2px;
        }
        .rup-footer-btn {
          height: 28px;
          padding: 0 10px;
          border-radius: 8px;
          border: 1px solid var(--border, #2f334a);
          background: var(--surface-elevated, #2a2d3f);
          color: var(--text-secondary, #9aa0b8);
          font-size: 11px;
          font-weight: 500;
          display: inline-flex;
          align-items: center;
          gap: 6px;
          cursor: pointer;
          transition: all 120ms var(--ease-out, cubic-bezier(0,0,0.2,1));
        }
        .rup-footer-btn:hover {
          background: var(--surface-hover, #343a56);
          color: var(--text-primary, #e8e9f0);
        }

        /* Empty states */
        .rup-empty {
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          text-align: center;
          gap: 8px;
          padding: 32px 16px;
          color: var(--text-tertiary, #6b7190);
        }
        .rup-empty-title {
          font-size: 13px;
          font-weight: 600;
          color: var(--text-secondary, #9aa0b8);
          margin: 0;
        }
        .rup-empty-desc {
          font-size: 11px;
          line-height: 1.4;
          margin: 0;
          max-width: 220px;
        }

        /* Footer */
        .rup-footer {
          height: 32px;
          min-height: 32px;
          padding: 0 12px;
          display: flex;
          align-items: center;
          justify-content: space-between;
          border-top: 1px solid var(--border, #2f334a);
          background: var(--surface-elevated, #2a2d3f);
          font-size: 10px;
          color: var(--text-tertiary, #6b7190);
        }
        .rup-footer-page { font-variant-numeric: tabular-nums; font-weight: 500; }
        .rup-footer-right { display: flex; align-items: center; gap: 8px; }
        .rup-footer-dot {
          width: 6px;
          height: 6px;
          border-radius: 50%;
          background: var(--accent, #79AEFF);
        }

        .rotated-90 { transform: rotate(90deg); transition: transform 150ms var(--ease-out, cubic-bezier(0,0,0.2,1)); }
        .rotated { transform: rotate(90deg); transition: transform 150ms var(--ease-out, cubic-bezier(0,0,0.2,1)); }

        @media (prefers-reduced-motion: reduce) {
          *, *::before, *::after {
            animation-duration: 0.01ms !important;
            transition-duration: 0.01ms !important;
          }
        }
      `}</style>
    </aside>
  );
};

export default RightUtilityPanel;
