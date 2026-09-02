import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const cropOperationConfig = defineSingleFileTool<ErasedToolParams>({
  operationType: "crop",
  endpoint: "/api/v1/general/crop",
  buildFormData: (params, file) =>
    objectToFormData(params as Record<string, unknown>, { fileInput: file }),
});

export const useCropOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...cropOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("crop.error.failed", "Operation failed."),
    ),
  });
};
