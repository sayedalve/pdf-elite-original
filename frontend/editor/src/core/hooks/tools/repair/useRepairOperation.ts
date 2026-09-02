import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const repairOperationConfig = defineSingleFileTool<ErasedToolParams>({
  operationType: "repair",
  endpoint: "/api/v1/misc/repair",
  buildFormData: (params, file) =>
    objectToFormData(params as Record<string, unknown>, { fileInput: file }),
});

export const useRepairOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...repairOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("repair.error.failed", "Operation failed."),
    ),
  });
};
