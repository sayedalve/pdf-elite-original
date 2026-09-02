/* eslint-disable */
import React, { useState } from "react";
import { useTranslatedToolCatalog } from "@app/data/useTranslatedToolRegistry";
import {
  Edit3,
  ArrowRightLeft,
  MessageSquare,
  Languages,
  Combine,
  Archive,
  Grid3x3,
  FileText,
  Image,
  Lock,
  Scissors,
} from "lucide-react";

type Tool = {
  id: string;
  name: string;
  desc: string;
  icon: React.ReactNode;
  color: string;
  bg: string;
};

const tools: Tool[] = [
  {
    id: "edit",
    name: "Edit PDF",
    desc: "Edit text and images in files.",
    icon: <Edit3 size={18} />,
    color: "#f59e0b",
    bg: "rgba(245,158,11,0.15)",
  },
  {
    id: "convert",
    name: "Convert PDF",
    desc: "Convert PDFs to Word, Excel, PPT, etc.",
    icon: <ArrowRightLeft size={18} />,
    color: "#10b981",
    bg: "rgba(16,185,129,0.15)",
  },
  {
    id: "comment",
    name: "Add Comments",
    desc: "Add highlights, notes, pencil, and other comments.",
    icon: <MessageSquare size={18} />,
    color: "#f97316",
    bg: "rgba(249,115,22,0.15)",
  },
  {
    id: "translate",
    name: "Translate PDF",
    desc: "Let AI translate the PDF with the original formatting.",
    icon: <Languages size={18} />,
    color: "#8b5cf6",
    bg: "rgba(139,92,246,0.15)",
  },
  {
    id: "merge",
    name: "Combine Files",
    desc: "Combine multiple files into a single PDF.",
    icon: <Combine size={18} />,
    color: "#3b82f6",
    bg: "rgba(59,130,246,0.15)",
  },
  {
    id: "compress",
    name: "Compress PDF",
    desc: "Reduce PDF file size.",
    icon: <Archive size={18} />,
    color: "#22c55e",
    bg: "rgba(34,197,94,0.15)",
  },
  {
    id: "batch",
    name: "Batch PDFs",
    desc: "Batch convert, create, print PDFs, etc.",
    icon: <Grid3x3 size={18} />,
    color: "#10b981",
    bg: "rgba(16,185,129,0.15)",
  },
];

type Props = {
  onToolClick: (id: string) => void;
};

export const QuickTools: React.FC<Props> = ({ onToolClick }) => {
  // Phase 33: progressive disclosure. Collapsed shows a clean, curated set for
  // basic users; "All Tools" reveals the FULL real tool catalog (the same
  // registry the viewer's tool grid uses) so advanced users can reach every
  // existing capability. Every card — curated or catalog — launches the real
  // tool via onToolClick(id); there are no fake, dead, or decorative controls.
  const [showAll, setShowAll] = useState(false);
  const { allTools } = useTranslatedToolCatalog();
  const catalogTools = Object.entries(allTools).map(([id, tool]) => ({
    id,
    ...tool,
  }));

  return (
    <div className="quick-tools">
      <div className="qt-header">
        <h2>{showAll ? "All Tools" : "Quick Tools"}</h2>
        <div className="qt-actions">
          <button
            className="all-tools-btn"
            onClick={() => setShowAll((v) => !v)}
            aria-expanded={showAll}
          >
            <Grid3x3 size={16} />
            {showAll ? "Show Less" : "All Tools"}
          </button>
        </div>
      </div>

      {showAll ? (
        <div className="tools-grid">
          {catalogTools.map((tool) => (
            <button
              key={tool.id}
              className="tool-card"
              onClick={() => onToolClick(tool.id)}
            >
              <div className="tool-icon tool-icon--catalog">{tool.icon}</div>
              <div className="tool-content">
                <span className="tool-name">{tool.name}</span>
                <span className="tool-desc">{tool.description}</span>
              </div>
            </button>
          ))}
        </div>
      ) : (
        <div className="tools-grid">
          {tools.map((tool) => (
            <button
              key={tool.id}
              className="tool-card"
              onClick={() => onToolClick(tool.id)}
            >
              <div
                className="tool-icon"
                style={{ background: tool.bg, color: tool.color }}
              >
                {tool.icon}
              </div>
              <div className="tool-content">
                <span className="tool-name">{tool.name}</span>
                <span className="tool-desc">{tool.desc}</span>
              </div>
            </button>
          ))}
        </div>
      )}

      <style>{`
        .quick-tools {
          background: var(--surface-elevated);
          border: 1px solid var(--border);
          border-radius: 16px;
          padding: 20px 24px;
        }
        .qt-header {
          display: flex;
          justify-content: space-between;
          align-items: center;
          margin-bottom: 20px;
        }
        .qt-header h2 {
          font-size: 16px;
          font-weight: 600;
          color: var(--text-primary);
          margin: 0;
        }
        .all-tools-btn {
          display: flex;
          align-items: center;
          gap: 6px;
          background: transparent;
          border: 1px solid var(--border);
          border-radius: 8px;
          padding: 6px 12px;
          font-size: 12px;
          font-weight: 500;
          color: var(--text-secondary);
          cursor: pointer;
          transition: all var(--duration-fast) var(--ease-out);
        }
        .all-tools-btn:hover {
          background: var(--surface-hover);
          color: var(--text-primary);
        }
        .tools-grid {
          display: grid;
          grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
          gap: 16px;
        }
        .tool-card {
          display: flex;
          gap: 12px;
          align-items: flex-start;
          background: transparent;
          border: 1px solid transparent;
          border-radius: 12px;
          padding: 14px;
          text-align: left;
          cursor: pointer;
          transition: all var(--duration-normal) var(--ease-out);
        }
        .tool-card:hover {
          background: var(--surface-card);
          border-color: var(--border);
          transform: translateY(-1px);
        }
        .tool-icon {
          width: 36px;
          height: 36px;
          border-radius: 10px;
          display: flex;
          align-items: center;
          justify-content: center;
          flex-shrink: 0;
        }
        .tool-icon--catalog {
          background: var(--surface-hover);
          color: var(--accent);
        }
        .tool-icon--catalog svg {
          width: 18px;
          height: 18px;
        }
        .tool-content {
          display: flex;
          flex-direction: column;
          gap: 4px;
          min-width: 0;
        }
        .tool-name {
          font-size: 14px;
          font-weight: 600;
          color: var(--text-primary);
        }
        .tool-desc {
          font-size: 12px;
          color: var(--text-secondary);
          line-height: 1.4;
          display: -webkit-box;
          -webkit-line-clamp: 2;
          -webkit-box-orient: vertical;
          overflow: hidden;
        }
        @media (max-width: 1280px) {
          .tools-grid {
            grid-template-columns: repeat(2, 1fr);
          }
        }
        @media (max-width: 900px) {
          .tools-grid {
            grid-template-columns: 1fr;
          }
        }
      `}</style>
    </div>
  );
};
