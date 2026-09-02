import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const compressOperationConfig = defineSingleFileTool<ErasedToolParams>({
  operationType: "compress",
  endpoint: "/api/v1/misc/compress-pdf",
  buildFormData: (params, file) =>
    objectToFormData(params as Record<string, unknown>, { fileInput: file }),
});

export const useCompressOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...compressOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("compress.error.failed", "Operation failed."),
    ),
  });
};
