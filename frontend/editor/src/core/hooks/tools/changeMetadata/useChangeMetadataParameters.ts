import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

// ─── Types ────────────────────────────────────────────────────────────────────

export interface CustomMetadataEntry {
  key: string;
  value: string;
}

export interface ChangeMetadataParameters extends BaseParameters {
  title?: string;
  author?: string;
  subject?: string;
  keywords?: string;
  producer?: string;
  creator?: string;
  creationDate?: string;
  modificationDate?: string;
  /** Whether to delete ALL existing metadata before applying new values. */
  deleteAll: boolean;
  trapped?: string;
  /** User-defined custom metadata key-value pairs. */
  customMetadata: CustomMetadataEntry[];
}

export const defaultParameters: ChangeMetadataParameters = {
  deleteAll: false,
  customMetadata: [],
};

export type ChangeMetadataParametersHook =
  BaseParametersHook<ChangeMetadataParameters>;

export const useChangeMetadataParameters = (): ChangeMetadataParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "update-metadata" });

// ─── Custom metadata CRUD helpers ─────────────────────────────────────────────

type OnParameterChange = <K extends keyof ChangeMetadataParameters>(
  key: K,
  value: ChangeMetadataParameters[K],
) => void;

/**
 * Returns add/remove/update helpers that modify the customMetadata array
 * inside ChangeMetadataParameters via the standard onParameterChange callback.
 */
export function createCustomMetadataFunctions(
  parameters: ChangeMetadataParameters,
  onParameterChange?: OnParameterChange,
) {
  const entries = parameters.customMetadata ?? [];

  function addCustomMetadata() {
    const next: CustomMetadataEntry[] = [...entries, { key: "", value: "" }];
    onParameterChange?.("customMetadata", next);
  }

  function removeCustomMetadata(index: number) {
    const next = entries.filter((_, i) => i !== index);
    onParameterChange?.("customMetadata", next);
  }

  function updateCustomMetadata(
    index: number,
    field: keyof CustomMetadataEntry,
    value: string,
  ) {
    const next = entries.map((entry, i) =>
      i === index ? { ...entry, [field]: value } : entry,
    );
    onParameterChange?.("customMetadata", next);
  }

  return { addCustomMetadata, removeCustomMetadata, updateCustomMetadata };
}
