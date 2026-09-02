import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const addAttachmentsOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "addAttachments",
    endpoint: "/api/v1/misc/add-attachments",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useAddAttachmentsOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...addAttachmentsOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("addAttachments.error.failed", "Operation failed."),
    ),
  });
};
