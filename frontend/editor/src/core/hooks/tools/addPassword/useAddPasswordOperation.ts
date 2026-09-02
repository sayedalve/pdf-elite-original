import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const addPasswordOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "addPassword",
    endpoint: "/api/v1/security/add-password",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useAddPasswordOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...addPasswordOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("addPassword.error.failed", "Operation failed."),
    ),
  });
};
