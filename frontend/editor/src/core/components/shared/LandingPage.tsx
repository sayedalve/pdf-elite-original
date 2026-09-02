/* eslint-disable no-restricted-syntax */
import React, { useState, useEffect, useCallback } from "react";
import { Dropzone } from "@mantine/dropzone";
import { useTranslation } from "react-i18next";
import { useFileHandler } from "@app/hooks/useFileHandler";
import { openFilesFromDisk } from "@app/services/openFilesFromDisk";
import { PDFEliteFileStub } from "@app/types/fileContext";
import { useToolWorkflow } from "@app/contexts/ToolWorkflowContext";
import { HomeQuickTools } from "@app/components/shared/HomeQuickTools";
import { ToolId } from "@app/types/toolId";
import { getFileDate } from "@app/utils/fileUtils";
import { useIndexedDB } from "@app/contexts/IndexedDBContext";
import { useFileActions } from "@app/contexts/file/fileHooks";
import { useNavigationActions } from "@app/contexts/NavigationContext";
import ArticleOutlinedIcon from "@mui/icons-material/ArticleOutlined";
import FileUploadOutlinedIcon from "@mui/icons-material/FileUploadOutlined";
import AddRoundedIcon from "@mui/icons-material/AddRounded";
import "@app/components/shared/HomePage.css";

const LandingPage = () => {
  const { t } = useTranslation();
  const { addFiles } = useFileHandler();
  const fileInputRef = React.useRef<HTMLInputElement | null>(null);

  const indexedDB = useIndexedDB();
  const { actions: fileActions } = useFileActions();
  const [recentFiles, setRecentFiles] = useState<PDFEliteFileStub[]>([]);

  const { actions } = useNavigationActions();

  const {
    filteredTools,
    handleToolSelect,
    setLeftPanelView,
    setSidebarsVisible,
  } = useToolWorkflow();

  useEffect(() => {
    indexedDB.loadLeafMetadata().then((files) => {
      files.sort((a, b) => (b.lastModified ?? 0) - (a.lastModified ?? 0));
      setRecentFiles(files.slice(0, 12));
    });
  }, [indexedDB]);

  const handleFileDrop = async (files: File[]) => {
    await addFiles(files);
  };

  const handleNativeUploadClick = useCallback(async () => {
    const files = await openFilesFromDisk({
      multiple: true,
      onFallbackOpen: () => fileInputRef.current?.click(),
    });
    if (files.length > 0) {
      await addFiles(files);
    }
  }, [addFiles]);

  const handleFileSelect = async (
    event: React.ChangeEvent<HTMLInputElement>,
  ) => {
    const files = Array.from(event.target.files || []);
    if (files.length > 0) {
      await addFiles(files);
    }
    event.target.value = "";
  };

  const handleOpenRecent = async (fileId: string) => {
    const stub = recentFiles.find((f) => f.id === fileId);
    if (!stub) return;
    await fileActions.addPDFEliteFileStubs([stub]);
    actions.setWorkbench("viewer");
  };

  const handleToolClick = (id: string) => {
    handleToolSelect(id as ToolId);
    setSidebarsVisible(true);
  };

  const handleShowAllTools = () => {
    setLeftPanelView("toolPicker");
    setSidebarsVisible(true);
  };

  return (
    <div
      className="home-layout"
      style={{ display: "flex", width: "100%", height: "100%" }}
    >
      {/* ── Main Workspace ───────────────────────────── */}
      <main className="home-workspace">
        <Dropzone
          onDrop={handleFileDrop}
          multiple
          activateOnClick={false}
          enablePointerEvents
          aria-label={t("home.dropFilesHere", "Drop PDF files here")}
          className="home-dropzone"
          styles={{
            root: {
              border: "none !important",
              backgroundColor: "transparent",
              height: "100%",
            },
            inner: {
              height: "100%",
              display: "flex",
              flexDirection: "column",
            },
          }}
        >
          <div className="home-content">
            {/* ── Hero section ─────────────────────── */}
            <div className="home-hero">
              <h1 className="home-hero__title">
                {t("home.hero.title", "Your Documents")}
              </h1>
              <p className="home-hero__subtitle">
                {t(
                  "home.hero.subtitle",
                  "Open a PDF to get started, or choose a tool below.",
                )}
              </p>
              <div className="home-hero__actions">
                <button
                  type="button"
                  style={{
                    display: "inline-flex",
                    alignItems: "center",
                    gap: "0.5rem",
                    padding: "0.5625rem 1.25rem",
                    background: "var(--c-accent, var(--p-doc-accent-primary))",
                    color: "var(--p-doc-surface-chrome, #1e202b)",
                    border: "none",
                    borderRadius: "6px",
                    fontSize: "0.875rem",
                    fontWeight: 600,
                    cursor: "pointer",
                    transition: "opacity 0.15s ease",
                  }}
                  onMouseEnter={(e) => (e.currentTarget.style.opacity = "0.88")}
                  onMouseLeave={(e) => (e.currentTarget.style.opacity = "1")}
                  onClick={(e) => {
                    e.stopPropagation();
                    void handleNativeUploadClick();
                  }}
                >
                  <FileUploadOutlinedIcon style={{ fontSize: "1rem" }} />
                  {t("home.actions.openPDF", "Open PDF")}
                </button>
                <button
                  type="button"
                  style={{
                    display: "inline-flex",
                    alignItems: "center",
                    gap: "0.5rem",
                    padding: "0.5rem 1.125rem",
                    background: "transparent",
                    color: "var(--c-text, var(--p-doc-text-primary))",
                    border:
                      "1px solid var(--c-border, var(--p-doc-surface-hover-selected))",
                    borderRadius: "6px",
                    fontSize: "0.875rem",
                    fontWeight: 500,
                    cursor: "pointer",
                    transition: "background-color 0.15s ease",
                  }}
                  onMouseEnter={(e) =>
                    (e.currentTarget.style.backgroundColor =
                      "var(--c-hover, var(--p-doc-surface-hover-selected))")
                  }
                  onMouseLeave={(e) =>
                    (e.currentTarget.style.backgroundColor = "transparent")
                  }
                  onClick={(e) => {
                    e.stopPropagation();
                    handleShowAllTools();
                  }}
                >
                  <AddRoundedIcon style={{ fontSize: "1rem" }} />
                  {t("home.actions.browseTools", "Browse Tools")}
                </button>
              </div>
            </div>

            {/* ── Recent Documents ─────────────────── */}
            {recentFiles.length > 0 && (
              <section
                className="home-section"
                aria-labelledby="recent-heading"
              >
                <div className="home-section__header">
                  <h2 id="recent-heading" className="home-section__title">
                    {t("home.recentDocuments", "Recent Documents")}
                  </h2>
                  {recentFiles.length >= 8 && (
                    <button
                      type="button"
                      className="home-section__action"
                      onClick={() => console.log("View all recent documents")}
                    >
                      {t("home.viewAll", "View all")}
                    </button>
                  )}
                </div>

                <div className="home-recent-list" role="list">
                  {recentFiles.map((file) => (
                    <button
                      key={file.id as string}
                      type="button"
                      role="listitem"
                      className="home-recent-item"
                      onClick={(e) => {
                        e.stopPropagation();
                        handleOpenRecent(file.id as string);
                      }}
                    >
                      <span className="home-recent-item__icon">
                        <ArticleOutlinedIcon style={{ fontSize: "1.125rem" }} />
                      </span>
                      <span className="home-recent-item__name">
                        {file.name}
                      </span>
                      <span className="home-recent-item__meta">
                        {getFileDate(file)}
                      </span>
                    </button>
                  ))}
                </div>
              </section>
            )}

            {/* ── Quick Tools ──────────────────────── */}
            <section className="home-section" aria-labelledby="tools-heading">
              <div className="home-section__header">
                <h2 id="tools-heading" className="home-section__title">
                  {t("home.quickTools", "Quick Tools")}
                </h2>
                <button
                  type="button"
                  className="home-section__action"
                  onClick={handleShowAllTools}
                >
                  {t("home.viewAllTools", "View all tools")}
                </button>
              </div>

              <div className="home-tools">
                <div className="home-tools__inner">
                  <HomeQuickTools
                    filteredTools={filteredTools}
                    onSelect={handleToolClick}
                  />
                </div>
              </div>
            </section>
          </div>
        </Dropzone>
      </main>

      {/* Hidden file input fallback */}
      <input
        ref={fileInputRef}
        type="file"
        multiple
        accept=".pdf"
        onChange={handleFileSelect}
        style={{ display: "none" }}
      />
    </div>
  );
};

export default LandingPage;
