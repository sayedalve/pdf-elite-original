import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const redactOperationConfig = defineSingleFileTool<ErasedToolParams>({
  operationType: "redact",
  endpoint: "/api/v1/security/redact",
  buildFormData: (params, file) =>
    objectToFormData(params as Record<string, unknown>, { fileInput: file }),
});

export const useRedactOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...redactOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("redact.error.failed", "Operation failed."),
    ),
  });
};
