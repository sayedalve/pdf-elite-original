/* eslint-disable */
import {
  objectToFormData,
  type ToolApiParams,
  type ToolEndpoint,
} from "@app/hooks/tools/shared/toolApiMapping";
import type { RemovePasswordParameters } from "@app/hooks/tools/removePassword/useRemovePasswordParameters";

const ENDPOINT = "/api/v1/security/remove-password" satisfies ToolEndpoint;
type RemovePasswordApiParams = ToolApiParams[typeof ENDPOINT];

/** Convert UI parameters into the remove-password request body. */
const toApiParams = (
  params: RemovePasswordParameters,
): RemovePasswordApiParams => ({
  password: params.password,
});

/**
 * Build the FormData payload for the /api/v1/security/remove-password endpoint.
 * Called by FileContext when unlocking an encrypted PDF.
 */
export const buildRemovePasswordFormData = (
  params: RemovePasswordParameters,
  file: File,
): FormData =>
  objectToFormData(toApiParams(params) as Record<string, unknown>, {
    fileInput: file,
  });
