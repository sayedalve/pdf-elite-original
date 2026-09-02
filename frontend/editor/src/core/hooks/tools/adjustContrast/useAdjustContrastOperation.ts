import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const adjustContrastOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "adjustContrast",
    endpoint: "/api/v1/misc/scanner-effect",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useAdjustContrastOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...adjustContrastOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("adjustContrast.error.failed", "Operation failed."),
    ),
  });
};
