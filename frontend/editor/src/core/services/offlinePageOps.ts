/**
 * offlinePageOps.ts
 *
 * Local (offline) PDF page operations using @cantoo/pdf-lib.
 *
 * Replaces the removed backend endpoints:
 *   POST /api/v1/general/remove-pages
 *   POST /api/v1/general/extract-pages
 *   POST /api/v1/general/rearrange-pages
 *
 * All functions accept a File and return a new Blob (PDF bytes) so they
 * plug directly into the useToolOperation customProcessor pattern.
 */

import { PDFDocument } from "@cantoo/pdf-lib";

/**
 * Parse a page-selection string like "1,3,5-8,10" into a sorted,
 * deduplicated, 0-indexed array of page indices.
 *
 * @param spec    Page selection string (1-indexed, e.g. "1-3,5,7-9")
 * @param total   Total page count in the document
 */
export function parsePageNumbers(spec: string, total: number): number[] {
  const indices = new Set<number>();
  const parts = spec
    .split(",")
    .map((p) => p.trim())
    .filter(Boolean);

  for (const part of parts) {
    if (part.includes("-")) {
      const [rawFrom, rawTo] = part.split("-");
      const from = parseInt(rawFrom, 10);
      const to = parseInt(rawTo, 10);
      if (isNaN(from) || isNaN(to)) continue;
      const lo = Math.max(1, Math.min(from, to));
      const hi = Math.min(total, Math.max(from, to));
      for (let i = lo; i <= hi; i++) {
        indices.add(i - 1); // convert to 0-indexed
      }
    } else {
      const n = parseInt(part, 10);
      if (!isNaN(n) && n >= 1 && n <= total) {
        indices.add(n - 1);
      }
    }
  }

  return [...indices].sort((a, b) => a - b);
}

/**
 * Parse a page-selection string like "3,1,7-9" into an ordered,
 * 0-indexed array of page indices, preserving the user's exact order.
 *
 * @param spec    Page selection string (1-indexed, e.g. "3,1,7-9")
 * @param total   Total page count in the document
 */
export function parsePageSequence(spec: string, total: number): number[] {
  const indices: number[] = [];
  const parts = spec
    .split(",")
    .map((p) => p.trim())
    .filter(Boolean);

  for (const part of parts) {
    if (part.includes("-")) {
      const [rawFrom, rawTo] = part.split("-");
      const from = parseInt(rawFrom, 10);
      const to = parseInt(rawTo, 10);
      if (isNaN(from) || isNaN(to)) continue;

      const start = Math.max(1, Math.min(from, total));
      const end = Math.max(1, Math.min(to, total));

      if (start <= end) {
        for (let i = start; i <= end; i++) {
          indices.push(i - 1);
        }
      } else {
        for (let i = start; i >= end; i--) {
          indices.push(i - 1);
        }
      }
    } else {
      const n = parseInt(part, 10);
      if (!isNaN(n) && n >= 1 && n <= total) {
        indices.push(n - 1);
      }
    }
  }

  return indices;
}

/**
 * Remove the specified pages from a PDF file.
 *
 * @param file        Source PDF File
 * @param pageNumbers 1-indexed page selection string (e.g. "1,3,5-8")
 * @returns           New PDF Blob with the specified pages removed
 */
export async function removePagesLocal(
  file: File,
  pageNumbers: string,
): Promise<Blob> {
  const bytes = await file.arrayBuffer();
  const srcDoc = await PDFDocument.load(bytes);
  const total = srcDoc.getPageCount();

  // Pages to remove (0-indexed)
  const toRemove = new Set(parsePageNumbers(pageNumbers, total));

  // Build the output by copying pages NOT in the removal set
  const outDoc = await PDFDocument.create();
  const keepIndices = Array.from({ length: total }, (_, i) => i).filter(
    (i) => !toRemove.has(i),
  );

  if (keepIndices.length === 0) {
    throw new Error("Cannot remove all pages from a PDF.");
  }

  const copied = await outDoc.copyPages(srcDoc, keepIndices);
  for (const page of copied) {
    outDoc.addPage(page);
  }

  const outBytes = await outDoc.save();
  return new Blob([outBytes.buffer as ArrayBuffer], {
    type: "application/pdf",
  });
}

/**
 * Extract (keep only) the specified pages from a PDF file.
 *
 * @param file        Source PDF File
 * @param pageNumbers 1-indexed page selection string (e.g. "2,4-6")
 * @returns           New PDF Blob containing only the specified pages
 */
export async function extractPagesLocal(
  file: File,
  pageNumbers: string,
): Promise<Blob> {
  const bytes = await file.arrayBuffer();
  const srcDoc = await PDFDocument.load(bytes);
  const total = srcDoc.getPageCount();

  const keepIndices = parsePageSequence(pageNumbers, total);
  if (keepIndices.length === 0) {
    throw new Error("No valid pages specified for extraction.");
  }

  const outDoc = await PDFDocument.create();
  const copied = await outDoc.copyPages(srcDoc, keepIndices);
  for (const page of copied) {
    outDoc.addPage(page);
  }

  const outBytes = await outDoc.save();
  return new Blob([outBytes.buffer as ArrayBuffer], {
    type: "application/pdf",
  });
}

/**
 * Rearrange pages in a PDF document.
 *
 * @param file        Source PDF File
 * @param pageOrder   Comma-separated 1-indexed page order (e.g. "3,1,2" to put page 3 first)
 * @returns           New PDF Blob with pages in the specified order
 */
export async function rearrangePagesLocal(
  file: File,
  pageOrder: string,
): Promise<Blob> {
  const bytes = await file.arrayBuffer();
  const srcDoc = await PDFDocument.load(bytes);
  const total = srcDoc.getPageCount();

  // pageOrder is a full ordered list (may repeat pages)
  const orderedIndices = pageOrder
    .split(",")
    .map((s) => parseInt(s.trim(), 10) - 1)
    .filter((i) => i >= 0 && i < total);

  if (orderedIndices.length === 0) {
    throw new Error("No valid page order specified.");
  }

  const outDoc = await PDFDocument.create();
  const copied = await outDoc.copyPages(srcDoc, orderedIndices);
  for (const page of copied) {
    outDoc.addPage(page);
  }

  const outBytes = await outDoc.save();
  return new Blob([outBytes.buffer as ArrayBuffer], {
    type: "application/pdf",
  });
}

/**
 * Rotate the specified pages in a PDF document.
 *
 * @param file        Source PDF File
 * @param pageNumbers 1-indexed page selection string, or "" for all pages
 * @param rotation    Degrees to rotate (90, 180, 270, or -90)
 * @returns           New PDF Blob with the pages rotated
 */
export async function rotatePagesLocal(
  file: File,
  pageNumbers: string,
  rotation: number,
): Promise<Blob> {
  const bytes = await file.arrayBuffer();
  const doc = await PDFDocument.load(bytes);
  const total = doc.getPageCount();

  const targetIndices = pageNumbers.trim()
    ? parsePageNumbers(pageNumbers, total)
    : Array.from({ length: total }, (_, i) => i);

  for (const idx of targetIndices) {
    const page = doc.getPage(idx);
    const currentRotation = page.getRotation().angle;
    // Normalise to 0–359
    const newAngle = (((currentRotation + rotation) % 360) + 360) % 360;
    page.setRotation({ type: "degrees", angle: newAngle } as Parameters<
      typeof page.setRotation
    >[0]);
  }

  const outBytes = await doc.save();
  return new Blob([outBytes.buffer as ArrayBuffer], {
    type: "application/pdf",
  });
}

/**
 * Apply a complete set of organize operations (reorder, duplicate, remove, rotate)
 * to a PDF document in a single pass.
 *
 * @param file        Source PDF File
 * @param pages       Array describing the desired final state of the document.
 *                    Each entry contains the 1-indexed original page number and
 *                    its desired absolute rotation (0, 90, 180, 270).
 * @returns           New PDF Blob with the changes applied
 */
export async function applyOrganizeChangesLocal(
  file: File,
  pages: {
    originalId: number;
    rotation: number;
    isBlank?: boolean;
    blankDimensions?: [number, number];
  }[],
): Promise<Blob> {
  const bytes = await file.arrayBuffer();
  const srcDoc = await PDFDocument.load(bytes);
  const total = srcDoc.getPageCount();

  const plan = pages.filter(
    (p) => p.isBlank || (p.originalId - 1 >= 0 && p.originalId - 1 < total),
  );

  if (plan.length === 0) {
    throw new Error("No valid pages specified for the reorganized document.");
  }

  const outDoc = await PDFDocument.create();

  // Find all original indices we need to copy
  const indicesToCopy = plan
    .filter((p) => !p.isBlank)
    .map((p) => p.originalId - 1);

  // We can't guarantee `copyPages` handles an empty array nicely, so conditionally copy
  const copied =
    indicesToCopy.length > 0
      ? await outDoc.copyPages(srcDoc, indicesToCopy)
      : [];

  let copiedIndex = 0;

  for (let i = 0; i < plan.length; i++) {
    const p = plan[i];

    if (p.isBlank) {
      let width = p.blankDimensions?.[0];
      let height = p.blankDimensions?.[1];

      // Match neighboring page size if dimensions aren't explicitly provided
      if (!width || !height) {
        // Try previous page first
        let neighborIndex = -1;
        for (let j = i - 1; j >= 0; j--) {
          if (!plan[j].isBlank) {
            neighborIndex = plan[j].originalId - 1;
            break;
          }
        }
        // If no previous page, try next page
        if (neighborIndex === -1) {
          for (let j = i + 1; j < plan.length; j++) {
            if (!plan[j].isBlank) {
              neighborIndex = plan[j].originalId - 1;
              break;
            }
          }
        }

        if (
          neighborIndex !== -1 &&
          neighborIndex >= 0 &&
          neighborIndex < total
        ) {
          const neighborPage = srcDoc.getPage(neighborIndex);
          const size = neighborPage.getSize();
          width = size.width;
          height = size.height;
        } else {
          // Fallback to A4
          width = 595.28;
          height = 841.89;
        }
      }

      const page = outDoc.addPage([width, height]);
      if (p.rotation !== 0) {
        const newAngle = ((p.rotation % 360) + 360) % 360;
        page.setRotation({ type: "degrees", angle: newAngle } as Parameters<
          typeof page.setRotation
        >[0]);
      }
    } else {
      const page = copied[copiedIndex++];
      const targetRot = p.rotation;
      const currentRotation = page.getRotation().angle;
      const newAngle = (((currentRotation + targetRot) % 360) + 360) % 360;

      page.setRotation({ type: "degrees", angle: newAngle } as Parameters<
        typeof page.setRotation
      >[0]);

      outDoc.addPage(page);
    }
  }

  const outBytes = await outDoc.save();
  return new Blob([outBytes.buffer as ArrayBuffer], {
    type: "application/pdf",
  });
}

/**
 * Insert every page of `sourceFile` into `baseFile` at the given 0-indexed
 * position (pages currently at index >= insertAtIndex shift right). Fully local
 * (pdf-lib) — no backend/cloud call — and page rotations are preserved for both
 * the base and the inserted pages. Returns a new PDF Blob.
 *
 * @param baseFile      Document to insert into (File or Blob)
 * @param sourceFile    PDF whose pages are inserted (File or Blob)
 * @param insertAtIndex 0-indexed insertion position (clamped to [0, baseCount])
 */
export async function insertPagesLocal(
  baseFile: File | Blob,
  sourceFile: File | Blob,
  insertAtIndex: number,
): Promise<Blob> {
  const [baseBytes, srcBytes] = await Promise.all([
    baseFile.arrayBuffer(),
    sourceFile.arrayBuffer(),
  ]);
  const baseDoc = await PDFDocument.load(baseBytes);
  const srcDoc = await PDFDocument.load(srcBytes);
  const baseCount = baseDoc.getPageCount();
  const srcCount = srcDoc.getPageCount();

  if (srcCount === 0) {
    throw new Error("The selected PDF has no pages to insert.");
  }

  const at = Math.max(0, Math.min(insertAtIndex, baseCount));
  const outDoc = await PDFDocument.create();

  const addFrom = async (doc: PDFDocument, indices: number[]) => {
    if (indices.length === 0) return;
    const copied = await outDoc.copyPages(doc, indices);
    for (const page of copied) outDoc.addPage(page);
  };

  // base [0, at)  +  every source page  +  base [at, end)
  await addFrom(
    baseDoc,
    Array.from({ length: at }, (_, i) => i),
  );
  await addFrom(
    srcDoc,
    Array.from({ length: srcCount }, (_, i) => i),
  );
  await addFrom(
    baseDoc,
    Array.from({ length: baseCount - at }, (_, i) => at + i),
  );

  const outBytes = await outDoc.save();
  return new Blob([outBytes.buffer as ArrayBuffer], {
    type: "application/pdf",
  });
}

/**
 * Replace the pages at `selectedIndices` (0-indexed, in the base document's
 * current order) with every page of `sourceFile`. The replacement pages are
 * spliced in at the position of the first replaced page, and all selected pages
 * are removed. Fully local (pdf-lib) — no backend/cloud call. Returns a new
 * PDF Blob.
 *
 * @param baseFile        Document being edited (File or Blob)
 * @param sourceFile      PDF whose pages become the replacement (File or Blob)
 * @param selectedIndices 0-indexed pages in the base document to replace
 */
export async function replacePagesLocal(
  baseFile: File | Blob,
  sourceFile: File | Blob,
  selectedIndices: number[],
): Promise<Blob> {
  const [baseBytes, srcBytes] = await Promise.all([
    baseFile.arrayBuffer(),
    sourceFile.arrayBuffer(),
  ]);
  const baseDoc = await PDFDocument.load(baseBytes);
  const srcDoc = await PDFDocument.load(srcBytes);
  const baseCount = baseDoc.getPageCount();
  const srcCount = srcDoc.getPageCount();

  if (srcCount === 0) {
    throw new Error("The selected PDF has no pages to use as a replacement.");
  }

  const remove = new Set(
    selectedIndices.filter(
      (i) => Number.isInteger(i) && i >= 0 && i < baseCount,
    ),
  );
  if (remove.size === 0) {
    throw new Error("No valid pages selected to replace.");
  }

  const insertAt = Math.min(...remove);

  // Partition the surviving base pages around the splice point: everything
  // kept before the first replaced page, then the source pages, then everything
  // kept after (selected pages are dropped entirely).
  const before: number[] = [];
  const after: number[] = [];
  for (let i = 0; i < baseCount; i++) {
    if (remove.has(i)) continue;
    if (i < insertAt) before.push(i);
    else after.push(i);
  }

  const outDoc = await PDFDocument.create();
  const addFrom = async (doc: PDFDocument, indices: number[]) => {
    if (indices.length === 0) return;
    const copied = await outDoc.copyPages(doc, indices);
    for (const page of copied) outDoc.addPage(page);
  };

  await addFrom(baseDoc, before);
  await addFrom(
    srcDoc,
    Array.from({ length: srcCount }, (_, i) => i),
  );
  await addFrom(baseDoc, after);

  const outBytes = await outDoc.save();
  return new Blob([outBytes.buffer as ArrayBuffer], {
    type: "application/pdf",
  });
}

/**
 * Split a PDF into two separate PDFs at the specified split point.
 *
 * @param file        Source PDF File
 * @param splitIndex  The 1-based page number where the second PDF starts.
 *                    (e.g., if splitIndex is 3, PDF 1 gets pages 1-2, PDF 2 gets 3-end).
 * @returns           An array containing two new PDF Blobs.
 */
export async function splitPagesLocal(
  file: File,
  splitIndex: number,
): Promise<[Blob, Blob]> {
  const bytes = await file.arrayBuffer();
  const srcDoc = await PDFDocument.load(bytes);
  const total = srcDoc.getPageCount();

  if (splitIndex <= 1 || splitIndex > total) {
    throw new Error(
      `Invalid split index ${splitIndex}. Must be between 2 and ${total}.`,
    );
  }

  const outDoc1 = await PDFDocument.create();
  const outDoc2 = await PDFDocument.create();

  const indices1 = Array.from({ length: splitIndex - 1 }, (_, i) => i);
  const indices2 = Array.from(
    { length: total - splitIndex + 1 },
    (_, i) => i + splitIndex - 1,
  );

  const copied1 = await outDoc1.copyPages(srcDoc, indices1);
  for (const page of copied1) outDoc1.addPage(page);

  const copied2 = await outDoc2.copyPages(srcDoc, indices2);
  for (const page of copied2) outDoc2.addPage(page);

  const outBytes1 = await outDoc1.save();
  const outBytes2 = await outDoc2.save();

  return [
    new Blob([outBytes1.buffer as ArrayBuffer], { type: "application/pdf" }),
    new Blob([outBytes2.buffer as ArrayBuffer], { type: "application/pdf" }),
  ];
}

/**
 * Split a PDF into multiple PDFs based on an array of page index arrays.
 *
 * @param file        Source PDF File
 * @param splits      Array of arrays of 0-indexed page numbers. Each array represents a new PDF.
 * @returns           An array containing the new PDF Blobs.
 */
export async function splitAdvancedLocal(
  file: File,
  splits: number[][],
): Promise<Blob[]> {
  const bytes = await file.arrayBuffer();
  const srcDoc = await PDFDocument.load(bytes);

  const outBlobs: Blob[] = [];

  for (const indices of splits) {
    if (indices.length === 0) continue;
    const outDoc = await PDFDocument.create();
    const copied = await outDoc.copyPages(srcDoc, indices);
    for (const page of copied) outDoc.addPage(page);

    const outBytes = await outDoc.save();
    outBlobs.push(
      new Blob([outBytes.buffer as ArrayBuffer], { type: "application/pdf" }),
    );
  }

  return outBlobs;
}

/**
 * Merge multiple PDFs into a single PDF in the provided order.
 *
 * @param files       Array of source PDF Files
 * @returns           New merged PDF Blob
 */
export async function mergePagesLocal(files: File[]): Promise<Blob> {
  if (files.length === 0) {
    throw new Error("No files provided to merge.");
  }

  const outDoc = await PDFDocument.create();

  for (const file of files) {
    const bytes = await file.arrayBuffer();
    try {
      const srcDoc = await PDFDocument.load(bytes);
      const total = srcDoc.getPageCount();
      if (total > 0) {
        const copied = await outDoc.copyPages(
          srcDoc,
          Array.from({ length: total }, (_, i) => i),
        );
        for (const page of copied) outDoc.addPage(page);
      }
    } catch (e) {
      console.warn(`Failed to merge file ${file.name}`, e);
      throw new Error(
        `Cannot read or merge file: ${file.name}. It might be encrypted or invalid.`,
      );
    }
  }

  if (outDoc.getPageCount() === 0) {
    throw new Error("Resulting merged document has no pages.");
  }

  const outBytes = await outDoc.save();
  return new Blob([outBytes.buffer as ArrayBuffer], {
    type: "application/pdf",
  });
}
