import {
  defineSingleFileTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ErasedToolParams } from "@app/hooks/tools/shared/toolOperationTypes";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";

export const changeMetadataOperationConfig =
  defineSingleFileTool<ErasedToolParams>({
    operationType: "changeMetadata",
    endpoint: "/api/v1/misc/update-metadata",
    buildFormData: (params, file) =>
      objectToFormData(params as Record<string, unknown>, { fileInput: file }),
  });

export const useChangeMetadataOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ErasedToolParams>({
    ...changeMetadataOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("changeMetadata.error.failed", "Operation failed."),
    ),
  });
};
