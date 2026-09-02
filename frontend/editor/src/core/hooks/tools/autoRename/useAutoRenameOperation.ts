import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const autoRenameOperationConfig = defineSingleFileTool<ErasedToolParams>(
  {
    operationType: "autoRename",
    endpoint: "/api/v1/misc/auto-rename",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  },
);

export const useAutoRenameOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...autoRenameOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("autoRename.error.failed", "Operation failed."),
    ),
  });
};
