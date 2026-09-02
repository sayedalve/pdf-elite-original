/* eslint-disable no-restricted-syntax */
import React from "react";

import { ToolId } from "@app/types/toolId";

interface HomeQuickToolsProps {
  filteredTools: Record<string, any>;
  onSelect: (toolId: string) => void;
}

const QUICK_TOOL_IDS: ToolId[] = [
  "merge",
  "split",
  "compress",
  "multiTool", // Organize
  "sign",
  "addPassword",
  "watermark",
  "convert",
];

export const HomeQuickTools: React.FC<HomeQuickToolsProps> = ({
  filteredTools,
  onSelect,
}) => {
  // Find the tools in the registry
  const quickTools = QUICK_TOOL_IDS.map((id) => ({
    id,
    tool: filteredTools[id],
  })).filter((item) => item.tool !== undefined);

  if (quickTools.length === 0) {
    return null;
  }

  return (
    <div
      style={{
        display: "grid",
        gridTemplateColumns: "repeat(auto-fill, minmax(140px, 1fr))",
        gap: "0.5rem",
        width: "100%",
      }}
    >
      {quickTools.map(({ id, tool }) => (
        <button
          key={id}
          type="button"
          onClick={() => onSelect(id)}
          style={{
            display: "flex",
            flexDirection: "row",
            alignItems: "center",
            gap: "0.5rem",
            padding: "0.5rem",
            background: "transparent",
            border: "1px solid var(--c-border-subtle, rgba(255,255,255,0.05))",
            borderRadius: "6px",
            textAlign: "left",
            cursor: "pointer",
            transition: "all 0.15s ease",
          }}
          onMouseEnter={(e) => {
            e.currentTarget.style.background =
              "var(--c-surface-elevated, var(--p-doc-surface-elevated))";
            e.currentTarget.style.borderColor =
              "var(--c-border, var(--p-doc-surface-hover-selected))";
          }}
          onMouseLeave={(e) => {
            e.currentTarget.style.background = "transparent";
            e.currentTarget.style.borderColor =
              "var(--c-border-subtle, rgba(255,255,255,0.05))";
          }}
        >
          <div
            style={{
              display: "flex",
              alignItems: "center",
              justifyContent: "center",
              width: "24px",
              height: "24px",
              color: "var(--c-accent, var(--p-doc-accent-primary))",
              flexShrink: 0,
            }}
          >
            {tool.icon}
          </div>
          <span
            style={{
              fontSize: "0.8125rem",
              fontWeight: 500,
              color: "var(--c-text, var(--p-doc-text-primary))",
              whiteSpace: "nowrap",
              overflow: "hidden",
              textOverflow: "ellipsis",
            }}
          >
            {tool.name}
          </span>
        </button>
      ))}
    </div>
  );
};
