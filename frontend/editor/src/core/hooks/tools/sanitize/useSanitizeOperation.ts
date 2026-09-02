import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const sanitizeOperationConfig = defineSingleFileTool<ErasedToolParams>({
  operationType: "sanitize",
  endpoint: "/api/v1/security/sanitize-pdf",
  buildFormData: (params, file) =>
    objectToFormData(params as Record<string, unknown>, { fileInput: file }),
});

export const useSanitizeOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...sanitizeOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("sanitize.error.failed", "Operation failed."),
    ),
  });
};
