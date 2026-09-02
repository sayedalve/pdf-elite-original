import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const scannerImageSplitOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "scannerImageSplit",
    endpoint: "/api/v1/misc/extract-image-scans",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useScannerImageSplitOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...scannerImageSplitOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("scannerImageSplit.error.failed", "Operation failed."),
    ),
  });
};
