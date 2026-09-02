import { useEffect, useRef } from "react";
import { AnnotationAPI } from "@app/components/viewer/viewerTypes";

export function useGlobalPaste(annotationApiRef?: React.RefObject<AnnotationAPI | null>) {
  // Track last mouse position to determine where to place pasted content
  const lastMousePosRef = useRef<{ clientX: number; clientY: number } | null>(null);

  useEffect(() => {
    const onMove = (e: MouseEvent) => {
      lastMousePosRef.current = { clientX: e.clientX, clientY: e.clientY };
    };
    window.addEventListener("mousemove", onMove);
    return () => window.removeEventListener("mousemove", onMove);
  }, []);

  useEffect(() => {
    const handlePaste = async (e: ClipboardEvent) => {
      const activeEl = document.activeElement as HTMLElement;
      // Do not intercept if user is typing in an input
      if (
        activeEl &&
        (activeEl.tagName === "INPUT" ||
          activeEl.tagName === "TEXTAREA" ||
          activeEl.isContentEditable)
      ) {
        return;
      }

      if (!e.clipboardData) return;

      const pos = lastMousePosRef.current;
      if (!pos) return;

      // Ensure we are hovering over the viewer
      const elements = document.elementsFromPoint(pos.clientX, pos.clientY);
      const pageEl = elements.find((el) => el.hasAttribute("data-page-index")) as HTMLElement;
      
      let pageIndex = 0;
      let pdfX = 100;
      let pdfY = 100;
      let scale = 1;

      if (pageEl) {
        pageIndex = parseInt(pageEl.getAttribute("data-page-index") || "0", 10);
        const rect = pageEl.getBoundingClientRect();
        // offsetWidth is the unscaled CSS size (usually ~595)
        scale = rect.width / (pageEl.offsetWidth || 595.28);
        pdfX = (pos.clientX - rect.left) / scale;
        // PDF coordinate origin is bottom-left
        pdfY = (rect.bottom - pos.clientY) / scale;
      } else {
        // If the user isn't hovering a page, don't paste
        return;
      }

      // 1. Check for images
      for (const item of Array.from(e.clipboardData.items)) {
        if (item.type.startsWith("image/")) {
          e.preventDefault();
          const file = item.getAsFile();
          if (!file) continue;

          const reader = new FileReader();
          reader.onload = (ev) => {
            const dataUrl = ev.target?.result as string;
            if (!dataUrl) return;

            const img = new Image();
            img.onload = () => {
              // Scale down large images to fit reasonably within page (~200pt default max)
              let width = img.width;
              let height = img.height;
              const maxDim = 200;
              if (width > maxDim || height > maxDim) {
                const ratio = Math.min(maxDim / width, maxDim / height);
                width *= ratio;
                height *= ratio;
              }
              
              const pasted = {
                id: crypto.randomUUID(),
                type: 13, // STAMP
                imageSrc: dataUrl,
                rect: { origin: { x: pdfX, y: pdfY }, size: { width, height } },
                opacity: 1,
              };
              annotationApiRef?.current?.createAnnotation?.(pageIndex, pasted);
            };
            img.src = dataUrl;
          };
          reader.readAsDataURL(file);
          return;
        }
      }

      // 2. Check for text
      const text = e.clipboardData.getData("text/plain");
      if (text) {
        e.preventDefault();
        const pasted = {
          id: crypto.randomUUID(),
          type: 3, // FREETEXT
          contents: text,
          rect: { origin: { x: pdfX, y: pdfY }, size: { width: 150, height: 50 } },
          textColor: "#111111",
          fontColor: "#111111",
          fontSize: 12,
        };
        annotationApiRef?.current?.createAnnotation?.(pageIndex, pasted);
      }
    };

    window.addEventListener("paste", handlePaste);
    return () => window.removeEventListener("paste", handlePaste);
  }, [annotationApiRef]);
}
