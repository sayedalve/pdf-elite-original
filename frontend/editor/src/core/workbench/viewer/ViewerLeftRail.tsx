import React from "react";
import {
  Eye,
  MessageSquare,
  Edit3,
  LayoutGrid,
  Wrench,
  Menu,
  Bookmark,
  Paperclip,
} from "lucide-react";
import type { ToolMode } from "@app/hooks/useToolLifecycle";

type Props = {
  activeMode: ToolMode;
  onModeChange: (mode: ToolMode) => void;
  // Sidebars
  sidebarOpen: "thumbnails" | "bookmarks" | "attachments" | null;
  onToggleSidebar: (
    sidebar: "thumbnails" | "bookmarks" | "attachments",
  ) => void;
};

const modes: { id: ToolMode; label: string; icon: React.ReactNode }[] = [
  { id: "view", label: "View", icon: <Eye size={24} /> },
  { id: "comment", label: "Comment", icon: <MessageSquare size={24} /> },
  { id: "edit", label: "Edit", icon: <Edit3 size={24} /> },
  { id: "organize", label: "Organize", icon: <LayoutGrid size={24} /> },
  { id: "tools", label: "Tools", icon: <Wrench size={24} /> },
];

export const ViewerLeftRail: React.FC<Props> = ({
  activeMode,
  onModeChange,
  sidebarOpen,
  onToggleSidebar,
}) => {
  return (
    <div className="viewer-left-rail">
      {/* eslint-disable no-restricted-syntax */}
      <div className="rail-modes">
        {modes.map((m) => (
          <button
            key={m.id}
            type="button"
            className={`rail-btn ${activeMode === m.id ? "active" : ""}`}
            onClick={() => onModeChange(m.id)}
            title={m.label}
          >
            <span className="rail-icon">{m.icon}</span>
            <span className="rail-label">{m.label}</span>
          </button>
        ))}
      </div>

      <div className="rail-divider" />

      <div
        className="rail-modes"
        style={{ marginTop: "auto", marginBottom: "8px" }}
      >
        <button
          type="button"
          className={`rail-btn ${sidebarOpen === "thumbnails" ? "active" : ""}`}
          onClick={() => onToggleSidebar("thumbnails")}
          title="Thumbnails"
        >
          <span className="rail-icon">
            <Menu size={22} />
          </span>
        </button>
        <button
          type="button"
          className={`rail-btn ${sidebarOpen === "bookmarks" ? "active" : ""}`}
          onClick={() => onToggleSidebar("bookmarks")}
          title="Bookmarks"
        >
          <span className="rail-icon">
            <Bookmark size={22} />
          </span>
        </button>
        <button
          type="button"
          className={`rail-btn ${sidebarOpen === "attachments" ? "active" : ""}`}
          onClick={() => onToggleSidebar("attachments")}
          title="Attachments"
        >
          <span className="rail-icon">
            <Paperclip size={22} />
          </span>
        </button>
      </div>
      {/* eslint-enable no-restricted-syntax */}

      <style>{`
        .viewer-left-rail {
          width: var(--viewer-left-rail-width);
          background: var(--viewer-left-rail-bg);
          border-right: 1px solid var(--border);
          display: flex;
          flex-direction: column;
          align-items: center;
          padding: 12px 0;
          gap: 8px;
          flex-shrink: 0;
        }
        .rail-modes {
          display: flex;
          flex-direction: column;
          gap: 4px;
          width: 100%;
          padding: 0 8px;
        }
        .rail-btn {
          display: flex;
          flex-direction: column;
          align-items: center;
          gap: 4px;
          padding: 10px 4px;
          border-radius: 10px;
          border: none;
          background: transparent;
          color: var(--text-secondary);
          cursor: pointer;
          transition: all var(--duration-fast) var(--ease-out);
          width: 100%;
        }
        .rail-btn:hover {
          background: var(--surface-hover);
          color: var(--text-primary);
        }
        .rail-btn.active {
          background: var(--surface-selected);
          color: var(--text-primary);
        }
        .rail-btn.active .rail-icon {
          color: var(--accent);
        }
        .rail-icon {
          display: flex;
        }
        .rail-label {
          font-size: 11px;
          font-weight: 500;
          letter-spacing: 0.2px;
        }
        .rail-divider {
          width: 32px;
          height: 1px;
          background: var(--border);
          margin: 8px 0;
        }
      `}</style>
    </div>
  );
};
