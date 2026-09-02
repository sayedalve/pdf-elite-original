import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const insertBlankPagesOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "insertBlankPages",
    endpoint: "/api/v1/general/insert-blank-pages",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useInsertBlankPagesOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...insertBlankPagesOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("insertBlankPages.error.failed", "Operation failed."),
    ),
  });
};
