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
import type { RemovePasswordParameters } from "@app/hooks/tools/removePassword/useRemovePasswordParameters";

const ENDPOINT = "/api/v1/security/remove-password" satisfies ToolEndpoint;
type RemovePasswordApiParams = ToolApiParams[typeof ENDPOINT];

export const removePasswordToApiParams = (
  parameters: RemovePasswordParameters,
): RemovePasswordApiParams => ({
  password: parameters.password,
});

export const removePasswordFromApiParams = (
  apiParams: RemovePasswordApiParams,
): Partial<RemovePasswordParameters> => ({
  password: apiParams.password ?? "",
});

export const buildRemovePasswordOperationFormData = (
  parameters: RemovePasswordParameters,
  file: File,
): FormData =>
  objectToFormData(
    removePasswordToApiParams(parameters) as Record<string, unknown>,
    { fileInput: file },
  );

export const removePasswordOperationConfig = defineSingleFileTool({
  buildFormData: buildRemovePasswordOperationFormData,
  toApiParams: removePasswordToApiParams as unknown as (
    params: RemovePasswordParameters,
  ) => any,
  fromApiParams: removePasswordFromApiParams as unknown as (
    apiParams: any,
  ) => Partial<RemovePasswordParameters>,
  operationType: "removePassword",
  endpoint: ENDPOINT,
  defaultParameters: { password: "" } as RemovePasswordParameters,
});

export const useRemovePasswordOperation = () => {
  const { t } = useTranslation();

  return useToolOperation<RemovePasswordParameters>({
    ...removePasswordOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("removePassword.error.failed", "Failed to remove the PDF password."),
    ),
  });
};
