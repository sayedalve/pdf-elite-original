/* eslint-disable */
import { useCallback } from "react";
import { useTranslation } from "react-i18next";
import {
  useToolOperation,
  defineSingleFileTool,
} from "@app/hooks/tools/shared/useToolOperation";
import {
  objectToFormData,
  type ToolApiParams,
  type ToolEndpoint,
} from "@app/hooks/tools/shared/toolApiMapping";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import {
  ReplaceImageParameters,
  defaultParameters,
} from "@app/hooks/tools/replaceImage/useReplaceImageParameters";
import { useToolResources } from "@app/hooks/tools/shared/useToolResources";

const ENDPOINT = "/api/v1/misc/replace-image" as any;
type ReplaceImageApiParams = any;

export const replaceImageToApiParams = (
  parameters: ReplaceImageParameters,
): ReplaceImageApiParams => ({
  imageIndex: parameters.imageIndex,
  pageNumber: parameters.pageNumber,
});

export const replaceImageFromApiParams = (
  apiParams: ReplaceImageApiParams,
): Partial<ReplaceImageParameters> => ({
  imageIndex: apiParams.imageIndex,
  pageNumber: apiParams.pageNumber,
});

// Static configuration that can be used by both the hook and automation executor
export const buildReplaceImageFormData = (
  parameters: ReplaceImageParameters,
  file: File,
  replacementImage: File,
): FormData => {
  const formData = new FormData();
  formData.append("fileInput", file);
  formData.append("replacementImage", replacementImage);

  if (parameters.imageIndex !== undefined) {
    formData.append("imageIndex", parameters.imageIndex.toString());
  }
  if (parameters.pageNumber !== undefined) {
    formData.append("pageNumber", parameters.pageNumber.toString());
  }

  return formData;
};

// Static configuration object (without response handler - will be added in hook)
export const replaceImageOperationConfig = {
  toApiParams: replaceImageToApiParams,
  fromApiParams: replaceImageFromApiParams,
  operationType: "replaceImage",
  endpoint: ENDPOINT,
  defaultParameters,
  toolType: "custom" as const,
  customProcessor: async (params: ReplaceImageParameters, files: File[]) => {
    if (files.length < 2) throw new Error("Missing replacement image");
    const formData = buildReplaceImageFormData(params, files[0], files[1]);
    const { default: apiClient } = await import("@app/services/apiClient");
    const { processResponse } =
      await import("@app/utils/toolResponseProcessor");
    const response = await apiClient.post(ENDPOINT, formData, {
      responseType: "blob",
    });
    const resultFiles = await processResponse(response.data, [files[0]], "");
    return { files: resultFiles };
  },
};

export const useReplaceImageOperation = () => {
  const { t } = useTranslation();

  return useToolOperation<ReplaceImageParameters>({
    ...replaceImageOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t(
        "replaceImage.error.failed",
        "An error occurred while replacing images in the PDF.",
      ),
    ),
  });
};
