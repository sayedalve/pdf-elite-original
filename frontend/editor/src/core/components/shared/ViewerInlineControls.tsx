import { useEffect, useState } from "react";
import { NumberInput, Slider } from "@mantine/core";
import { ActionIcon } from "@app/ui/ActionIcon";
import { useTranslation } from "react-i18next";
import { useViewer } from "@app/contexts/ViewerContext";
import { useNavigationState } from "@app/contexts/NavigationContext";
import ZoomInIcon from "@mui/icons-material/ZoomIn";
import ZoomOutIcon from "@mui/icons-material/ZoomOut";
import KeyboardArrowUpIcon from "@mui/icons-material/KeyboardArrowUp";
import KeyboardArrowDownIcon from "@mui/icons-material/KeyboardArrowDown";
import FitScreenIcon from "@mui/icons-material/FitScreen";
import CenterFocusWeakIcon from "@mui/icons-material/CenterFocusWeak";

/**
 * Compact zoom controls rendered inline in the WorkbenchBar when the current workbench is "viewer".
 */
export function ViewerInlineControls() {
  const { t } = useTranslation();
  const { workbench } = useNavigationState();
  const viewer = useViewer();

  const [zoomPercent, setZoomPercent] = useState(100);
  const [currentPage, setCurrentPage] = useState(1);
  const [totalPages, setTotalPages] = useState(1);

  useEffect(() => {
    const zoomState = viewer.getZoomState();
    setZoomPercent(zoomState.zoomPercent || 100);

    const unregisterZoom = viewer.registerImmediateZoomUpdate((pct) => {
      setZoomPercent(pct);
    });

    const scrollState = viewer.getScrollState();
    setCurrentPage(scrollState.currentPage || 1);
    setTotalPages(scrollState.totalPages || 1);

    const unregisterScroll = viewer.registerImmediateScrollUpdate(
      (page, total) => {
        setCurrentPage(page);
        if (total) setTotalPages(total);
      },
    );

    return () => {
      unregisterZoom?.();
      unregisterScroll?.();
    };
  }, [viewer]);

  if (workbench !== "viewer") return null;

  const sliderValue = Math.min(Math.max(zoomPercent, 20), 500);

  return (
    <div className="viewer-inline-controls">
      <div className="workbench-bar-divider" />

      {/* Page controls */}
      <ActionIcon
        variant="tertiary"
        className="workbench-bar-action-icon"
        onClick={() => viewer.scrollActions.scrollToPage(currentPage - 1)}
        disabled={currentPage <= 1}
        aria-label={t("viewer.prevPage", "Previous page")}
      >
        <KeyboardArrowUpIcon sx={{ fontSize: "1rem" }} />
      </ActionIcon>

      <div
        style={{
          display: "flex",
          alignItems: "center",
          gap: "0.25rem",
          margin: "0 0.25rem",
        }}
      >
        <NumberInput
          value={currentPage}
          min={1}
          max={totalPages}
          hideControls
          size="xs"
          styles={{
            input: {
              width: "3rem",
              textAlign: "center",
              padding: "0 0.25rem",
              height: "1.75rem",
              minHeight: "1.75rem",
            },
          }}
          onChange={(val) => {
            if (typeof val === "number") {
              viewer.scrollActions.scrollToPage(val);
            }
          }}
          aria-label={t("viewer.pageNumber", "Page number")}
        />
        <span
          style={{
            fontSize: "0.75rem",
            color: "var(--c-text-subtle)",
            userSelect: "none",
          }}
        >
          / {totalPages}
        </span>
      </div>

      <ActionIcon
        variant="tertiary"
        className="workbench-bar-action-icon"
        onClick={() => viewer.scrollActions.scrollToPage(currentPage + 1)}
        disabled={currentPage >= totalPages}
        aria-label={t("viewer.nextPage", "Next page")}
      >
        <KeyboardArrowDownIcon sx={{ fontSize: "1rem" }} />
      </ActionIcon>

      <div className="workbench-bar-divider" />

      {/* Zoom controls */}
      <ActionIcon
        variant="tertiary"
        className="workbench-bar-action-icon"
        onClick={() => viewer.zoomActions.zoomOut()}
        aria-label={t("viewer.zoomOut", "Zoom out")}
      >
        <ZoomOutIcon sx={{ fontSize: "1rem" }} />
      </ActionIcon>

      <div className="viewer-inline-controls__slider-wrap">
        <Slider
          value={sliderValue}
          min={20}
          max={500}
          step={5}
          onChange={(val) => {
            viewer.zoomActions.setZoomLevel?.(val / 75);
          }}
          size="xs"
          styles={{
            root: { width: "5rem" },
            thumb: { width: 14, height: 14 },
            track: { height: 3 },
          }}
          label={null}
        />
      </div>

      <ActionIcon
        variant="tertiary"
        className="workbench-bar-action-icon"
        onClick={() => viewer.zoomActions.zoomIn()}
        aria-label={t("viewer.zoomIn", "Zoom in")}
      >
        <ZoomInIcon sx={{ fontSize: "1rem" }} />
      </ActionIcon>

      <span className="viewer-inline-controls__zoom-pct">
        {Math.round(zoomPercent)}%
      </span>

      <ActionIcon
        variant="tertiary"
        className="workbench-bar-action-icon"
        onClick={() => viewer.zoomActions.requestZoom("fitWidth")}
        aria-label={t("viewer.fitWidth", "Fit width")}
        title={t("viewer.fitWidth", "Fit width")}
      >
        <FitScreenIcon sx={{ fontSize: "1rem" }} />
      </ActionIcon>

      <ActionIcon
        variant="tertiary"
        className="workbench-bar-action-icon"
        onClick={() => viewer.zoomActions.requestZoom("fitPage")}
        aria-label={t("viewer.fitPage", "Fit page")}
        title={t("viewer.fitPage", "Fit page")}
      >
        <CenterFocusWeakIcon sx={{ fontSize: "1rem" }} />
      </ActionIcon>

      <ActionIcon
        variant="tertiary"
        className="workbench-bar-action-icon"
        onClick={() => viewer.zoomActions.setZoomLevel?.(100 / 75)}
        aria-label={t("viewer.actualSize", "Actual size")}
        title={t("viewer.actualSize", "Actual size")}
      >
        <span className="viewer-inline-controls__actual-size">100</span>
      </ActionIcon>
    </div>
  );
}
