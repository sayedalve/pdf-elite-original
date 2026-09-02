/* eslint-disable */
import React from "react";
import {
  Home,
  Clock,
  Star,
  FolderClock,
  FolderOpen,
  FileText,
  Box,
  Cloud,
  FileCheck,
  Receipt,
  Plus,
  FilePlus,
} from "lucide-react";

type NavItem = {
  id: string;
  label: string;
  icon: React.ReactNode;
  badge?: string;
  active?: boolean;
  section?: string;
};

type Props = {
  activeId: string;
  onNavigate: (id: string) => void;
  onOpenPdf: () => void;
};

export const HomeLeftNav: React.FC<Props> = ({
  activeId,
  onNavigate,
  onOpenPdf,
}) => {
  const primaryActions = [
    {
      id: "open",
      label: "Open PDF",
      icon: <FolderOpen size={18} />,
      variant: "primary",
    },
    {
      id: "create",
      label: "Create PDF",
      icon: <FilePlus size={18} />,
      variant: "secondary",
    },
  ];

  const docNav: NavItem[] = [
    {
      id: "recent",
      label: "Recent Files",
      icon: <Clock size={18} />,
      active: activeId === "recent",
    },
    {
      id: "starred",
      label: "Starred Files",
      icon: <Star size={18} />,
      active: activeId === "starred",
    },
    {
      id: "folders",
      label: "Recent Folders",
      icon: <FolderClock size={18} />,
      active: activeId === "folders",
    },
  ];

  // Only local document-related others - NO cloud, NO sign-in, NO Google Drive per offline cleanup
  const others: NavItem[] = [
    { id: "organize", label: "Batch Tools", icon: <Box size={18} /> },
    { id: "history", label: "Document History", icon: <FileCheck size={18} /> },
  ];

  return (
    <div className="home-left-nav">
      <div className="brand">
        <div className="brand-icon">
          <svg width="32" height="32" viewBox="0 0 32 32" fill="none">
            <rect width="32" height="32" rx="8" fill="#79AEFF" />
            <path
              d="M9 10.5C9 9.67157 9.67157 9 10.5 9H16.5C17.8807 9 19 10.1193 19 11.5V11.5C19 12.8807 17.8807 14 16.5 14H10.5C9.67157 14 9 13.3284 9 12.5V10.5Z"
              fill="white"
            />
            <path
              d="M9 16.5C9 15.6716 9.67157 15 10.5 15H20.5C21.8807 15 23 16.1193 23 17.5V17.5C23 18.8807 21.8807 20 20.5 20H10.5C9.67157 20 9 19.3284 9 18.5V16.5Z"
              fill="white"
              fillOpacity="0.9"
            />
            <path
              d="M9 22.5C9 21.6716 9.67157 21 10.5 21H14.5C15.8807 21 17 22.1193 17 23.5V23.5C17 24.8807 15.8807 26 14.5 26H10.5C9.67157 26 9 25.3284 9 24.5V22.5Z"
              fill="white"
              fillOpacity="0.7"
            />
          </svg>
        </div>
        <div className="brand-text">
          <span className="brand-name">PDF Elite</span>
          <span className="brand-sub">Professional</span>
        </div>
      </div>

      <div className="primary-actions">
        <button className="btn-primary" onClick={onOpenPdf}>
          <FolderOpen size={18} />
          <span>Open PDF</span>
        </button>
        <button className="btn-secondary" onClick={() => onNavigate("create")}>
          <FilePlus size={18} />
          <span>Create PDF</span>
        </button>
      </div>

      <div className="nav-section">
        <div className="nav-items">
          {docNav.map((item) => (
            <button
              key={item.id}
              className={`nav-item ${item.active ? "active" : ""}`}
              onClick={() => onNavigate(item.id)}
            >
              <span className="nav-icon">{item.icon}</span>
              <span className="nav-label">{item.label}</span>
            </button>
          ))}
        </div>
      </div>

      <div className="nav-section">
        <div className="nav-section-title">Others</div>
        <div className="nav-items">
          {others.map((item) => (
            <button
              key={item.id}
              className={`nav-item ${activeId === item.id ? "active" : ""}`}
              onClick={() => onNavigate(item.id)}
            >
              <span className="nav-icon">{item.icon}</span>
              <span className="nav-label">{item.label}</span>
              {item.badge && <span className="nav-badge">{item.badge}</span>}
            </button>
          ))}
        </div>
      </div>

      <div className="nav-footer">
        <div className="storage-hint">
          <div className="storage-icon">💾</div>
          <div className="storage-text">
            <span>100% Offline</span>
            <small>Your files never leave device</small>
          </div>
        </div>
      </div>

      <style>{`
        .home-left-nav {
          width: var(--sidebar-width);
          background: var(--sidebar-bg);
          border-right: 1px solid var(--border);
          display: flex;
          flex-direction: column;
          height: 100%;
          padding: 20px 12px;
          gap: 24px;
        }
        .brand {
          display: flex;
          align-items: center;
          gap: 12px;
          padding: 4px 8px;
        }
        .brand-text {
          display: flex;
          flex-direction: column;
          line-height: 1.1;
        }
        .brand-name {
          font-size: 16px;
          font-weight: 700;
          color: var(--text-primary);
          letter-spacing: -0.2px;
        }
        .brand-sub {
          font-size: 11px;
          font-weight: 500;
          color: var(--text-tertiary);
          text-transform: uppercase;
          letter-spacing: 0.5px;
        }
        .primary-actions {
          display: flex;
          flex-direction: column;
          gap: 10px;
          padding: 0 4px;
        }
        .btn-primary {
          background: #ffffff;
          color: #1e2130;
          border: none;
          border-radius: 10px;
          height: 44px;
          display: flex;
          align-items: center;
          gap: 10px;
          padding: 0 16px;
          font-weight: 600;
          font-size: 14px;
          cursor: pointer;
          transition: all var(--duration-normal) var(--ease-out);
        }
        .btn-primary:hover {
          transform: translateY(-1px);
          box-shadow: 0 4px 12px rgba(255,255,255,0.15);
        }
        .btn-secondary {
          background: transparent;
          color: var(--text-primary);
          border: 1px solid var(--border-strong);
          border-radius: 10px;
          height: 44px;
          display: flex;
          align-items: center;
          gap: 10px;
          padding: 0 16px;
          font-weight: 500;
          font-size: 14px;
          cursor: pointer;
          transition: all var(--duration-normal) var(--ease-out);
        }
        .btn-secondary:hover {
          background: var(--surface-hover);
          border-color: var(--border-strong);
        }
        .nav-section {
          display: flex;
          flex-direction: column;
          gap: 8px;
        }
        .nav-section-title {
          font-size: 11px;
          font-weight: 600;
          text-transform: uppercase;
          letter-spacing: 0.6px;
          color: var(--text-tertiary);
          padding: 0 12px;
        }
        .nav-items {
          display: flex;
          flex-direction: column;
          gap: 2px;
        }
        .nav-item {
          display: flex;
          align-items: center;
          gap: 12px;
          height: 36px;
          padding: 0 12px;
          border-radius: 8px;
          border: none;
          background: transparent;
          color: var(--text-secondary);
          font-size: 13px;
          font-weight: 500;
          cursor: pointer;
          transition: all var(--duration-fast) var(--ease-out);
          text-align: left;
          width: 100%;
          position: relative;
        }
        .nav-item:hover {
          background: var(--sidebar-item-hover);
          color: var(--text-primary);
        }
        .nav-item.active {
          background: var(--sidebar-item-active);
          color: var(--text-primary);
        }
        .nav-item.active::before {
          content: '';
          position: absolute;
          left: -12px;
          top: 50%;
          transform: translateY(-50%);
          width: 3px;
          height: 20px;
          background: var(--accent);
          border-radius: 0 3px 3px 0;
        }
        .nav-icon {
          display: flex;
          opacity: 0.9;
        }
        .nav-badge {
          margin-left: auto;
          font-size: 10px;
          font-weight: 600;
          background: var(--accent);
          color: var(--text-inverse);
          padding: 2px 6px;
          border-radius: 4px;
        }
        .nav-footer {
          margin-top: auto;
          padding: 12px;
          border-top: 1px solid var(--border-subtle);
        }
        .storage-hint {
          display: flex;
          gap: 10px;
          align-items: center;
          background: var(--surface-card);
          border: 1px solid var(--border-subtle);
          border-radius: 10px;
          padding: 10px 12px;
        }
        .storage-text {
          display: flex;
          flex-direction: column;
        }
        .storage-text span {
          font-size: 12px;
          font-weight: 600;
          color: var(--text-primary);
        }
        .storage-text small {
          font-size: 11px;
          color: var(--text-tertiary);
        }
      `}</style>
    </div>
  );
};
