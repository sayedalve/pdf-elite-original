/* eslint-disable no-restricted-syntax */
import React, { useState, useMemo } from "react";
import {
  FileText,
  Search,
  LayoutGrid,
  List,
  Trash2,
  Star,
  MoreHorizontal,
  Clock,
} from "lucide-react";

export type RecentDoc = {
  id: string;
  name: string;
  path: string;
  size: string;
  sizeBytes: number;
  modified: string;
  modifiedTs: number;
  pages?: number;
  starred?: boolean;
  lastPage?: number;
  lastZoom?: number;
};

type Props = {
  docs: RecentDoc[];
  onOpen: (doc: RecentDoc) => void;
  onRemove: (id: string) => void;
  onToggleStar: (id: string) => void;
};

export const RecentFiles: React.FC<Props> = ({
  docs,
  onOpen,
  onRemove,
  onToggleStar,
}) => {
  const [view, setView] = useState<"list" | "grid">("list");
  const [query, setQuery] = useState("");
  const [sortBy, setSortBy] = useState<"modified" | "name" | "size">(
    "modified",
  );
  const [sortDir, setSortDir] = useState<"desc" | "asc">("desc");

  // Deduplication logic is in hook, but UI also ensures single entry per path
  const filtered = useMemo(() => {
    let list = [...docs];
    if (query) {
      const q = query.toLowerCase();
      list = list.filter(
        (d) =>
          d.name.toLowerCase().includes(q) || d.path.toLowerCase().includes(q),
      );
    }
    list.sort((a, b) => {
      let cmp = 0;
      if (sortBy === "modified") cmp = a.modifiedTs - b.modifiedTs;
      if (sortBy === "name") cmp = a.name.localeCompare(b.name);
      if (sortBy === "size") cmp = a.sizeBytes - b.sizeBytes;
      return sortDir === "desc" ? -cmp : cmp;
    });
    return list;
  }, [docs, query, sortBy, sortDir]);

  const handleSort = (col: typeof sortBy) => {
    if (sortBy === col) {
      setSortDir((d) => (d === "desc" ? "asc" : "desc"));
    } else {
      setSortBy(col);
      setSortDir("desc");
    }
  };

  return (
    <div className="recent-files">
      <div className="rf-header">
        <h2>Recent Files</h2>
        <div className="rf-controls">
          <div className="search-box">
            <Search size={16} />
            <input
              placeholder="Search"
              value={query}
              onChange={(e) => setQuery(e.target.value)}
            />
          </div>
          <div className="view-toggles">
            <button
              className={`vt-btn ${view === "list" ? "active" : ""}`}
              onClick={() => setView("list")}
            >
              <List size={16} />
            </button>
            <button
              className={`vt-btn ${view === "grid" ? "active" : ""}`}
              onClick={() => setView("grid")}
            >
              <LayoutGrid size={16} />
            </button>
          </div>
        </div>
      </div>

      {view === "list" ? (
        <div className="rf-table">
          <div className="rf-table-header">
            <span className="col-name" onClick={() => handleSort("name")}>
              Name {sortBy === "name" ? (sortDir === "desc" ? "↓" : "↑") : "↕"}
            </span>
            <span
              className="col-modified"
              onClick={() => handleSort("modified")}
            >
              Modified Time{" "}
              {sortBy === "modified" ? (sortDir === "desc" ? "↓" : "↑") : "↕"}
            </span>
            <span className="col-size" onClick={() => handleSort("size")}>
              Size {sortBy === "size" ? (sortDir === "desc" ? "↓" : "↑") : "↕"}
            </span>
            <span className="col-actions"></span>
          </div>
          <div className="rf-table-body">
            {filtered.length === 0 ? (
              <div className="empty-state">
                <FileText size={32} />
                <span>No recent files</span>
                <small>Open a PDF to see it here</small>
              </div>
            ) : (
              filtered.map((doc) => (
                <div
                  key={doc.id}
                  className="rf-row"
                  onClick={() => onOpen(doc)}
                >
                  <div className="col-name">
                    <div className="file-icon">
                      <FileText size={16} />
                    </div>
                    <span className="file-name" title={doc.path}>
                      {doc.name}
                    </span>
                    {doc.lastPage && doc.lastPage > 1 && (
                      <span className="page-badge">p{doc.lastPage}</span>
                    )}
                  </div>
                  <div className="col-modified">
                    <Clock size={12} />
                    {doc.modified}
                  </div>
                  <div className="col-size">{doc.size}</div>
                  <div className="col-actions">
                    <button
                      className="icon-btn"
                      onClick={(e) => {
                        e.stopPropagation();
                        onToggleStar(doc.id);
                      }}
                      title="Star"
                    >
                      <Star
                        size={14}
                        fill={doc.starred ? "#79AEFF" : "none"}
                        color={doc.starred ? "#79AEFF" : "currentColor"}
                      />
                    </button>
                    <button
                      className="icon-btn"
                      onClick={(e) => {
                        e.stopPropagation();
                        onRemove(doc.id);
                      }}
                      title="Remove"
                    >
                      <Trash2 size={14} />
                    </button>
                    <button
                      className="icon-btn"
                      onClick={(e) => e.stopPropagation()}
                    >
                      <MoreHorizontal size={14} />
                    </button>
                  </div>
                </div>
              ))
            )}
          </div>
        </div>
      ) : (
        <div className="rf-grid">
          {filtered.map((doc) => (
            <div key={doc.id} className="rf-card" onClick={() => onOpen(doc)}>
              <div className="card-preview">
                <FileText size={24} />
                <span className="pages">{doc.pages || "?"} pages</span>
              </div>
              <div className="card-info">
                <span className="card-name">{doc.name}</span>
                <span className="card-meta">
                  {doc.modified} • {doc.size}
                </span>
              </div>
            </div>
          ))}
        </div>
      )}

      <style>{`
        .recent-files {
          background: var(--surface-elevated);
          border: 1px solid var(--border);
          border-radius: 16px;
          padding: 20px 24px;
          display: flex;
          flex-direction: column;
          gap: 16px;
          min-height: 320px;
        }
        .rf-header {
          display: flex;
          justify-content: space-between;
          align-items: center;
          gap: 16px;
        }
        .rf-header h2 {
          font-size: 16px;
          font-weight: 600;
          margin: 0;
        }
        .rf-controls {
          display: flex;
          gap: 12px;
          align-items: center;
        }
        .search-box {
          display: flex;
          align-items: center;
          gap: 8px;
          background: var(--app-bg);
          border: 1px solid var(--border);
          border-radius: 10px;
          padding: 8px 12px;
          width: 220px;
        }
        .search-box input {
          background: transparent;
          border: none;
          outline: none;
          color: var(--text-primary);
          font-size: 13px;
          width: 100%;
        }
        .search-box input::placeholder {
          color: var(--text-tertiary);
        }
        .view-toggles {
          display: flex;
          background: var(--app-bg);
          border: 1px solid var(--border);
          border-radius: 10px;
          padding: 2px;
        }
        .vt-btn {
          width: 32px;
          height: 28px;
          border-radius: 7px;
          border: none;
          background: transparent;
          color: var(--text-secondary);
          display: flex;
          align-items: center;
          justify-content: center;
          cursor: pointer;
        }
        .vt-btn.active {
          background: var(--surface-card);
          color: var(--text-primary);
        }
        .rf-table-header {
          display: grid;
          grid-template-columns: 1fr 180px 100px 100px;
          gap: 16px;
          padding: 8px 12px;
          font-size: 12px;
          font-weight: 500;
          color: var(--text-tertiary);
          border-bottom: 1px solid var(--border-subtle);
        }
        .rf-table-header span {
          cursor: pointer;
          user-select: none;
        }
        .rf-table-header span:hover {
          color: var(--text-secondary);
        }
        .rf-table-body {
          display: flex;
          flex-direction: column;
        }
        .rf-row {
          display: grid;
          grid-template-columns: 1fr 180px 100px 100px;
          gap: 16px;
          padding: 12px;
          border-radius: 10px;
          align-items: center;
          cursor: pointer;
          transition: background var(--duration-fast) var(--ease-out);
        }
        .rf-row:hover {
          background: var(--surface-hover);
        }
        .rf-row:hover .col-actions {
          opacity: 1;
        }
        .col-name {
          display: flex;
          align-items: center;
          gap: 10px;
          min-width: 0;
        }
        .file-icon {
          width: 28px;
          height: 28px;
          background: var(--accent-subtle);
          color: var(--accent);
          border-radius: 6px;
          display: flex;
          align-items: center;
          justify-content: center;
          flex-shrink: 0;
        }
        .file-name {
          font-size: 13px;
          font-weight: 500;
          color: var(--text-primary);
          white-space: nowrap;
          overflow: hidden;
          text-overflow: ellipsis;
        }
        .page-badge {
          font-size: 10px;
          background: var(--surface-card);
          border: 1px solid var(--border);
          padding: 2px 6px;
          border-radius: 4px;
          color: var(--text-secondary);
        }
        .col-modified {
          font-size: 12px;
          color: var(--text-secondary);
          display: flex;
          align-items: center;
          gap: 6px;
        }
        .col-size {
          font-size: 12px;
          color: var(--text-secondary);
        }
        .col-actions {
          display: flex;
          gap: 4px;
          opacity: 0;
          transition: opacity var(--duration-fast) var(--ease-out);
        }
        .icon-btn {
          width: 28px;
          height: 28px;
          border-radius: 6px;
          border: none;
          background: transparent;
          color: var(--text-secondary);
          display: flex;
          align-items: center;
          justify-content: center;
          cursor: pointer;
        }
        .icon-btn:hover {
          background: var(--surface-card);
          color: var(--text-primary);
        }
        .rf-grid {
          display: grid;
          grid-template-columns: repeat(auto-fill, minmax(180px, 1fr));
          gap: 16px;
        }
        .rf-card {
          background: var(--surface-card);
          border: 1px solid var(--border);
          border-radius: 12px;
          overflow: hidden;
          cursor: pointer;
          transition: all var(--duration-normal) var(--ease-out);
        }
        .rf-card:hover {
          transform: translateY(-2px);
          border-color: var(--border-strong);
          box-shadow: 0 4px 12px rgba(0,0,0,0.2);
        }
        .card-preview {
          height: 120px;
          background: var(--app-bg);
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          gap: 8px;
          color: var(--text-tertiary);
        }
        .card-preview .pages {
          font-size: 11px;
        }
        .card-info {
          padding: 12px;
          display: flex;
          flex-direction: column;
          gap: 4px;
        }
        .card-name {
          font-size: 13px;
          font-weight: 500;
          white-space: nowrap;
          overflow: hidden;
          text-overflow: ellipsis;
        }
        .card-meta {
          font-size: 11px;
          color: var(--text-secondary);
        }
        .empty-state {
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          gap: 12px;
          padding: 48px;
          color: var(--text-tertiary);
        }
        .empty-state span {
          font-size: 14px;
          font-weight: 500;
        }
        .empty-state small {
          font-size: 12px;
        }
      `}</style>
    </div>
  );
};
