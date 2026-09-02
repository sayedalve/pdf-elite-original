import { useTranslation } from "react-i18next";
import {
  defineCustomTool,
  useToolOperation,
  CustomProcessorResult,
} from "@app/hooks/tools/shared/useToolOperation";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import {
  validateExtractPagesParameters,
  ExtractPagesParameters,
  defaultParameters,
} from "@app/hooks/tools/extractPages/useExtractPagesParameters";
import { pdfWorkerManager } from "@app/services/pdfWorkerManager";
import { parseSelection } from "@app/utils/bulkselection/parseSelection";
import { extractPagesLocal } from "@app/services/offlinePageOps";

/**
 * Resolve an advanced selection expression (e.g. "odd", "2-5") into a
 * comma-separated list of 1-indexed page numbers using pdf.js.
 */
async function resolveSelectionToCsv(
  expression: string,
  file: File,
): Promise<string> {
  const arrayBuffer = await file.arrayBuffer();
  const pdf = await pdfWorkerManager.createDocument(arrayBuffer, {
    disableAutoFetch: true,
    disableStream: true,
  });
  try {
    const maxPages = pdf.numPages;
    const pages = parseSelection(expression || "", maxPages);
    return pages.join(",");
  } finally {
    pdfWorkerManager.destroyDocument(pdf);
  }
}

export const extractPagesOperationConfig = defineCustomTool({
  validateParams: validateExtractPagesParameters,
  operationType: "extractPages",
  /**
   * Offline processor — uses @cantoo/pdf-lib to extract pages locally.
   * Replaces the removed backend endpoint /api/v1/general/rearrange-pages
   * that was being used for extraction.
   */
  customProcessor: async (
    parameters: ExtractPagesParameters,
    files: File[],
  ): Promise<CustomProcessorResult> => {
    const outputs: File[] = [];

    for (const file of files) {
      // Resolve selection expression ("odd", "even", "2-5") into CSV
      const csv = await resolveSelectionToCsv(parameters.pageNumbers, file);

      const blob = await extractPagesLocal(file, csv);

      const base = (file.name || "document.pdf").replace(/\.[^.]+$/u, "");
      const outFile = new File([blob], `${base}_extracted_pages.pdf`, {
        type: "application/pdf",
      });
      outputs.push(outFile);
    }

    return { files: outputs, consumedAllInputs: false };
  },
  defaultParameters,
});

export const useExtractPagesOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ExtractPagesParameters>({
    ...extractPagesOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("extractPages.error.failed", "Failed to extract pages"),
    ),
  });
};
