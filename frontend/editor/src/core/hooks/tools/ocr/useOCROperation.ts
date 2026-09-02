import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const ocrOperationConfig = defineSingleFileTool<ErasedToolParams>({
  operationType: "ocr",
  endpoint: "/api/v1/misc/ocr-pdf",
  buildFormData: (params, file) =>
    objectToFormData(params as Record<string, unknown>, { fileInput: file }),
});

export const useOCROperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...ocrOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("ocr.error.failed", "Operation failed."),
    ),
  });
};
