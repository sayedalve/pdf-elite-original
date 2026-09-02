/* eslint-disable no-restricted-syntax */
import React, { useState } from "react";
import { useTranslatedToolCatalog } from "@app/data/useTranslatedToolRegistry";
import { SubcategoryId } from "@app/data/toolsTaxonomy";

type Props = {
  onToolSelect?: (toolId: string) => void;
};

export const ViewerToolsGrid: React.FC<Props> = ({ onToolSelect }) => {
  const { allTools } = useTranslatedToolCatalog();
  const [activeCategory, setActiveCategory] = useState<SubcategoryId | "all">(
    "all",
  );

  const toolsList = Object.entries(allTools).map(([id, tool]) => ({
    id,
    ...tool,
  }));

  const filteredTools =
    activeCategory === "all"
      ? toolsList
      : toolsList.filter((t) => t.subcategoryId === activeCategory);

  const categories = Array.from(new Set(toolsList.map((t) => t.subcategoryId)));

  return (
    <div className="viewer-tools-grid">
      <div className="vtg-header">
        <h2>All Tools</h2>
        <div className="vtg-filters">
          <button
            className={`vtg-filter ${activeCategory === "all" ? "active" : ""}`}
            onClick={() => setActiveCategory("all")}
          >
            All
          </button>
          {categories.map((cat) => (
            <button
              key={cat}
              className={`vtg-filter ${activeCategory === cat ? "active" : ""}`}
              onClick={() => setActiveCategory(cat)}
            >
              {cat}
            </button>
          ))}
        </div>
      </div>

      <div className="vtg-grid">
        {filteredTools.map((tool) => (
          <button
            key={tool.id}
            className="vtg-card"
            onClick={() => onToolSelect?.(tool.id)}
          >
            <div className="vtg-icon">{tool.icon}</div>
            <div className="vtg-info">
              <span className="vtg-name">{tool.name}</span>
              <span className="vtg-desc">{tool.description}</span>
            </div>
          </button>
        ))}
      </div>

      <style>{`
        .viewer-tools-grid {
          position: absolute;
          inset: 0;
          background: var(--workspace-bg);
          z-index: 100;
          display: flex;
          flex-direction: column;
          overflow-y: auto;
          padding: 32px 48px;
        }
        .vtg-header {
          margin-bottom: 32px;
        }
        .vtg-header h2 {
          font-size: 24px;
          font-weight: 600;
          margin: 0 0 16px 0;
          color: var(--text-primary);
        }
        .vtg-filters {
          display: flex;
          gap: 8px;
          flex-wrap: wrap;
        }
        .vtg-filter {
          padding: 6px 14px;
          border-radius: 20px;
          background: var(--surface-elevated);
          border: 1px solid var(--border);
          color: var(--text-secondary);
          font-size: 13px;
          font-weight: 500;
          cursor: pointer;
          transition: all 0.2s;
          text-transform: capitalize;
        }
        .vtg-filter:hover {
          background: var(--surface-hover);
          color: var(--text-primary);
        }
        .vtg-filter.active {
          background: var(--accent);
          color: white;
          border-color: var(--accent);
        }
        .vtg-grid {
          display: grid;
          grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
          gap: 16px;
          padding-bottom: 64px;
        }
        .vtg-card {
          display: flex;
          align-items: flex-start;
          gap: 16px;
          background: var(--surface-card);
          border: 1px solid var(--border);
          border-radius: 12px;
          padding: 16px;
          text-align: left;
          cursor: pointer;
          transition: all 0.2s;
        }
        .vtg-card:hover {
          transform: translateY(-2px);
          box-shadow: 0 4px 12px rgba(0,0,0,0.05);
          border-color: var(--border-strong);
        }
        .vtg-icon {
          width: 40px;
          height: 40px;
          border-radius: 10px;
          background: var(--surface-hover);
          color: var(--accent);
          display: flex;
          align-items: center;
          justify-content: center;
          flex-shrink: 0;
        }
        .vtg-icon svg {
          width: 20px;
          height: 20px;
        }
        .vtg-info {
          display: flex;
          flex-direction: column;
          gap: 4px;
        }
        .vtg-name {
          font-weight: 600;
          font-size: 14px;
          color: var(--text-primary);
        }
        .vtg-desc {
          font-size: 12px;
          color: var(--text-secondary);
          line-height: 1.4;
          display: -webkit-box;
          -webkit-line-clamp: 2;
          -webkit-box-orient: vertical;
          overflow: hidden;
        }
      `}</style>
    </div>
  );
};
