import { Tooltip, Group } from "@mantine/core";
import { ActionIcon } from "@app/ui/ActionIcon";
import ContentCopyIcon from "@mui/icons-material/ContentCopy";
import HighlightIcon from "@mui/icons-material/Highlight";
import FormatUnderlinedIcon from "@mui/icons-material/FormatUnderlined";
import StrikethroughSIcon from "@mui/icons-material/StrikethroughS";
import AddCommentIcon from "@mui/icons-material/AddComment";
import { useCallback, useEffect, useRef, useState } from "react";
import { createPortal } from "react-dom";
import { useTranslation } from "react-i18next";
import type { SelectionSelectionMenuProps } from "@embedpdf/plugin-selection/react";
import { useSelectionCapability } from "@embedpdf/plugin-selection/react";
import { useAnnotationCapability } from "@embedpdf/plugin-annotation/react";

// --- one-time text-markup creation ---------------------------------------
// The selection popup replays EmbedPDF's own text-markup creation logic once,
// directly from the live selection, instead of arming a tool via setActiveTool.
// Arming leaves the tool active ("sticky") so every later selection keeps being
// marked up until Escape — the bug this component used to have.

type FormattedSelectionSnapshot = {
  pageIndex: number;
  rect: unknown;
  segmentRects: unknown;
};

type SelectionTextTask = {
  wait: (
    onOk: (parts: string[]) => void,
    onErr: (reason: unknown) => void,
  ) => void;
};

type SelectionSurface = {
  getFormattedSelection?: () => FormattedSelectionSnapshot[] | undefined;
  getSelectedText?: () => SelectionTextTask | undefined;
  clear?: () => void;
};

type MarkupTool = {
  interaction?: { textSelection?: boolean };
  defaults?: Record<string, unknown>;
};

type AnnotationSurface = {
  getTool?: (toolId: string) => MarkupTool | undefined;
  createAnnotation?: (
    pageIndex: number,
    annotation: Record<string, unknown>,
  ) => void;
  setActiveTool?: (toolId: string | null) => void;
};

const generateAnnotationId = (): string => {
  const c = (globalThis as { crypto?: Crypto }).crypto;
  if (c && typeof c.randomUUID === "function") return c.randomUUID();
  return `anno-${Date.now()}-${Math.random().toString(36).slice(2, 10)}`;
};

export function TextSelectionMenu({
  selected,
  menuWrapperProps,
  placement,
}: SelectionSelectionMenuProps) {
  const { t } = useTranslation();
  const { provides: selection } = useSelectionCapability();
  const { provides: annotationApi } = useAnnotationCapability();
  const wrapperRef = useRef<HTMLDivElement>(null);
  const [position, setPosition] = useState<{
    top: number;
    left: number;
  } | null>(null);

  const setRef = useCallback(
    (node: HTMLDivElement | null) => {
      wrapperRef.current = node;
      menuWrapperProps?.ref?.(node);
    },
    [menuWrapperProps],
  );

  const showAbove = placement?.suggestTop ?? true;

  useEffect(() => {
    if (!selected || !wrapperRef.current) {
      setPosition(null);
      return;
    }
    const update = () => {
      const wrapper = wrapperRef.current;
      if (!wrapper) return;
      const r = wrapper.getBoundingClientRect();
      setPosition({
        top: showAbove ? r.top - 8 : r.bottom + 8,
        left: r.left + r.width / 2,
      });
    };
    update();
    window.addEventListener("scroll", update, true);
    window.addEventListener("resize", update);
    return () => {
      window.removeEventListener("scroll", update, true);
      window.removeEventListener("resize", update);
    };
  }, [selected, showAbove]);

  const handleCopy = useCallback(() => {
    selection?.copyToClipboard();
  }, [selection]);

  const handleTool = useCallback(
    (toolId: string) => {
      const anno = annotationApi as unknown as AnnotationSurface | undefined;
      if (!anno) return;

      const tool = anno.getTool?.(toolId);
      const sel = selection as unknown as SelectionSurface | null;

      // Text-markup tools (highlight / underline / strikeout) are anchored to
      // the live text selection. Rather than ARM the tool (setActiveTool), which
      // leaves it active so every later selection keeps getting marked up until
      // Escape, we create the annotation once — directly from the current
      // selection — mirroring the plugin's own onEndSelection handler. The popup
      // is therefore a true one-time action and never enters a persistent mode.
      // (The toolbar keeps using setActiveTool for its intentional multi-markup
      // mode; that path is untouched.)
      if (tool?.interaction?.textSelection && sel) {
        const regions = sel.getFormattedSelection?.() ?? [];
        if (regions.length === 0) return;

        const defaults = tool.defaults ?? {};

        // Kick off text extraction BEFORE clearing the selection so the task
        // captures the range synchronously — the same ordering the plugin uses
        // (getText() is started, then the selection is cleared). The resolved
        // text is stored on `custom.text`, exactly like gesture-created markup.
        const textTask = sel.getSelectedText?.();
        const textPromise = new Promise<string | undefined>((resolve) => {
          if (!textTask?.wait) {
            resolve(undefined);
            return;
          }
          textTask.wait(
            (parts) =>
              resolve(Array.isArray(parts) ? parts.join("\n") : undefined),
            () => resolve(undefined),
          );
        });

        for (const region of regions) {
          const id = generateAnnotationId();
          void textPromise.then((text) => {
            anno.createAnnotation?.(region.pageIndex, {
              ...defaults,
              rect: region.rect,
              segmentRects: region.segmentRects,
              pageIndex: region.pageIndex,
              created: new Date(),
              id,
              ...(text != null ? { custom: { text } } : {}),
            });
          });
        }

        // Collapse the selection so the popup dismisses and we return to normal
        // viewing. No tool was ever armed, so nothing stays active.
        sel.clear?.();
        return;
      }

      // Non-text-selection tools (e.g. the click-to-place "textComment") keep
      // the arming interaction — the user places them by clicking on the page,
      // which is a different and intended model.
      if (anno.setActiveTool) {
        anno.setActiveTool(null);
        anno.setActiveTool(toolId);
      }
    },
    [annotationApi, selection],
  );

  const portalContent =
    position &&
    createPortal(
      <div
        style={{
          position: "fixed",
          top: position.top,
          left: position.left,
          transform: `translate(-50%, ${showAbove ? "-100%" : "0"})`,
          zIndex: 10000,
          pointerEvents: "auto",
        }}
        onMouseDown={(e) => e.preventDefault()}
      >
        <Group
          gap={4}
          style={{
            backgroundColor: "var(--c-surface-elevated)",
            padding: 4,
            borderRadius: "var(--radius-md)",
            boxShadow: "0 2px 12px rgba(0, 0, 0, 0.25)",
            border: "1px solid var(--c-border)",
          }}
        >
          <Tooltip label={t("annotation.highlight", "Highlight")} withArrow>
            <ActionIcon
              variant="quiet"
              accent="neutral"
              size="md"
              onClick={() => handleTool("highlight")}
              aria-label={t("annotation.highlight", "Highlight")}
            >
              <HighlightIcon style={{ fontSize: 18 }} />
            </ActionIcon>
          </Tooltip>

          <Tooltip label={t("annotation.underline", "Underline")} withArrow>
            <ActionIcon
              variant="quiet"
              accent="neutral"
              size="md"
              onClick={() => handleTool("underline")}
              aria-label={t("annotation.underline", "Underline")}
            >
              <FormatUnderlinedIcon style={{ fontSize: 18 }} />
            </ActionIcon>
          </Tooltip>

          <Tooltip label={t("annotation.strikeout", "Strikeout")} withArrow>
            <ActionIcon
              variant="quiet"
              accent="neutral"
              size="md"
              onClick={() => handleTool("strikeout")}
              aria-label={t("annotation.strikeout", "Strikeout")}
            >
              <StrikethroughSIcon style={{ fontSize: 18 }} />
            </ActionIcon>
          </Tooltip>

          <Tooltip label={t("annotation.comment", "Comment")} withArrow>
            <ActionIcon
              variant="quiet"
              accent="neutral"
              size="md"
              onClick={() => handleTool("textComment")}
              aria-label={t("annotation.comment", "Comment")}
            >
              <AddCommentIcon style={{ fontSize: 18 }} />
            </ActionIcon>
          </Tooltip>

          <div
            style={{
              width: 1,
              height: 20,
              backgroundColor: "var(--c-border)",
              margin: "0 4px",
            }}
          />

          <Tooltip label={t("viewer.copyText", "Copy")} withArrow>
            <ActionIcon
              variant="quiet"
              accent="neutral"
              size="md"
              onClick={handleCopy}
              aria-label={t("viewer.copyText", "Copy")}
            >
              <ContentCopyIcon style={{ fontSize: 18 }} />
            </ActionIcon>
          </Tooltip>
        </Group>
      </div>,
      document.body,
    );

  return (
    <>
      <div ref={setRef} style={menuWrapperProps?.style} />
      {portalContent}
    </>
  );
}
