import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const editTableOfContentsOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "editTableOfContents",
    endpoint: "/api/v1/general/edit-table-of-contents",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useEditTableOfContentsOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...editTableOfContentsOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("editTableOfContents.error.failed", "Operation failed."),
    ),
  });
};
