import {
  defineMultiFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const overlayPdfsOperationConfig = defineMultiFileTool<ErasedToolParams>(
  {
    operationType: "overlayPdfs",
    endpoint: "/api/v1/general/overlay-pdfs",
    buildFormData: (params, files) => {
      const fd = objectToFormData(params as Record<string, unknown>);
      files.forEach((f) => fd.append("fileInput", f));
      return fd;
    },
  },
);

export const useOverlayPdfsOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...overlayPdfsOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("overlayPdfs.error.failed", "Operation failed."),
    ),
  });
};
