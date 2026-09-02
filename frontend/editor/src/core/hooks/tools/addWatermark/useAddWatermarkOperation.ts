import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const addWatermarkOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "addWatermark",
    endpoint: "/api/v1/security/add-watermark",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useAddWatermarkOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...addWatermarkOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("addWatermark.error.failed", "Operation failed."),
    ),
  });
};
