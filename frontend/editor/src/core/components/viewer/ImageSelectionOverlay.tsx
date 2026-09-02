import React, { useEffect, useState, useRef } from "react";
import { Box } from "@mantine/core";
import type { PDFEliteFileStub } from "@app/types/fileContext";

// Reuse the layerUtils import trick
let pdfjsLib: any = null;

interface ImageBox {
  pageIndex: number;
  imageIndex: number; // The Nth image on this page
  rect: { left: number; top: number; width: number; height: number }; // In viewport percentage (0-100)
}

interface ImageSelectionOverlayProps {
  isActive: boolean;
  file: PDFEliteFileStub | null;
  mode: "replace" | "remove";
  onImageSelect: (pageNumber: number, imageIndex: number) => void;
}

export const ImageSelectionOverlay: React.FC<ImageSelectionOverlayProps> = ({
  isActive,
  file,
  mode,
  onImageSelect,
}) => {
  const [imageBoxes, setImageBoxes] = useState<ImageBox[]>([]);
  const [loading, setLoading] = useState(false);
  const parsedFileId = useRef<string | null>(null);

  useEffect(() => {
    if (!isActive || !file || !file.blobUrl) {
      setImageBoxes([]);
      return;
    }

    if (parsedFileId.current === file.id) {
      return; // Already parsed
    }

    let isMounted = true;

    const parseImages = async () => {
      setLoading(true);
      try {
        if (!pdfjsLib) {
          pdfjsLib = await import("pdfjs-dist/legacy/build/pdf.mjs");
          const workerUrl = (await import("pdfjs-dist/legacy/build/pdf.worker.min.mjs?url")).default;
          if (!pdfjsLib.GlobalWorkerOptions.workerSrc) {
            pdfjsLib.GlobalWorkerOptions.workerSrc = workerUrl;
          }
        }

        const response = await fetch(file.blobUrl!);
        const arrayBuffer = await response.arrayBuffer();
        
        const loadingTask = pdfjsLib.getDocument({ data: arrayBuffer, verbosity: 0 });
        const pdfDoc = await loadingTask.promise;
        
        const boxes: ImageBox[] = [];

        for (let i = 1; i <= pdfDoc.numPages; i++) {
          const page = await pdfDoc.getPage(i);
          const opList = await page.getOperatorList();
          
          let imageCount = 0;
          let currentTransform = [1, 0, 0, 1, 0, 0]; // Identity
          const transformStack: number[][] = [];

          // Viewport to normalize coordinates
          const viewport = page.getViewport({ scale: 1 });
          const pageWidth = viewport.width;
          const pageHeight = viewport.height;

          // Simple parsing of operator list
          // Note: Full parsing requires a state machine. We will do a basic one.
          for (let j = 0; j < opList.fnArray.length; j++) {
            const fn = opList.fnArray[j];
            const args = opList.argsArray[j];

            if (fn === pdfjsLib.OPS.save) {
              transformStack.push([...currentTransform]);
            } else if (fn === pdfjsLib.OPS.restore) {
              if (transformStack.length > 0) {
                currentTransform = transformStack.pop()!;
              }
            } else if (fn === pdfjsLib.OPS.transform) {
              // matrix multiplication
              const [a, b, c, d, e, f] = args;
              const [a0, b0, c0, d0, e0, f0] = currentTransform;
              currentTransform = [
                a0 * a + c0 * b,
                b0 * a + d0 * b,
                a0 * c + c0 * d,
                b0 * c + d0 * d,
                a0 * e + c0 * f + e0,
                b0 * e + d0 * f + f0
              ];
            } else if (fn === pdfjsLib.OPS.paintImageXObject || fn === pdfjsLib.OPS.paintInlineImageXObject) {
              // Image bounds are determined by the current transform matrix
              // The image is mapped to a 1x1 square at origin, then transformed
              const p0 = [0, 0];
              const p1 = [1, 0];
              const p2 = [1, 1];
              const p3 = [0, 1];

              const applyTransform = (p: number[]) => {
                return [
                  currentTransform[0] * p[0] + currentTransform[2] * p[1] + currentTransform[4],
                  currentTransform[1] * p[0] + currentTransform[3] * p[1] + currentTransform[5]
                ];
              };

              const tp0 = applyTransform(p0);
              const tp1 = applyTransform(p1);
              const tp2 = applyTransform(p2);
              const tp3 = applyTransform(p3);

              const minX = Math.min(tp0[0], tp1[0], tp2[0], tp3[0]);
              const maxX = Math.max(tp0[0], tp1[0], tp2[0], tp3[0]);
              const minY = Math.min(tp0[1], tp1[1], tp2[1], tp3[1]);
              const maxY = Math.max(tp0[1], tp1[1], tp2[1], tp3[1]);

              // pdf coordinates are bottom-left origin. Viewport is top-left.
              const left = (minX / pageWidth) * 100;
              const top = (1 - (maxY / pageHeight)) * 100;
              const width = ((maxX - minX) / pageWidth) * 100;
              const height = ((maxY - minY) / pageHeight) * 100;

              boxes.push({
                pageIndex: i, // 1-indexed
                imageIndex: imageCount,
                rect: { left, top, width, height }
              });

              imageCount++;
            }
          }
        }
        
        if (isMounted) {
          setImageBoxes(boxes);
          parsedFileId.current = file.id;
        }
      } catch (err) {
        console.error("Failed to parse images", err);
      } finally {
        if (isMounted) setLoading(false);
      }
    };

    parseImages();

    return () => {
      isMounted = false;
    };
  }, [isActive, file]);

  if (!isActive) return null;

  return (
    <Box
      style={{
        position: "absolute",
        top: 0,
        left: 0,
        right: 0,
        bottom: 0,
        pointerEvents: "none",
        zIndex: 50,
      }}
    >
      {loading && (
        <div style={{ position: "absolute", top: 10, left: "50%", transform: "translateX(-50%)", background: "rgba(0,0,0,0.7)", color: "white", padding: "4px 12px", borderRadius: 4, zIndex: 60, pointerEvents: "auto" }}>
          Detecting images...
        </div>
      )}
      {!loading && imageBoxes.length === 0 && (
        <div style={{ position: "absolute", top: 10, left: "50%", transform: "translateX(-50%)", background: "rgba(0,0,0,0.7)", color: "white", padding: "4px 12px", borderRadius: 4, zIndex: 60, pointerEvents: "auto" }}
          onClick={() => {
            import("@app/components/toast").then(({ alert }) => {
              alert({ alertType: "error", title: "No Images", body: "Please click on an existing image to select it." });
            });
          }}
        >
          Click on an image to {mode} it
        </div>
      )}
      
      {imageBoxes.map((box, i) => (
        <ImageBoxElement key={i} box={box} onSelect={() => onImageSelect(box.pageIndex, box.imageIndex)} mode={mode} />
      ))}
    </Box>
  );
};

const ImageBoxElement = ({ box, onSelect, mode }: { box: ImageBox, onSelect: () => void, mode: string }) => {
  const [pageEl, setPageEl] = useState<HTMLElement | null>(null);

  useEffect(() => {
    const interval = setInterval(() => {
      const el = document.querySelector(`.page[data-page-number="${box.pageIndex}"]`);
      if (el && el !== pageEl) {
        setPageEl(el as HTMLElement);
        clearInterval(interval);
      }
    }, 500);
    
    return () => clearInterval(interval);
  }, [box.pageIndex, pageEl]);

  if (!pageEl) return null;

  // Render a portal to the page element
  const ReactDOM = require("react-dom");
  
  return ReactDOM.createPortal(
    <div
      onClick={(e) => {
        e.stopPropagation();
        e.preventDefault();
        onSelect();
      }}
      className="image-box-element"
      title={`Click to ${mode} image`}
      style={{
        position: "absolute",
        left: `${Math.max(0, box.rect.left)}%`,
        top: `${Math.max(0, box.rect.top)}%`,
        width: `${Math.min(100, box.rect.width)}%`,
        height: `${Math.min(100, box.rect.height)}%`,
        backgroundColor: mode === 'remove' ? 'rgba(255, 0, 0, 0.2)' : 'rgba(0, 150, 255, 0.2)',
        border: `2px dashed ${mode === 'remove' ? 'red' : '#0096ff'}`,
        cursor: "pointer",
        pointerEvents: "auto",
        zIndex: 100,
        transition: "all 0.2s",
      }}
      onMouseEnter={(e) => {
        e.currentTarget.style.backgroundColor = mode === 'remove' ? 'rgba(255, 0, 0, 0.4)' : 'rgba(0, 150, 255, 0.4)';
      }}
      onMouseLeave={(e) => {
        e.currentTarget.style.backgroundColor = mode === 'remove' ? 'rgba(255, 0, 0, 0.2)' : 'rgba(0, 150, 255, 0.2)';
      }}
    />,
    pageEl
  );
};
