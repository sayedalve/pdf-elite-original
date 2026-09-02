import { useState, useCallback } from "react";
import type { ChangeMetadataParameters } from "@app/hooks/tools/changeMetadata/useChangeMetadataParameters";

// ─── Types ────────────────────────────────────────────────────────────────────

/** Shape of PDF document metadata as read from the backend. */
export interface PdfMetadata {
  title?: string;
  author?: string;
  subject?: string;
  keywords?: string;
  producer?: string;
  creator?: string;
  creationDate?: string;
  modificationDate?: string;
  [key: string]: string | undefined;
}

type OnParameterChange = <K extends keyof ChangeMetadataParameters>(
  key: K,
  value: ChangeMetadataParameters[K],
) => void;

interface UseMetadataExtractionOptions {
  /** The updateParameter callback from the tool's parameters hook. */
  updateParameter?: OnParameterChange;
}

export interface MetadataExtractionHook {
  /** Currently loaded/edited metadata values. */
  metadata: PdfMetadata;
  /** True while metadata is being fetched from the PDF. */
  isLoading: boolean;
  /** True while an extraction request is in progress. */
  isExtractingMetadata: boolean;
  /** Update a single metadata field. */
  updateField: (key: keyof PdfMetadata, value: string) => void;
  /** Overwrite all metadata with a freshly extracted set. */
  setMetadata: (data: PdfMetadata) => void;
  /** Reset back to the last extracted values. */
  reset: () => void;
}

const EMPTY: PdfMetadata = {};

/**
 * Manages extraction and editing of PDF document metadata.
 * When `updateParameter` is provided, changes are forwarded to the tool's
 * parameter hook so they are included in the next operation invocation.
 */
export function useMetadataExtraction(
  options?: UseMetadataExtractionOptions,
): MetadataExtractionHook {
  const [metadata, setMetadataState] = useState<PdfMetadata>(EMPTY);
  const [original, setOriginal] = useState<PdfMetadata>(EMPTY);
  const [isLoading, setIsLoading] = useState(false);
  const [isExtractingMetadata, setIsExtractingMetadata] = useState(false);

  const updateField = useCallback(
    (key: keyof PdfMetadata, value: string) => {
      setMetadataState((prev) => ({ ...prev, [key]: value }));
      // Forward to the parameters hook if wired up
      if (options?.updateParameter) {
        options.updateParameter(
          key as keyof ChangeMetadataParameters,
          value as never,
        );
      }
    },
    [options],
  );

  const setMetadata = useCallback((data: PdfMetadata) => {
    setIsLoading(false);
    setIsExtractingMetadata(false);
    setMetadataState(data);
    setOriginal(data);
  }, []);

  const reset = useCallback(() => {
    setMetadataState(original);
  }, [original]);

  return {
    metadata,
    isLoading,
    isExtractingMetadata,
    updateField,
    setMetadata,
    reset,
  };
}
