import { useTranslation } from "react-i18next";
import { useToolOperation } from "@app/hooks/tools/shared/useToolOperation";
import {
  objectToFormData,
  type ToolApiParams,
} from "@app/hooks/tools/shared/toolApiMapping";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import {
  validateRemovePagesParameters,
  RemovePagesParameters,
  defaultParameters,
} from "@app/hooks/tools/removePages/useRemovePagesParameters";
import { removePagesLocal } from "@app/services/offlinePageOps";

// Keep the API-param helpers so automation / pipeline steps still work when
// the backend is present. They are not used for the actual operation.
type RemovePagesApiParams = ToolApiParams["/api/v1/general/remove-pages"];

export const removePagesToApiParams = (
  parameters: RemovePagesParameters,
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
): any => ({
  pageNumbers: parameters.pageNumbers.replace(/\s+/g, ""),
});

export const removePagesFromApiParams = (
  apiParams: RemovePagesApiParams,
): Partial<RemovePagesParameters> => ({
  pageNumbers: apiParams.pageNumbers ?? defaultParameters.pageNumbers,
});

export const buildRemovePagesFormData = (
  parameters: RemovePagesParameters,
  file: File,
): FormData =>
  objectToFormData(
    { pageNumbers: parameters.pageNumbers.replace(/\s+/g, "") },
    { fileInput: file },
  );

/**
 * Local (offline) processor — replaces the removed backend endpoint.
 *
 * Uses @cantoo/pdf-lib to remove the specified pages entirely in the browser.
 * No network request is made.
 */
async function removePagesCustomProcessor(
  params: RemovePagesParameters,
  files: File[],
): Promise<{ files: File[]; consumedAllInputs: boolean }> {
  if (files.length === 0) {
    throw new Error("No file provided for page removal.");
  }

  const file = files[0];
  const blob = await removePagesLocal(file, params.pageNumbers);

  // Preserve the original filename with a suffix
  const baseName = file.name.replace(/\.pdf$/iu, "");
  const outFile = new File([blob], `${baseName}_removed.pdf`, {
    type: "application/pdf",
  });

  return { files: [outFile], consumedAllInputs: true };
}

export const removePagesOperationConfig = {
  toolType: "custom" as const,
  operationType: "removePages",
  defaultParameters,
  validateParams: validateRemovePagesParameters,
  customProcessor: removePagesCustomProcessor,
  toApiParams: removePagesToApiParams,
  fromApiParams: removePagesFromApiParams,
};

export const useRemovePagesOperation = () => {
  const { t } = useTranslation();

  return useToolOperation<RemovePagesParameters>({
    ...removePagesOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("removePages.error.failed", "Failed to remove pages"),
    ),
  });
};
