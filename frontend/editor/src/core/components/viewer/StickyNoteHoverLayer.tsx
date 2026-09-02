import React, { useMemo } from "react";
import { useDocumentState } from "@embedpdf/core/react";
import { useAnnotation } from "@embedpdf/plugin-annotation/react";
import { PdfAnnotationSubtype } from "@embedpdf/models";
import { Tooltip } from "@mantine/core";

interface StickyNoteHoverLayerProps {
  documentId: string;
  pageIndex: number;
}

export const StickyNoteHoverLayer: React.FC<StickyNoteHoverLayerProps> = ({
  documentId,
  pageIndex,
}) => {
  const { state, provides } = useAnnotation(documentId);
  const documentState = useDocumentState(documentId);

  // Extract TEXT annotations for this page that have contents
  const textAnnotations = useMemo(() => {
    if (!state) return [];
    const uids = state.pages[pageIndex] ?? [];
    const result = [];
    for (const uid of uids) {
      const ta = state.byUid[uid];
      if (
        ta &&
        ta.commitState !== "deleted" &&
        ta.object.type === PdfAnnotationSubtype.TEXT
      ) {
        if (ta.object.contents && ta.object.contents.trim().length > 0) {
          result.push(ta);
        }
      }
    }
    return result;
  }, [state, pageIndex]);

  // EmbedPDF scale factor (annotation rects are in PDF points at scale 1)
  const scale = documentState?.scale ?? 1;

  if (textAnnotations.length === 0) return null;

  return (
    <div
      className="absolute inset-0"
      style={{ pointerEvents: "none", zIndex: 10 }}
    >
      {textAnnotations.map((ta) => {
        // AnnotationRect is typically bottom-left based in standard PDF logic,
        // but EmbedPDF normalizes origin.x / origin.y for top-left rendering in the DOM.
        const left = ta.object.rect.origin.x * scale;
        const top = ta.object.rect.origin.y * scale;
        const width = ta.object.rect.size.width * scale;
        const height = ta.object.rect.size.height * scale;

        return (
          <Tooltip
            key={ta.object.id}
            label={ta.object.contents}
            multiline
            w={250}
            withArrow
            withinPortal
            openDelay={200}
          >
            <div
              tabIndex={0}
              onClick={(e) => {
                e.stopPropagation();
                e.preventDefault();
                provides?.selectAnnotation?.(pageIndex, ta.object.id);
              }}
              onKeyDown={(e) => {
                if (e.key === "Delete" || e.key === "Backspace") {
                  e.stopPropagation();
                  e.preventDefault();
                  provides?.deleteAnnotation?.(pageIndex, ta.object.id);
                }
              }}
              style={{
                position: "absolute",
                pointerEvents: "auto",
                cursor: "pointer",
                left: `${left}px`,
                top: `${top}px`,
                width: `${width}px`,
                height: `${height}px`,
                minWidth: "24px",
                minHeight: "24px",
              }}
            />
          </Tooltip>
        );
      })}
    </div>
  );
};
