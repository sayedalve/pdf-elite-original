import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const removeCertificateSignOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "removeCertificateSign",
    endpoint: "/api/v1/security/remove-cert-sign",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useRemoveCertificateSignOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...removeCertificateSignOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("removeCertificateSign.error.failed", "Operation failed."),
    ),
  });
};
