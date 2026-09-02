import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const unlockPdfFormsOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "unlockPdfForms",
    endpoint: "/api/v1/misc/unlock-pdf-forms",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useUnlockPdfFormsOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...unlockPdfFormsOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("unlockPdfForms.error.failed", "Operation failed."),
    ),
  });
};
