import { useMemo, useState, useEffect, useCallback } from "react";
import { Slider, Popover, Select } from "@mantine/core";
import { ActionIcon } from "@app/ui/ActionIcon";
import { useTranslation } from "react-i18next";
import { supportedLanguages } from "@app/i18n";
import { useViewer } from "@app/contexts/ViewerContext";
import {
  useWorkbenchBarButtons,
  WorkbenchBarButtonWithAction,
} from "@app/hooks/useWorkbenchBarButtons";
import LocalIcon from "@app/components/shared/LocalIcon";
import { Tooltip } from "@app/components/shared/Tooltip";
import { SearchInterface } from "@app/components/viewer/SearchInterface";
import ViewerAnnotationControls from "@app/components/viewer/ViewerAnnotationControls";
import { useSidebarContext } from "@app/contexts/SidebarContext";
import { useWorkbenchBarTooltipSide } from "@app/hooks/useWorkbenchBarTooltipSide";
import { useToolWorkflow } from "@app/contexts/ToolWorkflowContext";
import { useNavigationState } from "@app/contexts/NavigationContext";
import { stripBasePath } from "@app/constants/app";
import { useRedaction, useRedactionMode } from "@app/contexts/RedactionContext";
import { useAnnotation } from "@app/contexts/AnnotationContext";
import TextFieldsIcon from "@mui/icons-material/TextFields";
import StraightenIcon from "@mui/icons-material/Straighten";
import VolumeUpIcon from "@mui/icons-material/VolumeUp";
import StopIcon from "@mui/icons-material/Stop";
import ViewWeekIcon from "@mui/icons-material/ViewWeek";
import DescriptionIcon from "@mui/icons-material/Description";
import DarkModeIcon from "@mui/icons-material/DarkMode";
import WbSunnyIcon from "@mui/icons-material/WbSunny";
import WbTwilightIcon from "@mui/icons-material/WbTwilight";
import { Group } from "@mantine/core";
import { useViewerReadAloud } from "@app/components/viewer/useViewerReadAloud";

export function useViewerWorkbenchBarButtons(
  isRulerActive?: boolean,
  setIsRulerActive?: (v: boolean) => void,
) {
  const { t, i18n } = useTranslation();
  const viewer = useViewer();
  const {
    isThumbnailSidebarVisible,
    isBookmarkSidebarVisible,
    isAttachmentSidebarVisible,
    isLayerSidebarVisible,
    hasLayers,
    isCommentsSidebarVisible,
    toggleCommentsSidebar,
    isSearchInterfaceVisible,
    registerImmediatePanUpdate,
    registerImmediateSpreadUpdate,
    getSpreadState,
    spreadActions,
    pdfRenderMode,
    cyclePdfRenderMode,
  } = viewer;
  const [isPanning, setIsPanning] = useState<boolean>(false);
  const [isDualPageActive, setIsDualPageActive] = useState<boolean>(
    getSpreadState().isDualPage,
  );
  const { sidebarRefs } = useSidebarContext();
  const { position: tooltipPosition } = useWorkbenchBarTooltipSide(
    sidebarRefs,
    12,
  );
  const { handleToolSelect, handleBackToTools } = useToolWorkflow();
  const { selectedTool } = useNavigationState();
  const { pendingCount } = useRedaction();
  const { activateAnnotationToolRef, activeAnnotationToolId } = useAnnotation();
  const { activeType: redactionActiveType } = useRedactionMode();

  const {
    isReadingAloud,
    speechRate,
    speechLanguage,
    speechVoice,
    supportedLanguageCodes,
    handleReadAloud,
    handleSpeechRateChange,
    handleSpeechLanguageChange,
  } = useViewerReadAloud(i18n.language || "en-US");

  useEffect(() => {
    return registerImmediatePanUpdate((newIsPanning) => {
      setIsPanning(newIsPanning);
    });
  }, [registerImmediatePanUpdate]);

  useEffect(() => {
    return registerImmediateSpreadUpdate((_mode, isDualPage) => {
      setIsDualPageActive(isDualPage);
    });
  }, [registerImmediateSpreadUpdate]);

  const isAnnotationsPath = useCallback(() => {
    const cleanPath = stripBasePath(window.location.pathname).toLowerCase();
    return cleanPath === "/annotations" || cleanPath.endsWith("/annotations");
  }, []);

  const [isAnnotationsActive, setIsAnnotationsActive] = useState<boolean>(() =>
    isAnnotationsPath(),
  );

  useEffect(() => {
    if (selectedTool === "annotate") {
      setIsAnnotationsActive(true);
    } else if (selectedTool) {
      setIsAnnotationsActive(false);
    } else {
      setIsAnnotationsActive(isAnnotationsPath());
    }
  }, [selectedTool, isAnnotationsPath]);

  useEffect(() => {
    const handlePopState = () => setIsAnnotationsActive(isAnnotationsPath());
    window.addEventListener("popstate", handlePopState);
    return () => window.removeEventListener("popstate", handlePopState);
  }, [isAnnotationsPath]);

  const searchLabel = t("workbenchBar.search", "Search PDF");
  const panLabel = t("workbenchBar.panMode", "Pan Mode");
  const applyRedactionsLabel = t(
    "workbenchBar.applyRedactionsFirst",
    "Apply redactions first",
  );
  const rotateLeftLabel = t("workbenchBar.rotateLeft", "Rotate Left");
  const rotateRightLabel = t("workbenchBar.rotateRight", "Rotate Right");
  const sidebarLabel = t("workbenchBar.toggleSidebar", "Toggle Sidebar");
  const bookmarkLabel = t("workbenchBar.toggleBookmarks", "Toggle Bookmarks");
  const attachmentLabel = t(
    "workbenchBar.toggleAttachments",
    "Toggle Attachments",
  );
  const layersLabel = t("workbenchBar.toggleLayers", "Toggle Layers");
  const commentsLabel = t("workbenchBar.toggleComments", "Comments");
  const annotationsLabel = t("workbenchBar.annotations", "Annotations");
  const formFillLabel = t("workbenchBar.formFill", "Fill Form");
  const rulerLabel = t("workbenchBar.ruler", "Ruler / Measure");
  const readAloudLabel = t("workbenchBar.readAloud", "Read Aloud");
  const readAloudSpeedLabel = t("workbenchBar.readAloudSpeed", "Speed");

  const isFormFillActive = (selectedTool as string) === "formFill";

  // Filter languages based on available voices
  const filteredLanguages = useMemo(
    () =>
      Object.entries(supportedLanguages)
        .filter(
          ([code]) =>
            supportedLanguageCodes.size === 0 ||
            supportedLanguageCodes.has(code) ||
            supportedLanguageCodes.has(code.split("-")[0]),
        )
        .map(([code, label]) => ({
          value: code,
          label: label,
        })),
    [supportedLanguageCodes],
  );

  const shouldShowLanguageSelector =
    supportedLanguageCodes.size === 0 || filteredLanguages.length > 1;

  const viewerButtons = useMemo<WorkbenchBarButtonWithAction[]>(() => {
    const buttons: WorkbenchBarButtonWithAction[] = [
      {
        id: "viewer-search",
        tooltip: searchLabel,
        section: "search" as const,
        ariaLabel: searchLabel,
        order: 10,
        render: ({ disabled }) => (
          <Tooltip
            content={searchLabel}
            position={tooltipPosition}
            offset={12}
            arrow
            portalTarget={document.body}
          >
            <Popover
              position={tooltipPosition}
              withArrow
              shadow="md"
              offset={8}
              opened={isSearchInterfaceVisible}
              onClose={viewer.searchInterfaceActions.close}
            >
              <Popover.Target>
                <div style={{ display: "inline-flex" }}>
                  <ActionIcon
                    variant="tertiary"
                    className="workbench-bar-action-icon"
                    disabled={disabled}
                    aria-label={searchLabel}
                    onClick={viewer.searchInterfaceActions.toggle}
                  >
                    <LocalIcon icon="search" width="1rem" height="1rem" />
                  </ActionIcon>
                </div>
              </Popover.Target>
              <Popover.Dropdown>
                <div style={{ minWidth: "20rem" }}>
                  <SearchInterface
                    visible={isSearchInterfaceVisible}
                    onClose={viewer.searchInterfaceActions.close}
                  />
                </div>
              </Popover.Dropdown>
            </Popover>
          </Tooltip>
        ),
      },
      {
        id: "viewer-pan-mode",
        icon: <LocalIcon icon="pan-tool-rounded" width="1rem" height="1rem" />,
        section: "view" as const,
        tooltip:
          !isPanning && pendingCount > 0 && redactionActiveType !== null
            ? applyRedactionsLabel
            : panLabel,
        ariaLabel:
          !isPanning && pendingCount > 0 && redactionActiveType !== null
            ? applyRedactionsLabel
            : panLabel,
        order: 20,
        active: isPanning,
        disabled:
          !isPanning && pendingCount > 0 && redactionActiveType !== null,
        onClick: () => {
          viewer.panActions.togglePan();
          setIsPanning((prev) => {
            const next = !prev;
            if (next && isRulerActive) setIsRulerActive?.(false);
            return next;
          });
        },
      },
      {
        id: "viewer-ruler",
        icon: <StraightenIcon sx={{ fontSize: "1rem" }} />,
        section: "view" as const,
        tooltip: rulerLabel,
        ariaLabel: rulerLabel,
        order: 25,
        active: Boolean(isRulerActive),
        onClick: () => {
          const next = !isRulerActive;
          setIsRulerActive?.(next);
          if (next && isPanning) {
            viewer.panActions.disablePan();
            setIsPanning(false);
          }
        },
      },
      {
        id: "viewer-spread-mode",
        section: "view" as const,
        tooltip: isDualPageActive
          ? t("viewer.singlePageView", "Single Page View")
          : t("viewer.dualPageView", "Dual Page View"),
        ariaLabel: isDualPageActive
          ? t("viewer.singlePageView", "Single Page View")
          : t("viewer.dualPageView", "Dual Page View"),
        order: 28,
        onClick: () => spreadActions.toggleSpreadMode(),
        render: ({ disabled }) => (
          <Tooltip
            content={
              isDualPageActive
                ? t("viewer.singlePageView", "Single Page View")
                : t("viewer.dualPageView", "Dual Page View")
            }
            position={tooltipPosition}
            arrow
          >
            <ActionIcon
              variant={isDualPageActive ? "primary" : "tertiary"}
              className="workbench-bar-action-icon"
              onClick={() => spreadActions.toggleSpreadMode()}
              disabled={disabled}
              aria-label={
                isDualPageActive
                  ? t("viewer.singlePageView", "Single Page View")
                  : t("viewer.dualPageView", "Dual Page View")
              }
            >
              {isDualPageActive ? (
                <DescriptionIcon fontSize="small" />
              ) : (
                <ViewWeekIcon fontSize="small" />
              )}
            </ActionIcon>
          </Tooltip>
        ),
      },
      {
        id: "viewer-pdf-render-mode",
        section: "view" as const,
        tooltip:
          pdfRenderMode === "normal"
            ? t("viewer.enableDarkFilter", "Enable Dark Filter")
            : pdfRenderMode === "dark"
              ? t("viewer.enableSepiaFilter", "Enable Sepia Filter")
              : t("viewer.disableColorFilter", "Disable Color Filter"),
        ariaLabel:
          pdfRenderMode === "normal"
            ? t("viewer.enableDarkFilter", "Enable Dark Filter")
            : pdfRenderMode === "dark"
              ? t("viewer.enableSepiaFilter", "Enable Sepia Filter")
              : t("viewer.disableColorFilter", "Disable Color Filter"),
        order: 29,
        onClick: () => cyclePdfRenderMode(),
        render: ({ disabled }) => (
          <Tooltip
            content={
              pdfRenderMode === "normal"
                ? t("viewer.enableDarkFilter", "Enable Dark Filter")
                : pdfRenderMode === "dark"
                  ? t("viewer.enableSepiaFilter", "Enable Sepia Filter")
                  : t("viewer.disableColorFilter", "Disable Color Filter")
            }
            position={tooltipPosition}
            arrow
          >
            <ActionIcon
              variant={pdfRenderMode !== "normal" ? "primary" : "tertiary"}
              className="workbench-bar-action-icon"
              onClick={() => cyclePdfRenderMode()}
              disabled={disabled}
              aria-label={
                pdfRenderMode === "normal"
                  ? t("viewer.enableDarkFilter", "Enable Dark Filter")
                  : pdfRenderMode === "dark"
                    ? t("viewer.enableSepiaFilter", "Enable Sepia Filter")
                    : t("viewer.disableColorFilter", "Disable Color Filter")
              }
            >
              {pdfRenderMode === "normal" && <DarkModeIcon fontSize="small" />}
              {pdfRenderMode === "dark" && <WbTwilightIcon fontSize="small" />}
              {pdfRenderMode === "sepia" && <WbSunnyIcon fontSize="small" />}
            </ActionIcon>
          </Tooltip>
        ),
      },
      {
        id: "viewer-rotate-left",
        icon: <LocalIcon icon="rotate-left" width="1rem" height="1rem" />,
        tooltip: rotateLeftLabel,
        ariaLabel: rotateLeftLabel,
        section: "organize" as const,
        order: 30,
        onClick: () => {
          viewer.rotationActions.rotateBackward();
        },
      },
      {
        id: "viewer-rotate-right",
        icon: <LocalIcon icon="rotate-right" width="1rem" height="1rem" />,
        tooltip: rotateRightLabel,
        ariaLabel: rotateRightLabel,
        section: "organize" as const,
        order: 40,
        onClick: () => {
          viewer.rotationActions.rotateForward();
        },
      },
      // Sidebar toggles have been moved to the Contextual Left Panel
      {
        id: "viewer-toggle-comments",
        icon: <LocalIcon icon="comment" width="1rem" height="1rem" />,
        tooltip: commentsLabel,
        ariaLabel: commentsLabel,
        section: "navigation" as const,
        order: 56.5,
        active: isCommentsSidebarVisible,
        onClick: () => {
          toggleCommentsSidebar();
        },
      },
      {
        id: "viewer-read-aloud",
        tooltip: readAloudLabel,
        ariaLabel: readAloudLabel,
        section: "view" as const,
        order: 57,
        active: isReadingAloud,
        render: ({ disabled }) => (
          <Popover
            position={tooltipPosition}
            withArrow
            shadow="md"
            offset={8}
            opened={isReadingAloud}
            onClose={() => {}}
            withinPortal
          >
            <Popover.Target>
              <div style={{ display: "inline-flex" }}>
                <Tooltip
                  content={readAloudLabel}
                  position={tooltipPosition}
                  offset={12}
                  arrow
                  portalTarget={document.body}
                >
                  <ActionIcon
                    variant={isReadingAloud ? "primary" : "tertiary"}
                    className="workbench-bar-action-icon"
                    disabled={
                      disabled ||
                      typeof window === "undefined" ||
                      !window.speechSynthesis
                    }
                    aria-label={readAloudLabel}
                    onClick={handleReadAloud}
                  >
                    {isReadingAloud ? (
                      <StopIcon sx={{ fontSize: "1rem" }} />
                    ) : (
                      <VolumeUpIcon sx={{ fontSize: "1rem" }} />
                    )}
                  </ActionIcon>
                </Tooltip>
              </div>
            </Popover.Target>
            <Popover.Dropdown>
              <div style={{ width: "16rem", padding: "0.5rem" }}>
                <div
                  style={{
                    fontSize: "0.75rem",
                    marginBottom: "0.5rem",
                    textAlign: "center",
                  }}
                >
                  {readAloudSpeedLabel}: {speechRate.toFixed(1)}x
                </div>
                <Slider
                  value={speechRate}
                  onChange={handleSpeechRateChange}
                  min={0.5}
                  max={2}
                  step={0.1}
                  marks={[
                    { value: 0.5, label: "0.5x" },
                    { value: 1, label: "1x" },
                    { value: 2, label: "2x" },
                  ]}
                  styles={{
                    markLabel: { fontSize: "0.6rem" },
                  }}
                  mb="md"
                />
                {shouldShowLanguageSelector && (
                  <Select
                    label={t("workbenchBar.readAloudLanguage", "Language")}
                    placeholder={t(
                      "workbenchBar.selectLanguage",
                      "Select language",
                    )}
                    value={speechLanguage}
                    onChange={(value) => {
                      if (value) {
                        handleSpeechLanguageChange(value);
                      }
                    }}
                    data={filteredLanguages}
                    size="xs"
                    searchable
                    mb="sm"
                  />
                )}
              </div>
            </Popover.Dropdown>
          </Popover>
        ),
      },
      {
        id: "viewer-annotate-highlight",
        tooltip: t("workbenchBar.highlight", "Highlight"),
        ariaLabel: t("workbenchBar.highlight", "Highlight"),
        section: "annotate" as const,
        order: 57,
        active: isAnnotationsActive && activeAnnotationToolId === "highlight",
        render: ({ disabled }) => {
          const isActive =
            isAnnotationsActive && activeAnnotationToolId === "highlight";
          const [highlightColor, setHighlightColor] = useState(
            () =>
              localStorage.getItem("pdf-elite-highlight-color") || "#ffd54f",
          );
          const [isColorPickerOpen, setIsColorPickerOpen] = useState(false);

          useEffect(() => {
            const handleStorageChange = (e: any) => {
              if (e.detail?.type === "highlight" && e.detail?.color) {
                setHighlightColor(e.detail.color);
              }
            };
            window.addEventListener(
              "pdf-elite-color-change",
              handleStorageChange,
            );
            return () =>
              window.removeEventListener(
                "pdf-elite-color-change",
                handleStorageChange,
              );
          }, []);

          const handleColorSelect = (color: string) => {
            setHighlightColor(color);
            localStorage.setItem("pdf-elite-highlight-color", color);
            window.dispatchEvent(
              new CustomEvent("pdf-elite-color-change", {
                detail: { type: "highlight", color },
              }),
            );
            setIsColorPickerOpen(false);
            if (isActive) {
              activateAnnotationToolRef.current?.("highlight"); // Reactivate to apply color
            }
          };

          return (
            <Popover
              opened={isColorPickerOpen}
              onChange={setIsColorPickerOpen}
              position={tooltipPosition}
              withArrow
              shadow="md"
              offset={4}
            >
              <Popover.Target>
                <div style={{ display: "inline-flex" }}>
                  <Tooltip
                    content={t("workbenchBar.highlight", "Highlight")}
                    position={tooltipPosition}
                    offset={12}
                    arrow
                    portalTarget={document.body}
                  >
                    <div style={{ position: "relative" }}>
                      <ActionIcon
                        variant={isActive ? "primary" : "tertiary"}
                        className="workbench-bar-action-icon"
                        onClick={() => {
                          if (isActive) {
                            setIsColorPickerOpen((prev) => !prev);
                          } else {
                            activateAnnotationToolRef.current?.("highlight");
                          }
                        }}
                        onContextMenu={(e) => {
                          e.preventDefault();
                          setIsColorPickerOpen(true);
                        }}
                        disabled={disabled}
                        aria-pressed={isActive}
                        aria-label={t("workbenchBar.highlight", "Highlight")}
                      >
                        <LocalIcon
                          icon="ink-highlighter"
                          width="1.25rem"
                          height="1.25rem"
                        />
                      </ActionIcon>
                      <div
                        style={{
                          position: "absolute",
                          bottom: 4,
                          right: 4,
                          width: 8,
                          height: 8,
                          borderRadius: "50%",
                          backgroundColor: highlightColor,
                          border: "1px solid rgba(0,0,0,0.2)",
                          pointerEvents: "none",
                        }}
                      />
                    </div>
                  </Tooltip>
                </div>
              </Popover.Target>
              <Popover.Dropdown>
                <Group gap="xs" p={4} w={140} justify="center">
                  {[
                    "#ffd54f",
                    "#81c784",
                    "#64b5f6",
                    "#e57373",
                    "#ba68c8",
                    "#ffb74d",
                  ].map((color) => (
                    <ActionIcon
                      key={color}
                      variant="quiet"
                      onClick={() => handleColorSelect(color)}
                      aria-label={`Color ${color}`}
                      style={{
                        width: 24,
                        height: 24,
                        borderRadius: "50%",
                        backgroundColor: color,
                        border:
                          highlightColor === color
                            ? "2px solid var(--c-border-active, #3b82f6)"
                            : "1px solid var(--c-border-subtle)",
                      }}
                    />
                  ))}
                </Group>
              </Popover.Dropdown>
            </Popover>
          );
        },
      },
      {
        id: "viewer-annotate-note",
        tooltip: t("workbenchBar.note", "Sticky Note"),
        ariaLabel: t("workbenchBar.note", "Sticky Note"),
        section: "annotate" as const,
        order: 57.1,
        active: isAnnotationsActive && activeAnnotationToolId === "note",
        render: ({ disabled }) => {
          const isActive =
            isAnnotationsActive && activeAnnotationToolId === "note";
          const [noteColor, setNoteColor] = useState(
            () => localStorage.getItem("pdf-elite-note-color") || "#ffd54f",
          );
          const [isColorPickerOpen, setIsColorPickerOpen] = useState(false);

          useEffect(() => {
            const handleStorageChange = (e: any) => {
              if (e.detail?.type === "note" && e.detail?.color) {
                setNoteColor(e.detail.color);
              }
            };
            window.addEventListener(
              "pdf-elite-color-change",
              handleStorageChange,
            );
            return () =>
              window.removeEventListener(
                "pdf-elite-color-change",
                handleStorageChange,
              );
          }, []);

          const handleColorSelect = (color: string) => {
            setNoteColor(color);
            localStorage.setItem("pdf-elite-note-color", color);
            window.dispatchEvent(
              new CustomEvent("pdf-elite-color-change", {
                detail: { type: "note", color },
              }),
            );
            setIsColorPickerOpen(false);
            if (isActive) {
              activateAnnotationToolRef.current?.("note");
            }
          };

          return (
            <Popover
              opened={isColorPickerOpen}
              onChange={setIsColorPickerOpen}
              position={tooltipPosition}
              withArrow
              shadow="md"
              offset={4}
            >
              <Popover.Target>
                <div style={{ display: "inline-flex" }}>
                  <Tooltip
                    content={t("workbenchBar.note", "Sticky Note")}
                    position={tooltipPosition}
                    offset={12}
                    arrow
                    portalTarget={document.body}
                  >
                    <div style={{ position: "relative" }}>
                      <ActionIcon
                        variant={isActive ? "primary" : "tertiary"}
                        className="workbench-bar-action-icon"
                        onClick={() => {
                          if (isActive) {
                            setIsColorPickerOpen((prev) => !prev);
                          } else {
                            activateAnnotationToolRef.current?.("note");
                          }
                        }}
                        onContextMenu={(e) => {
                          e.preventDefault();
                          setIsColorPickerOpen(true);
                        }}
                        disabled={disabled}
                        aria-pressed={isActive}
                        aria-label={t("workbenchBar.note", "Sticky Note")}
                      >
                        <LocalIcon
                          icon="sticky-note-2"
                          width="1.25rem"
                          height="1.25rem"
                        />
                      </ActionIcon>
                      <div
                        style={{
                          position: "absolute",
                          bottom: 4,
                          right: 4,
                          width: 8,
                          height: 8,
                          borderRadius: "50%",
                          backgroundColor: noteColor,
                          border: "1px solid rgba(0,0,0,0.2)",
                          pointerEvents: "none",
                        }}
                      />
                    </div>
                  </Tooltip>
                </div>
              </Popover.Target>
              <Popover.Dropdown>
                <Group gap="xs" p={4} w={140} justify="center">
                  {[
                    "#ffd54f",
                    "#81c784",
                    "#64b5f6",
                    "#e57373",
                    "#ba68c8",
                    "#ffb74d",
                  ].map((color) => (
                    <ActionIcon
                      key={color}
                      variant="quiet"
                      onClick={() => handleColorSelect(color)}
                      aria-label={`Color ${color}`}
                      style={{
                        width: 24,
                        height: 24,
                        borderRadius: "50%",
                        backgroundColor: color,
                        border:
                          noteColor === color
                            ? "2px solid var(--c-border-active, #3b82f6)"
                            : "1px solid var(--c-border-subtle)",
                      }}
                    />
                  ))}
                </Group>
              </Popover.Dropdown>
            </Popover>
          );
        },
      },
      {
        id: "viewer-annotate-text",
        tooltip: t("workbenchBar.text", "Text Comment"),
        ariaLabel: t("workbenchBar.text", "Text Comment"),
        section: "annotate" as const,
        order: 57.2,
        active: isAnnotationsActive && activeAnnotationToolId === "text",
        render: ({ disabled }) => (
          <Tooltip
            content={t("workbenchBar.text", "Text Comment")}
            position={tooltipPosition}
            offset={12}
            arrow
            portalTarget={document.body}
          >
            <ActionIcon
              variant={
                isAnnotationsActive && activeAnnotationToolId === "text"
                  ? "primary"
                  : "tertiary"
              }
              className="workbench-bar-action-icon"
              onClick={() => activateAnnotationToolRef.current?.("text")}
              disabled={disabled}
              aria-pressed={
                isAnnotationsActive && activeAnnotationToolId === "text"
              }
              aria-label={t("workbenchBar.text", "Text Comment")}
            >
              <LocalIcon icon="match-case" width="1.25rem" height="1.25rem" />
            </ActionIcon>
          </Tooltip>
        ),
      },
      {
        id: "viewer-annotate-ink",
        tooltip: t("workbenchBar.ink", "Drawing/Pen"),
        ariaLabel: t("workbenchBar.ink", "Drawing/Pen"),
        section: "annotate" as const,
        order: 57.3,
        active: isAnnotationsActive && activeAnnotationToolId === "ink",
        render: ({ disabled }) => (
          <Tooltip
            content={t("workbenchBar.ink", "Drawing/Pen")}
            position={tooltipPosition}
            offset={12}
            arrow
            portalTarget={document.body}
          >
            <ActionIcon
              variant={
                isAnnotationsActive && activeAnnotationToolId === "ink"
                  ? "primary"
                  : "tertiary"
              }
              className="workbench-bar-action-icon"
              onClick={() => activateAnnotationToolRef.current?.("ink")}
              disabled={disabled}
              aria-pressed={
                isAnnotationsActive && activeAnnotationToolId === "ink"
              }
              aria-label={t("workbenchBar.ink", "Drawing/Pen")}
            >
              <LocalIcon icon="edit" width="1.25rem" height="1.25rem" />
            </ActionIcon>
          </Tooltip>
        ),
      },
      {
        id: "viewer-annotation-controls",
        section: "annotate" as const,
        order: 60,
        render: ({ disabled }) => (
          <ViewerAnnotationControls currentView="viewer" disabled={disabled} />
        ),
      },
      {
        id: "viewer-form-fill",
        tooltip: formFillLabel,
        ariaLabel: formFillLabel,
        section: "annotate" as const,
        order: 62,
        render: ({ disabled }) => (
          <Tooltip
            content={formFillLabel}
            position={tooltipPosition}
            offset={12}
            arrow
            portalTarget={document.body}
          >
            <ActionIcon
              variant={isFormFillActive ? "primary" : "tertiary"}
              className="workbench-bar-action-icon"
              onClick={() => {
                if (disabled) return;
                if (isFormFillActive) {
                  handleBackToTools();
                } else {
                  handleToolSelect("formFill" as any);
                }
              }}
              disabled={disabled}
              aria-pressed={isFormFillActive}
              aria-label={formFillLabel}
            >
              <TextFieldsIcon sx={{ fontSize: "1rem" }} />
            </ActionIcon>
          </Tooltip>
        ),
      },
    ];

    return buttons;
  }, [
    t,
    i18n.language,
    viewer,
    isThumbnailSidebarVisible,
    isBookmarkSidebarVisible,
    isAttachmentSidebarVisible,
    isLayerSidebarVisible,
    hasLayers,
    isSearchInterfaceVisible,
    isPanning,
    searchLabel,
    panLabel,
    applyRedactionsLabel,
    rotateLeftLabel,
    rotateRightLabel,
    sidebarLabel,
    bookmarkLabel,
    attachmentLabel,
    layersLabel,
    tooltipPosition,
    annotationsLabel,
    isAnnotationsActive,
    handleToolSelect,
    pendingCount,
    redactionActiveType,
    formFillLabel,
    isFormFillActive,
    rulerLabel,
    isRulerActive,
    setIsRulerActive,
    readAloudLabel,
    readAloudSpeedLabel,
    isReadingAloud,
    speechRate,
    speechLanguage,
    speechVoice,
    supportedLanguageCodes,
    filteredLanguages,
    shouldShowLanguageSelector,
    handleReadAloud,
    handleSpeechRateChange,
    handleSpeechLanguageChange,
  ]);

  useWorkbenchBarButtons(viewerButtons);
}
