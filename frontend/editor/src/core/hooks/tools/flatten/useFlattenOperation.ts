import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const flattenOperationConfig = defineSingleFileTool<ErasedToolParams>({
  operationType: "flatten",
  endpoint: "/api/v1/misc/flatten",
  buildFormData: (params, file) =>
    objectToFormData(params as Record<string, unknown>, { fileInput: file }),
});

export const useFlattenOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...flattenOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("flatten.error.failed", "Operation failed."),
    ),
  });
};
