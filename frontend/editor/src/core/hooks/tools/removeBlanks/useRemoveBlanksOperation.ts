import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const removeBlanksOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "removeBlanks",
    endpoint: "/api/v1/misc/remove-blanks",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useRemoveBlanksOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...removeBlanksOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("removeBlanks.error.failed", "Operation failed."),
    ),
  });
};
