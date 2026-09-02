import { useTranslation } from "react-i18next";
import {
  defineCustomTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { type ToolApiParams } from "@app/hooks/tools/shared/toolApiMapping";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import {
  ReorganizePagesParameters,
  defaultReorganizePagesParameters,
} from "@app/hooks/tools/reorganizePages/useReorganizePagesParameters";
import { PDFDocument } from "@cantoo/pdf-lib";

// Keep API helpers for automation / pipeline compatibility
type ReorganizePagesApiParams =
  ToolApiParams["/api/v1/general/rearrange-pages"];

export const reorganizePagesToApiParams = (
  parameters: ReorganizePagesParameters,
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
): any => {
  const apiParams: ReorganizePagesApiParams = {};
  if (parameters.customMode) {
    apiParams.customMode =
      parameters.customMode as ReorganizePagesApiParams["customMode"];
  }
  if (parameters.pageNumbers) {
    apiParams.pageNumbers = parameters.pageNumbers.replace(/\s+/g, "");
  }
  return apiParams;
};

export const reorganizePagesFromApiParams = (
  apiParams: ReorganizePagesApiParams,
): Partial<ReorganizePagesParameters> => ({
  customMode:
    apiParams.customMode ?? defaultReorganizePagesParameters.customMode,
  pageNumbers:
    apiParams.pageNumbers ?? defaultReorganizePagesParameters.pageNumbers,
});

/**
 * Compute the 0-indexed page order for a given mode and total page count.
 * Returns null when the mode is unsupported or needs custom pageNumbers.
 */
function computePageOrder(
  mode: string,
  total: number,
  pageNumbers: string,
): number[] | null {
  switch (mode) {
    case "":
    case "DUPLICATE": {
      // Custom order provided as comma-separated 1-based page numbers
      const parts = pageNumbers
        .split(",")
        .map((s) => parseInt(s.trim(), 10) - 1)
        .filter((i) => i >= 0 && i < total);
      if (parts.length === 0) return null;
      // DUPLICATE: each index appears N times (where N = parts.length / total)
      if (mode === "DUPLICATE") {
        const count = parts.length;
        const expanded: number[] = [];
        for (let i = 0; i < total; i++) {
          for (let d = 0; d < count; d++) {
            expanded.push(i);
          }
        }
        return expanded;
      }
      return parts;
    }
    case "REVERSE_ORDER":
      return Array.from({ length: total }, (_, i) => total - 1 - i);
    case "DUPLEX_SORT": {
      // Fronts (1, 2, …, n/2) then backs (n/2+1, …, n) interleaved
      const half = Math.ceil(total / 2);
      const order: number[] = [];
      for (let i = 0; i < half; i++) {
        order.push(i);
        const back = total - 1 - i;
        if (back !== i) order.push(back);
      }
      return order;
    }
    case "BOOKLET_SORT": {
      // last, first, second, second-last, …
      const order: number[] = [];
      let lo = 0;
      let hi = total - 1;
      while (lo <= hi) {
        order.push(hi--);
        if (lo <= hi) order.push(lo++);
      }
      return order;
    }
    case "SIDE_STITCH_BOOKLET_SORT": {
      // Pad to multiple of 4, then arrange for side-stitch
      const padded = Math.ceil(total / 4) * 4;
      const order: number[] = [];
      for (let i = 0; i < padded / 2; i += 2) {
        order.push(padded - 1 - i); // last
        order.push(i); // first
        order.push(i + 1); // second
        order.push(padded - 2 - i); // second-last
      }
      return order.filter((i) => i < total);
    }
    case "ODD_EVEN_SPLIT":
      // Odds first, then evens (two separate docs would be ideal,
      // but we concatenate here as a single output)
      return [
        ...Array.from({ length: Math.ceil(total / 2) }, (_, i) => i * 2),
        ...Array.from({ length: Math.floor(total / 2) }, (_, i) => i * 2 + 1),
      ];
    case "ODD_EVEN_MERGE":
      // Interleave: page 0, total/2, 1, total/2+1, …
      return Array.from({ length: total }, (_, i) =>
        i % 2 === 0 ? i / 2 : Math.ceil(total / 2) + Math.floor(i / 2),
      ).filter((i) => i < total);
    case "REMOVE_FIRST":
      return total > 1
        ? Array.from({ length: total - 1 }, (_, i) => i + 1)
        : null;
    case "REMOVE_LAST":
      return total > 1 ? Array.from({ length: total - 1 }, (_, i) => i) : null;
    case "REMOVE_FIRST_AND_LAST":
      return total > 2
        ? Array.from({ length: total - 2 }, (_, i) => i + 1)
        : null;
    default:
      return null;
  }
}

/**
 * Offline processor — uses @cantoo/pdf-lib to rearrange pages locally.
 * Replaces the removed backend endpoint /api/v1/general/rearrange-pages.
 */
async function reorganizePagesProcessor(
  params: ReorganizePagesParameters,
  files: File[],
): Promise<{ files: File[]; consumedAllInputs: boolean }> {
  const outputs: File[] = [];

  for (const file of files) {
    const bytes = await file.arrayBuffer();
    const srcDoc = await PDFDocument.load(bytes);
    const total = srcDoc.getPageCount();

    const order = computePageOrder(
      params.customMode,
      total,
      params.pageNumbers,
    );
    if (!order || order.length === 0) {
      throw new Error(
        `No valid page order computed for mode "${params.customMode}".`,
      );
    }

    const outDoc = await PDFDocument.create();
    const copied = await outDoc.copyPages(srcDoc, order);
    for (const page of copied) {
      outDoc.addPage(page);
    }

    const outBytes = await outDoc.save();
    const base = file.name.replace(/\.pdf$/iu, "");
    outputs.push(
      new File([outBytes.buffer as ArrayBuffer], `${base}_reorganized.pdf`, {
        type: "application/pdf",
      }),
    );
  }

  return { files: outputs, consumedAllInputs: true };
}

export const reorganizePagesOperationConfig = defineCustomTool({
  operationType: "reorganizePages",
  toApiParams: reorganizePagesToApiParams,
  fromApiParams: reorganizePagesFromApiParams,
  customProcessor: reorganizePagesProcessor,
});

export const useReorganizePagesOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ReorganizePagesParameters>({
    ...reorganizePagesOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("reorganizePages.error.failed", "Failed to reorganize pages"),
    ),
  });
};
