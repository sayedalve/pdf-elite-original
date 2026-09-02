import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const removeAnnotationsOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "removeAnnotations",
    endpoint: "/api/v1/misc/flatten",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useRemoveAnnotationsOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...removeAnnotationsOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("removeAnnotations.error.failed", "Operation failed."),
    ),
  });
};
