import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const adjustPageScaleOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "adjustPageScale",
    endpoint: "/api/v1/general/scale-pages",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useAdjustPageScaleOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...adjustPageScaleOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("adjustPageScale.error.failed", "Operation failed."),
    ),
  });
};
