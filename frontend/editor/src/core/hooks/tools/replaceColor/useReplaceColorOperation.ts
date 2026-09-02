import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const replaceColorOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "replaceColor",
    endpoint: "/api/v1/misc/replace-invert-pdf",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useReplaceColorOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...replaceColorOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("replaceColor.error.failed", "Operation failed."),
    ),
  });
};
