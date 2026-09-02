/**
 * pdfToJsonLocal.ts
 *
 * LOCAL (offline) replacement for the removed backend endpoint:
 *   POST /api/v1/convert/pdf/text-editor
 *
 * Uses pdfjs-dist (already bundled in the app) to extract text positions
 * from a PDF and produce a PdfJsonDocument that the PdfTextEditor can
 * display and edit.
 *
 * Technical notes:
 * - pdf.js text content gives us each "text item" with a transform matrix and
 *   size. We map that to PdfJsonTextElement exactly as the backend did.
 * - Fonts come from the page's resources; we surface what pdf.js exposes.
 * - Images are NOT extracted here (too expensive in pure JS). The image
 *   layer remains empty — the editor gracefully handles this.
 * - The extraction runs page-by-page and calls a progress callback so the
 *   existing progress UI continues to work.
 */

import { pdfWorkerManager } from "@app/services/pdfWorkerManager";
import type {
  PdfJsonDocument,
  PdfJsonPage,
  PdfJsonTextElement,
  PdfJsonFont,
} from "@app/tools/pdfTextEditor/pdfTextEditorTypes";

export interface LocalConversionProgress {
  percent: number;
  stage: string;
  message: string;
  current?: number;
  total?: number;
}

export type ProgressCallback = (p: LocalConversionProgress) => void;

/**
 * Convert a PDF File to PdfJsonDocument entirely on the frontend.
 *
 * @param file    The PDF File object.
 * @param onProgress  Optional callback called as pages are processed.
 * @param signal  Optional AbortSignal to cancel a long conversion.
 */
export async function convertPdfToJsonLocal(
  file: File,
  onProgress?: ProgressCallback,
  signal?: AbortSignal,
): Promise<PdfJsonDocument> {
  onProgress?.({
    percent: 2,
    stage: "loading",
    message: "Loading PDF for extraction...",
  });

  const buffer = await file.arrayBuffer();
  if (signal?.aborted) throw new DOMException("Aborted", "AbortError");

  const pdfDoc = await pdfWorkerManager.createDocument(buffer);

  try {
    const totalPages = pdfDoc.numPages;
    const pages: PdfJsonPage[] = [];
    const fontMap = new Map<string, PdfJsonFont>();

    onProgress?.({
      percent: 5,
      stage: "processing",
      message: `Extracting text from ${totalPages} page${totalPages !== 1 ? "s" : ""}...`,
      total: totalPages,
    });

    for (let pageNum = 1; pageNum <= totalPages; pageNum++) {
      if (signal?.aborted) throw new DOMException("Aborted", "AbortError");

      const page = await pdfDoc.getPage(pageNum);
      const viewport = page.getViewport({ scale: 1.0 });
      const textContent = await page.getTextContent({
        includeMarkedContent: false,
      });

      const textElements: PdfJsonTextElement[] = [];
      const pageHeight = viewport.height;

      for (const item of textContent.items) {
        // Each item is a TextItem (with str, transform, width, height, fontName)
        // or a TextMarkedContent (with type, items). Filter to real text.
        if (!("str" in item)) continue;
        const textItem = item as {
          str: string;
          transform: number[];
          width: number;
          height: number;
          fontName: string;
          hasEOL?: boolean;
        };

        if (!textItem.str) continue;

        // pdf.js transform: [a, b, c, d, e, f]
        // e = x, f = y (in pdf.js coordinates, origin bottom-left for the page)
        // The text matrix scale gives us approximate font size.
        const [a, b, _c, d, e, f] = textItem.transform;

        // Approximate font size from the matrix scale factor
        const fontSizePt = Math.sqrt(a * a + b * b);
        const fontId = textItem.fontName ?? "unknown";

        // Register font if not seen yet
        if (!fontMap.has(fontId)) {
          fontMap.set(fontId, {
            id: fontId,
            uid: fontId,
            baseName: fontId,
            subtype: "Type1",
            embedded: false,
          });
        }

        // Convert pdf.js y (bottom-up) to PDF user-space (also bottom-up)
        // PDF user space origin is bottom-left, same as pdf.js viewport with scale=1.
        const x = e;
        const y = f; // PDF coordinate (bottom-up)
        const width = Math.abs(textItem.width);
        const height = Math.abs(textItem.height) || fontSizePt;

        // Reconstruct text matrix as [a, b, c, d, x, y]
        const textMatrix = [a, b, _c, d, x, y];

        const el: PdfJsonTextElement = {
          text: textItem.str,
          fontId,
          fontSize: fontSizePt,
          fontSizeInPt: fontSizePt,
          x,
          // Convert to top-left y for consistency with backend output:
          // backend reports y as distance from top of page
          y: pageHeight - y,
          width,
          height: height || fontSizePt,
          textMatrix,
          zOrder: 0,
          renderingMode: 0,
          horizontalScaling: 100,
          characterSpacing: 0,
          wordSpacing: 0,
          fillColor: { colorSpace: "DeviceGray", components: [0] },
        };

        textElements.push(el);
      }

      const pdfPage: PdfJsonPage = {
        pageNumber: pageNum,
        width: viewport.width,
        height: viewport.height,
        rotation: page.rotate ?? 0,
        textElements,
        imageElements: [], // image extraction is deferred / not supported locally
      };

      pages.push(pdfPage);
      page.cleanup();

      const percent = Math.round(5 + (pageNum / totalPages) * 90);
      onProgress?.({
        percent,
        stage: "processing",
        message: `Extracted page ${pageNum} of ${totalPages}`,
        current: pageNum,
        total: totalPages,
      });
    }

    onProgress?.({
      percent: 98,
      stage: "finalizing",
      message: "Finalizing...",
    });

    const doc: PdfJsonDocument = {
      metadata: null,
      fonts: Array.from(fontMap.values()),
      pages,
      lazyImages: false,
    };

    return doc;
  } finally {
    pdfWorkerManager.destroyDocument(pdfDoc);
  }
}
