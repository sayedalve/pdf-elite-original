import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const bookletImpositionOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "bookletImposition",
    endpoint: "/api/v1/general/booklet-imposition",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useBookletImpositionOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...bookletImpositionOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("bookletImposition.error.failed", "Operation failed."),
    ),
  });
};
