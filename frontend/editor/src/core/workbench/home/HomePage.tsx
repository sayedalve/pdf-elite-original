/* eslint-disable */
import React, { useCallback, useMemo, useRef, useState } from "react";
import { HomeLeftNav } from "./HomeLeftNav";
import { QuickTools } from "./QuickTools";
import { RecentFiles, RecentDoc } from "./RecentFiles";
import { FolderOpen, FilePlus, Inbox } from "lucide-react";

export type HomeViewId =
  | "recent"
  | "starred"
  | "folders"
  | "organize"
  | "history"
  | "create"
  | string;

export interface HomePageProps {
  /** List of recent documents. Source of truth lives in parent or hook. */
  docs: RecentDoc[];
  /** Called when a local PDF file is selected via Open PDF */
  onOpenPdf: (file: File) => void;
  /** Called when a recent doc row is opened */
  onOpenDoc: (doc: RecentDoc) => void;
  /** Remove from recents */
  onRemove?: (id: string) => void;
  /** Star / unstar */
  onToggleStar?: (id: string) => void;
  /** Quick tool selected */
  onToolClick?: (toolId: string) => void;
  /** Create blank PDF */
  onCreatePdf?: () => void;
  /** Active navigation id override (controlled) */
  activeView?: HomeViewId;
  /** Navigation change callback */
  onNavigate?: (id: HomeViewId) => void;
}

export const HomePage: React.FC<HomePageProps> = ({
  docs,
  onOpenPdf,
  onOpenDoc,
  onRemove,
  onToggleStar,
  onToolClick,
  onCreatePdf,
  activeView,
  onNavigate,
}) => {
  const [internalView, setInternalView] = useState<HomeViewId>("recent");
  const activeId = activeView ?? internalView;
  const fileInputRef = useRef<HTMLInputElement>(null);
  const [isOpening, setIsOpening] = useState(false);

  const setView = useCallback(
    (id: HomeViewId) => {
      if (onNavigate) {
        onNavigate(id);
      } else {
        setInternalView(id);
      }
    },
    [onNavigate],
  );

  // Filtering logic: document-first, progressive disclosure
  const filteredDocs = useMemo(() => {
    switch (activeId) {
      case "starred":
        return docs.filter((d) => d.starred);
      case "folders":
        // Folders view placeholder - parent can replace with real folder data
        return [];
      case "recent":
      default:
        return docs;
    }
  }, [docs, activeId]);

  const tryTauriOpen = useCallback(async (): Promise<boolean> => {
    // Tauri v1/v2 dialog support with graceful fallback
    try {
      const tauri = (window as any).__TAURI__;
      if (!tauri) return false;

      // Try modern plugin first, then legacy API
      let openFn: any = null;
      try {
        const mod = await import("@tauri-apps/plugin-dialog" as any);
        openFn = mod.open;
      } catch {
        try {
          const mod = await import("@tauri-apps/plugin-dialog" as any);
          openFn = mod.open;
        } catch {
          if (tauri.dialog?.open) openFn = tauri.dialog.open;
        }
      }

      if (!openFn) return false;

      const selected = await openFn({
        multiple: false,
        filters: [{ name: "PDF", extensions: ["pdf"] }],
      });

      if (!selected) return true; // user cancelled, but handled

      const pathVal =
        typeof selected === "string"
          ? selected
          : Array.isArray(selected)
            ? selected[0]
            : null;
      if (!pathVal) return true;

      const fileName =
        pathVal.split("/").pop()?.split("\\").pop() || "document.pdf";
      // Create a File-like object carrying the native path for the backend to read
      const fakeFile = new File([], fileName, { type: "application/pdf" });
      (fakeFile as any).path = pathVal;
      onOpenPdf(fakeFile);
      return true;
    } catch {
      return false;
    }
  }, [onOpenPdf]);

  const handleOpenPdfClick = useCallback(async () => {
    if (isOpening) return;
    setIsOpening(true);
    const handledByTauri = await tryTauriOpen();
    if (!handledByTauri) {
      fileInputRef.current?.click();
    }
    setIsOpening(false);
  }, [isOpening, tryTauriOpen]);

  const handleFileInputChange = useCallback(
    (e: React.ChangeEvent<HTMLInputElement>) => {
      const file = e.target.files?.[0];
      if (file) {
        // Only accept PDFs at this gate; other types can be handled by convert/create flows
        if (
          file.type === "application/pdf" ||
          file.name.toLowerCase().endsWith(".pdf")
        ) {
          onOpenPdf(file);
        } else {
          // Still forward - parent decides (e.g., combine, convert)
          onOpenPdf(file);
        }
      }
      // Reset value so same file can be selected again
      if (fileInputRef.current) {
        fileInputRef.current.value = "";
      }
    },
    [onOpenPdf],
  );

  const handleNavigate = useCallback(
    (id: string) => {
      if (id === "create") {
        if (onCreatePdf) {
          onCreatePdf();
        } else if (onToolClick) {
          onToolClick("create-blank");
        }
        return;
      }
      setView(id);
    },
    [setView, onCreatePdf, onToolClick],
  );

  const handleToolClick = useCallback(
    (toolId: string) => {
      if (onToolClick) {
        onToolClick(toolId);
      }
    },
    [onToolClick],
  );

  const safeOnRemove = useCallback(
    (id: string) => {
      if (onRemove) onRemove(id);
    },
    [onRemove],
  );

  const safeOnToggleStar = useCallback(
    (id: string) => {
      if (onToggleStar) onToggleStar(id);
    },
    [onToggleStar],
  );

  const renderCenterEmpty = (
    title: string,
    desc: string,
    icon: React.ReactNode,
  ) => (
    <div className="home-empty">
      <div className="home-empty-icon">{icon}</div>
      <h3 className="home-empty-title">{title}</h3>
      <p className="home-empty-desc">{desc}</p>
    </div>
  );

  const renderMainContent = () => {
    if (activeId === "folders") {
      return (
        <>
          <QuickTools onToolClick={handleToolClick} />
          {renderCenterEmpty(
            "No recent folders",
            "Folders you open will appear here for quick access.",
            <FolderOpen size={28} />,
          )}
        </>
      );
    }

    if (activeId === "starred" && filteredDocs.length === 0) {
      return (
        <>
          <QuickTools onToolClick={handleToolClick} />
          {renderCenterEmpty(
            "No starred files",
            "Star important documents to find them faster.",
            <Inbox size={28} />,
          )}
        </>
      );
    }

    if (activeId === "organize" || activeId === "history") {
      return (
        <>
          <QuickTools onToolClick={handleToolClick} />
          {renderCenterEmpty(
            activeId === "organize" ? "Batch Tools" : "Document History",
            activeId === "organize"
              ? "Batch convert, compress, and organize tools will be available here."
              : "Version history and audit trail for local documents.",
            <FilePlus size={28} />,
          )}
        </>
      );
    }

    // Default: document-first view - QuickTools on top, RecentFiles below
    return (
      <>
        <QuickTools onToolClick={handleToolClick} />
        <RecentFiles
          docs={filteredDocs}
          onOpen={onOpenDoc}
          onRemove={safeOnRemove}
          onToggleStar={safeOnToggleStar}
        />
      </>
    );
  };

  return (
    <div className="home-page-root">
      <div className="home-left-wrapper">
        <HomeLeftNav
          activeId={activeId}
          onNavigate={handleNavigate}
          onOpenPdf={handleOpenPdfClick}
        />
      </div>

      <main className="home-main">
        <div className="home-main-inner">{renderMainContent()}</div>
      </main>

      {/* Hidden input for web file open fallback */}
      <input
        ref={fileInputRef}
        type="file"
        accept=".pdf,application/pdf"
        style={{ display: "none" }}
        onChange={handleFileInputChange}
        tabIndex={-1}
        aria-hidden="true"
      />

      <style>{`
        .home-page-root {
          display: flex;
          flex-direction: row;
          width: 100%;
          height: 100vh;
          background: var(--app-bg);
          overflow: hidden;
          font-family: var(--font-sans);
          color: var(--text-primary);
          -webkit-font-smoothing: antialiased;
        }

        .home-left-wrapper {
          width: 260px;
          min-width: 260px;
          height: 100%;
          flex-shrink: 0;
          background: var(--sidebar-bg);
          border-right: 1px solid var(--border);
          overflow: hidden;
          display: flex;
          flex-direction: column;
        }

        .home-main {
          flex: 1;
          min-width: 0;
          height: 100%;
          overflow-y: auto;
          overflow-x: hidden;
          background: var(--app-bg);
          padding: 24px;
          display: flex;
          flex-direction: column;
          align-items: center;
          scrollbar-width: thin;
          scrollbar-color: var(--scrollbar) transparent;
        }

        .home-main-inner {
          width: 100%;
          max-width: 1280px;
          display: flex;
          flex-direction: column;
          gap: 24px;
          margin: 0 auto;
          /* Intelligent full-width usage */
          flex: 1;
        }

        /* Empty states - professional hierarchy without duplicate controls */
        .home-empty {
          background: var(--surface-elevated);
          border: 1px solid var(--border);
          border-radius: 16px;
          padding: 48px 32px;
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          text-align: center;
          gap: 12px;
          min-height: 320px;
        }

        .home-empty-icon {
          width: 56px;
          height: 56px;
          border-radius: 14px;
          background: var(--surface-card);
          border: 1px solid var(--border);
          display: flex;
          align-items: center;
          justify-content: center;
          color: var(--text-tertiary);
          margin-bottom: 8px;
        }

        .home-empty-title {
          margin: 0;
          font-size: var(--text-lg);
          font-weight: 600;
          line-height: var(--leading-snug);
          color: var(--text-primary);
          letter-spacing: -0.2px;
        }

        .home-empty-desc {
          margin: 0;
          font-size: var(--text-md);
          font-weight: 400;
          line-height: var(--leading-normal);
          color: var(--text-secondary);
          max-width: 360px;
        }

        /* Focus and interaction polish */
        .home-main:focus {
          outline: none;
        }

        /* Responsive: left nav collapses behavior expected to be handled by parent layout */
        @media (max-width: 960px) {
          .home-left-wrapper {
            width: 72px;
            min-width: 72px;
          }
          .home-main {
            padding: 16px;
          }
        }

        @media (max-width: 640px) {
          .home-page-root {
            flex-direction: column;
          }
          .home-left-wrapper {
            width: 100%;
            min-width: 100%;
            height: auto;
            max-height: 200px;
          }
          .home-main {
            padding: 12px;
          }
        }
      `}</style>
    </div>
  );
};

export default HomePage;
