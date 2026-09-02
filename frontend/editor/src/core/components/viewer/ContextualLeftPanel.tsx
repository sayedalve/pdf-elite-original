import { Box, Tooltip as MantineTooltip } from "@mantine/core";
import { ActionIcon } from "@app/ui/ActionIcon";
import { useTranslation } from "react-i18next";
import { useViewer } from "@app/contexts/ViewerContext";
import LocalIcon from "@app/components/shared/LocalIcon";
import ViewListIcon from "@mui/icons-material/ViewList";
import LayersIcon from "@mui/icons-material/Layers";
import { ThumbnailSidebar } from "@app/components/viewer/ThumbnailSidebar";
import { BookmarkSidebar } from "@app/components/viewer/BookmarkSidebar";
import { AttachmentSidebar } from "@app/components/viewer/AttachmentSidebar";
import { LayerSidebar } from "@app/components/viewer/LayerSidebar";

interface ContextualLeftPanelProps {
  activeFileId?: string | null;
  bookmarkCacheKey: string;
  allBookmarkCacheKeys: string[];
  effectiveFile: any;
  handleLayerApply: (blob: Blob) => Promise<void>;
}

export const CONTEXTUAL_RAIL_WIDTH_REM = 0;

export function ContextualLeftPanel({
  activeFileId,
  bookmarkCacheKey,
  allBookmarkCacheKeys,
  effectiveFile,
  handleLayerApply,
}: ContextualLeftPanelProps) {
  const { t } = useTranslation();
  const viewer = useViewer();
  const {
    isThumbnailSidebarVisible,
    isBookmarkSidebarVisible,
    isAttachmentSidebarVisible,
    isLayerSidebarVisible,
    hasLayers,
    setHasLayers,
  } = viewer;

  const activePanel = isThumbnailSidebarVisible
    ? "thumbnails"
    : isBookmarkSidebarVisible
      ? "bookmarks"
      : isAttachmentSidebarVisible
        ? "attachments"
        : isLayerSidebarVisible
          ? "layers"
          : null;

  const togglePanel = (
    panel: "thumbnails" | "bookmarks" | "attachments" | "layers",
  ) => {
    if (activePanel === panel) {
      if (panel === "thumbnails") viewer.toggleThumbnailSidebar();
      if (panel === "bookmarks") viewer.toggleBookmarkSidebar();
      if (panel === "attachments") viewer.toggleAttachmentSidebar();
      if (panel === "layers") viewer.toggleLayerSidebar();
    } else {
      if (isThumbnailSidebarVisible) viewer.toggleThumbnailSidebar();
      if (isBookmarkSidebarVisible) viewer.toggleBookmarkSidebar();
      if (isAttachmentSidebarVisible) viewer.toggleAttachmentSidebar();
      if (isLayerSidebarVisible) viewer.toggleLayerSidebar();

      if (panel === "thumbnails") viewer.toggleThumbnailSidebar();
      if (panel === "bookmarks") viewer.toggleBookmarkSidebar();
      if (panel === "attachments") viewer.toggleAttachmentSidebar();
      if (panel === "layers") viewer.toggleLayerSidebar();
    }
  };

  const navItems = [
    {
      id: "thumbnails" as const,
      icon: <ViewListIcon fontSize="small" />,
      label: t("workbenchBar.toggleSidebar", "Thumbnails"),
      visible: true,
    },
    {
      id: "bookmarks" as const,
      icon: (
        <LocalIcon
          icon="bookmark-add-rounded"
          width="1.25rem"
          height="1.25rem"
        />
      ),
      label: t("workbenchBar.toggleBookmarks", "Bookmarks"),
      visible: true,
    },
    {
      id: "attachments" as const,
      icon: (
        <LocalIcon icon="attachment-rounded" width="1.25rem" height="1.25rem" />
      ),
      label: t("workbenchBar.toggleAttachments", "Attachments"),
      visible: true,
    },
    {
      id: "layers" as const,
      icon: <LayersIcon sx={{ fontSize: "1rem" }} />,
      label: t("workbenchBar.toggleLayers", "Layers"),
      visible: hasLayers,
    },
  ];

  return (
    <>
      {/* Render Sidebars offset by the rail */}
      <Box
        style={{
          position: "absolute",
          left: `${CONTEXTUAL_RAIL_WIDTH_REM}rem`,
          top: 0,
          bottom: 0,
          zIndex: 998,
        }}
      >
        <ThumbnailSidebar
          visible={activePanel === "thumbnails"}
          onToggle={() => togglePanel("thumbnails")}
          activeFileId={activeFileId}
        />
        <BookmarkSidebar
          visible={activePanel === "bookmarks"}
          thumbnailVisible={false}
          documentCacheKey={bookmarkCacheKey}
          preloadCacheKeys={allBookmarkCacheKeys}
        />
        <AttachmentSidebar
          visible={activePanel === "attachments"}
          thumbnailVisible={false}
          bookmarkVisible={false}
          documentCacheKey={bookmarkCacheKey}
          preloadCacheKeys={allBookmarkCacheKeys}
        />
        <LayerSidebar
          visible={activePanel === "layers"}
          leftOffset={0}
          file={effectiveFile?.file ?? null}
          documentCacheKey={bookmarkCacheKey}
          onApplyLayers={handleLayerApply}
          onLayersDetected={setHasLayers}
        />
      </Box>
    </>
  );
}
