import React, { useCallback, useRef, useEffect } from "react";
import { useFileState, useFileManagement } from "@app/contexts/FileContext";
import { useViewer } from "@app/contexts/ViewerContext";
import {
  useNavigationState,
  useNavigationActions,
} from "@app/contexts/NavigationContext";
import { isPDFEliteFile } from "@app/types/fileContext";
import type { FileId } from "@app/types/fileContext";
import { Button } from "@app/ui/Button";
import { ActionIcon } from "@app/ui/ActionIcon";
import CloseIcon from "@mui/icons-material/Close";
import "@app/components/viewer/DocumentTabBar.css";

/**
 * DocumentTabBar - renders one tab per active file in FileContext.
 *
 * Clicking a tab instantly switches to that document via ViewerContext.activeFileId.
 * Closing a tab removes the file from FileContext.
 * Rules of hooks: all hooks are called unconditionally before any early-return.
 */
export function DocumentTabBar() {
  // ── All hooks first (no early return before this point) ───────────────────
  const { selectors } = useFileState();
  const { removeFiles } = useFileManagement();
  const { activeFileId, setActiveFileId } = useViewer();
  const { actions: navActions } = useNavigationActions();
  const { workbench } = useNavigationState();
  const tabBarRef = useRef<HTMLDivElement>(null);

  const activeFiles = selectors.getFiles();

  const handleTabClick = useCallback(
    (fileId: FileId) => {
      setActiveFileId(fileId);
      navActions.setWorkbench("viewer");
    },
    [setActiveFileId, navActions],
  );

  const handleTabClose = useCallback(
    (e: React.MouseEvent, fileId: FileId) => {
      e.stopPropagation();
      removeFiles([fileId]);

      // If closing the active tab, switch to another remaining file
      if (activeFileId === fileId) {
        const remaining = activeFiles.filter(
          (f) => isPDFEliteFile(f) && f.fileId !== fileId,
        );
        if (remaining.length > 0 && isPDFEliteFile(remaining[0])) {
          setActiveFileId(remaining[0].fileId);
          navActions.setWorkbench("viewer");
        } else {
          setActiveFileId(null);
        }
      }
    },
    [activeFileId, activeFiles, removeFiles, setActiveFileId, navActions],
  );

  // Scroll the active tab into view when it changes
  useEffect(() => {
    if (!activeFileId || !tabBarRef.current) return;
    const activeEl = tabBarRef.current.querySelector<HTMLElement>(
      `[data-file-id="${activeFileId}"]`,
    );
    activeEl?.scrollIntoView({
      block: "nearest",
      inline: "nearest",
      behavior: "smooth",
    });
  }, [activeFileId]);

  // ── Conditional renders after all hooks ────────────────────────────────────

  // Don't render in page-editor or myFiles views
  if (workbench === "myFiles" || workbench === "pageEditor") {
    return null;
  }

  // Only show when there are documents open
  if (activeFiles.length === 0) {
    return null;
  }

  return (
    <div
      className="document-tab-bar"
      ref={tabBarRef}
      role="tablist"
      aria-label="Open documents"
    >
      {activeFiles.map((file) => {
        if (!isPDFEliteFile(file)) return null;
        const fileId = file.fileId;

        // Look up the stub to get isDirty metadata
        const stub = selectors.getPDFEliteFileStub(fileId);
        const isDirty = stub?.isDirty ?? false;

        // Active if explicitly selected, or auto-select when it's the only tab
        const isActive =
          fileId === activeFileId ||
          (!activeFileId && activeFiles.length === 1);

        // Strip extension for compact tab label
        const displayName = file.name.replace(/\.[^.]+$/u, "");

        return (
          <div
            key={fileId}
            role="tab"
            aria-selected={isActive}
            data-file-id={fileId}
            className={`document-tab${isActive ? " document-tab--active" : ""}`}
          >
            {/* Tab button — takes up available space and triggers navigation */}
            <Button
              variant="quiet"
              accent="neutral"
              size="sm"
              className="document-tab__btn"
              onClick={() => handleTabClick(fileId)}
              title={file.name}
              aria-label={`Switch to ${file.name}`}
            >
              {/* PDF icon */}
              <span className="document-tab__icon" aria-hidden="true">
                <svg
                  width="14"
                  height="14"
                  viewBox="0 0 24 24"
                  fill="currentColor"
                >
                  <path d="M20 2H8c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm-8.5 7.5c0 .83-.67 1.5-1.5 1.5H9v2H7.5V7H10c.83 0 1.5.67 1.5 1.5v1zm5 2c0 .83-.67 1.5-1.5 1.5h-2.5V7H15c.83 0 1.5.67 1.5 1.5v3zm4-3H19v1h1.5V11H19v2h-1.5V7h3v1.5zM9 9.5h1v-1H9v1zM4 6H2v14c0 1.1.9 2 2 2h14v-2H4V6zm10 5.5h1v-3h-1v3z" />
                </svg>
              </span>

              {/* File name */}
              <span className="document-tab__name">{displayName}</span>

              {/* Unsaved-changes indicator */}
              {isDirty && (
                <span
                  className="document-tab__dirty"
                  aria-label="Unsaved changes"
                  title="Unsaved changes"
                >
                  ●
                </span>
              )}
            </Button>

            {/* Close button */}
            <ActionIcon
              variant="quiet"
              size="sm"
              className="document-tab__close"
              onClick={(e) => handleTabClose(e, fileId)}
              aria-label={`Close ${file.name}`}
            >
              <CloseIcon fontSize="inherit" />
            </ActionIcon>
          </div>
        );
      })}
    </div>
  );
}
