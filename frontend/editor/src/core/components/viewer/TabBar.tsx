/* eslint-disable */
import React, { useRef } from "react";
import { X, Plus, FileText } from "lucide-react";

export interface TabItem {
  id: string;
  name: string;
  path: string;
  active: boolean;
}

export interface TabBarProps {
  tabs: TabItem[];
  onSwitch: (id: string) => void;
  onClose: (id: string) => void;
  onNew: () => void;
}

export const TabBar: React.FC<TabBarProps> = ({
  tabs,
  onSwitch,
  onClose,
  onNew,
}) => {
  const scrollRef = useRef<HTMLDivElement>(null);

  return (
    <div className="pdf-tab-bar" role="tablist" aria-label="Document tabs">
      <div className="pdf-tab-bar__scroll" ref={scrollRef}>
        {tabs.map((tab) => (
          <button
            key={tab.id}
            role="tab"
            aria-selected={tab.active}
            aria-label={`${tab.name}${tab.active ? " (active)" : ""}`}
            title={`${tab.name} — ${tab.path}`}
            className={`pdf-tab ${tab.active ? "pdf-tab--active" : "pdf-tab--inactive"}`}
            onClick={() => onSwitch(tab.id)}
            tabIndex={tab.active ? 0 : -1}
          >
            <span className="pdf-tab__icon" aria-hidden="true">
              <FileText size={14} strokeWidth={1.75} />
            </span>
            <span className="pdf-tab__name">{tab.name}</span>
            <span
              className="pdf-tab__close"
              role="button"
              aria-label={`Close ${tab.name}`}
              tabIndex={0}
              onClick={(e) => {
                e.stopPropagation();
                onClose(tab.id);
              }}
              onKeyDown={(e) => {
                if (e.key === "Enter" || e.key === " ") {
                  e.preventDefault();
                  e.stopPropagation();
                  onClose(tab.id);
                }
              }}
            >
              <X size={13} strokeWidth={2.2} />
            </span>
          </button>
        ))}

        <button
          className="pdf-tab-bar__new"
          aria-label="New tab"
          title="New tab"
          onClick={onNew}
          type="button"
        >
          <Plus size={14} strokeWidth={2} aria-hidden="true" />
        </button>
      </div>

      <style>{`
        .pdf-tab-bar {
          height: 38px;
          min-height: 38px;
          max-height: 38px;
          width: 100%;
          max-width: 100%;
          background: var(--tab-bar-bg, var(--app-bg, #1e2030));
          border-bottom: 1px solid var(--border, rgba(255,255,255,0.07));
          display: flex;
          align-items: stretch;
          flex-shrink: 0;
          position: relative;
          z-index: 5;
          box-sizing: border-box;
          user-select: none;
        }

        .pdf-tab-bar__scroll {
          display: flex;
          align-items: stretch;
          flex: 1 1 auto;
          min-width: 0;
          overflow-x: auto;
          overflow-y: hidden;
          scrollbar-width: none;
          -ms-overflow-style: none;
          gap: 2px;
          padding: 0 8px 0 10px;
        }

        .pdf-tab-bar__scroll::-webkit-scrollbar {
          display: none;
          height: 0;
          width: 0;
        }

        /* Tab */
        .pdf-tab {
          position: relative;
          display: inline-flex;
          align-items: center;
          gap: 7px;
          height: 28px;
          align-self: center;
          min-width: 124px;
          max-width: 220px;
          padding: 0 6px 0 10px;
          margin: 0;
          border: 1px solid transparent;
          border-bottom-width: 2px;
          border-radius: 6px;
          background: transparent;
          color: var(--text-secondary, #8b8fa3);
          font-family: inherit;
          font-size: 12.5px;
          font-weight: 450;
          line-height: 1;
          letter-spacing: -0.01em;
          white-space: nowrap;
          cursor: pointer;
          flex-shrink: 0;
          transition:
            background-color 150ms ease-out,
            color 150ms ease-out,
            border-color 150ms ease-out,
            opacity 120ms ease-out;
        }

        .pdf-tab:focus-visible {
          outline: 2px solid var(--focus-ring, var(--accent, #79AEFF));
          outline-offset: 1px;
        }

        .pdf-tab--inactive {
          background: transparent;
          border-color: transparent;
          border-bottom-color: transparent;
        }

        .pdf-tab--inactive:hover {
          background: var(--surface-hover, rgba(255,255,255,0.05));
          color: var(--text-primary, #d7d9e6);
          border-color: rgba(255,255,255,0.04);
          border-bottom-color: transparent;
        }

        .pdf-tab--active {
          background: var(--tab-active-bg, #2a2e42);
          color: var(--text-primary, #eceef8);
          border-color: rgba(255,255,255,0.06);
          border-bottom-color: var(--accent, #79AEFF);
          box-shadow:
            0 1px 0 0 var(--tab-active-bg, #2a2e42),
            inset 0 1px 0 rgba(255,255,255,0.04);
        }

        /* Professional subtle accent underline - no full chrome highlight */
        .pdf-tab--active::after {
          content: '';
          position: absolute;
          left: 8px;
          right: 8px;
          bottom: -2px;
          height: 2px;
          background: var(--accent, #79AEFF);
          border-radius: 1px 1px 0 0;
        }

        .pdf-tab__icon {
          display: inline-flex;
          align-items: center;
          justify-content: center;
          flex-shrink: 0;
          opacity: 0.7;
          transition: opacity 150ms ease-out;
        }

        .pdf-tab--active .pdf-tab__icon {
          opacity: 0.95;
          color: var(--accent, #79AEFF);
        }

        .pdf-tab__name {
          flex: 1 1 auto;
          min-width: 0;
          overflow: hidden;
          text-overflow: ellipsis;
          text-align: left;
          font-weight: 500;
          letter-spacing: -0.01em;
        }

        /* Close control - progressive disclosure */
        .pdf-tab__close {
          display: inline-flex;
          align-items: center;
          justify-content: center;
          width: 20px;
          height: 20px;
          margin-left: 2px;
          border-radius: 4px;
          border: none;
          background: transparent;
          color: inherit;
          flex-shrink: 0;
          cursor: pointer;
          opacity: 0;
          transform: scale(0.92);
          transition:
            background-color 120ms ease-out,
            opacity 120ms ease-out,
            transform 120ms ease-out,
            color 120ms ease-out;
          pointer-events: none;
        }

        /* Show on hover or when active - avoids layout shift */
        .pdf-tab:hover .pdf-tab__close,
        .pdf-tab--active .pdf-tab__close,
        .pdf-tab:focus-within .pdf-tab__close,
        .pdf-tab__close:focus-visible {
          opacity: 0.72;
          transform: scale(1);
          pointer-events: auto;
        }

        .pdf-tab__close:hover,
        .pdf-tab__close:focus-visible {
          opacity: 1 !important;
          background: var(--surface-hover-strong, rgba(255,255,255,0.08));
          color: var(--text-primary, #ffffff);
        }

        .pdf-tab__close:active {
          transform: scale(0.94);
        }

        .pdf-tab__close:focus-visible {
          outline: 1px solid var(--accent, #79AEFF);
          outline-offset: 0;
        }

        /* New tab button - secondary hierarchy */
        .pdf-tab-bar__new {
          display: inline-flex;
          align-items: center;
          justify-content: center;
          width: 28px;
          height: 26px;
          align-self: center;
          flex-shrink: 0;
          margin-left: 4px;
          margin-right: 2px;
          border-radius: 6px;
          border: 1px solid transparent;
          background: transparent;
          color: var(--text-tertiary, var(--text-secondary, #7e839c));
          cursor: pointer;
          transition:
            background-color 120ms ease-out,
            color 120ms ease-out,
            border-color 120ms ease-out;
        }

        .pdf-tab-bar__new:hover {
          background: var(--surface-hover, rgba(255,255,255,0.06));
          color: var(--text-primary, #dfe1ef);
          border-color: rgba(255,255,255,0.06);
        }

        .pdf-tab-bar__new:active {
          background: var(--surface-selected, rgba(255,255,255,0.08));
          transform: scale(0.97);
        }

        .pdf-tab-bar__new:focus-visible {
          outline: 2px solid var(--focus-ring, var(--accent, #79AEFF));
          outline-offset: 1px;
        }

        /* Document-first: subtle divider between tabs vs heavy chrome */
        .pdf-tab + .pdf-tab {
          position: relative;
        }

        /* Reduced motion */
        @media (prefers-reduced-motion: reduce) {
          .pdf-tab,
          .pdf-tab__close,
          .pdf-tab-bar__new,
          .pdf-tab__icon {
            transition: none !important;
            transform: none !important;
          }
        }

        /* Ensure visibility in light mode and high-contrast */
        @media (forced-colors: active) {
          .pdf-tab--active {
            border: 1px solid CanvasText;
          }
          .pdf-tab__close {
            opacity: 1;
            pointer-events: auto;
          }
        }
      `}</style>
    </div>
  );
};

export default TabBar;
