// Re-export the generated API types so tool files import from one canonical place.
export type { ToolApiParams, ToolEndpoint } from "@app/types/toolApiTypes";

/**
 * Append API parameters and optional file inputs to a FormData object.
 *
 * @param apiParams  Flat object of request body fields (booleans, numbers, strings).
 * @param files      Optional map of field names to File or File[].
 */
export function objectToFormData(
  apiParams: Record<string, unknown>,
  files?: { [fieldName: string]: File | File[] },
): FormData {
  const formData = new FormData();

  // Append files first so they appear before params in the multipart body.
  if (files) {
    for (const [fieldName, value] of Object.entries(files)) {
      if (Array.isArray(value)) {
        for (const file of value) {
          formData.append(fieldName, file);
        }
      } else {
        formData.append(fieldName, value);
      }
    }
  }

  // Append scalar parameters.
  for (const [key, value] of Object.entries(apiParams)) {
    if (value === undefined || value === null) continue;
    if (Array.isArray(value)) {
      for (const item of value) {
        formData.append(key, String(item));
      }
    } else {
      formData.append(key, String(value));
    }
  }

  return formData;
}

/**
 * Helper for tools whose only input is a file (no extra API parameters).
 * Returns identity mappers so the tool can still be serialised/deserialised.
 */
export function fileOnlyMapping(): {
  toApiParams: () => Record<string, never>;
  fromApiParams: () => Record<string, never>;
} {
  return {
    toApiParams: () => ({}),
    fromApiParams: () => ({}),
  };
}
