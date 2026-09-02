import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const timestampPdfOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "timestampPdf",
    endpoint: "/api/v1/security/timestamp-pdf",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useTimestampPdfOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...timestampPdfOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("timestampPdf.error.failed", "Operation failed."),
    ),
  });
};
