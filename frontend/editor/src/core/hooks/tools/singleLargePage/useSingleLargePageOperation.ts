import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const singleLargePageOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "singleLargePage",
    endpoint: "/api/v1/general/pdf-to-single-page",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useSingleLargePageOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...singleLargePageOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("singleLargePage.error.failed", "Operation failed."),
    ),
  });
};
