import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const signOperationConfig = defineSingleFileTool<ErasedToolParams>({
  operationType: "sign",
  endpoint: "/api/v1/security/cert-sign",
  buildFormData: (params, file) =>
    objectToFormData(params as Record<string, unknown>, { fileInput: file }),
});

export const useSignOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...signOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("sign.error.failed", "Operation failed."),
    ),
  });
};
