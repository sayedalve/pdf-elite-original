/* eslint-disable */
import React, { useState, useCallback } from "react";
import { HomePage } from "./workbench/home/HomePage";
import { ViewerShell } from "./workbench/viewer/ViewerShell";
import { useRecentDocs } from "./core/hooks/useRecentDocs";
import "./styles/design-tokens.css";

type AppMode = "home" | "viewer";

function App() {
  const { docs, addOrUpdate, remove, toggleStar } = useRecentDocs();
  const [mode, setMode] = useState<AppMode>("home");
  const [activeView, setActiveView] = useState("recent");

  const handleOpenPdf = useCallback(
    (file: File) => {
      const sizeMB = file.size / (1024 * 1024);
      const sizeStr =
        sizeMB >= 1
          ? `${sizeMB.toFixed(1)} MB`
          : `${(file.size / 1024).toFixed(1)} KB`;
      addOrUpdate({
        name: file.name,
        path: (file as any).path || file.name,
        size: sizeStr,
        sizeBytes: file.size,
      });
      setMode("viewer");
    },
    [addOrUpdate],
  );

  const handleOpenDoc = useCallback(
    (doc: any) => {
      // Update recency without duplicating - critical fix
      addOrUpdate({
        name: doc.name,
        path: doc.path,
        size: doc.size,
        sizeBytes: doc.sizeBytes,
      });
      setMode("viewer");
    },
    [addOrUpdate],
  );

  const handleToolClick = useCallback((toolId: string) => {
    console.log("Tool clicked:", toolId);
    // Progressive disclosure: tools open appropriate mode in viewer or stay in home
    // For now, open viewer with relevant mode
    if (["edit", "comment", "organize"].includes(toolId)) {
      setMode("viewer");
    }
  }, []);

  return (
    <div className="pdf-elite-app">
      {mode === "home" ? (
        <HomePage
          docs={docs}
          onOpenPdf={handleOpenPdf}
          onOpenDoc={handleOpenDoc}
          onRemove={remove}
          onToggleStar={toggleStar}
          onToolClick={handleToolClick}
          activeView={activeView}
          onNavigate={setActiveView}
        />
      ) : (
        <ViewerShell onClose={() => setMode("home")} />
      )}

      <style>{`
        .pdf-elite-app {
          width: 100vw;
          height: 100vh;
          overflow: hidden;
          background: var(--app-bg);
          font-family: var(--font-sans);
          -webkit-font-smoothing: antialiased;
        }
        /* Global professional resets */
        button {
          font-family: inherit;
        }
        /* Tauri window drag region handling */
        .pdf-elite-app * {
          -webkit-user-select: none;
        }
        .pdf-elite-app input,
        .pdf-elite-app [contenteditable] {
          -webkit-user-select: text;
        }
      `}</style>
    </div>
  );
}

export default App;
