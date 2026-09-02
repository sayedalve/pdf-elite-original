import { useState, useCallback } from "react";
import apiClient from "@app/services/apiClient";
import type { ValidateSignatureParameters } from "@app/hooks/tools/validateSignature/useValidateSignatureParameters";

// ─── Types ────────────────────────────────────────────────────────────────────

export interface SignatureValidationResult {
  signatureValid?: boolean;
  certValid?: boolean;
  documentUnmodified?: boolean;
  signerName?: string;
  signDate?: string;
  reason?: string;
  location?: string;
  contactInfo?: string;
  status?: string;
  [key: string]: string | boolean | undefined;
}

export interface ValidateSignatureOperationHook {
  isLoading: boolean;
  /** Output files: may include a .pdf, .csv, and .json result file. */
  files: File[];
  results: SignatureValidationResult[];
  executeOperation: (
    params: ValidateSignatureParameters,
    files: File[],
  ) => Promise<void>;
  resetResults: () => void;
  undoOperation: () => Promise<void>;
}

// ─── Hook ────────────────────────────────────────────────────────────────────

export function useValidateSignatureOperation(): ValidateSignatureOperationHook {
  const [isLoading, setIsLoading] = useState(false);
  const [files, setFiles] = useState<File[]>([]);
  const [results, setResults] = useState<SignatureValidationResult[]>([]);

  const executeOperation = useCallback(
    async (_params: ValidateSignatureParameters, inputFiles: File[]) => {
      if (inputFiles.length === 0) return;
      setIsLoading(true);
      try {
        const formData = new FormData();
        formData.append("fileInput", inputFiles[0]);
        const response = await apiClient.post(
          "/api/v1/security/validate-signature",
          formData,
        );
        const data = response.data as
          | SignatureValidationResult
          | SignatureValidationResult[];
        const resultList = Array.isArray(data) ? data : [data];
        setResults(resultList);

        // Produce a JSON file so components can call findFileByExtension(".json")
        const jsonBlob = new Blob([JSON.stringify(resultList, null, 2)], {
          type: "application/json",
        });
        const jsonFile = new File([jsonBlob], "validation-result.json");
        setFiles([jsonFile]);
      } catch {
        setResults([]);
        setFiles([]);
      } finally {
        setIsLoading(false);
      }
    },
    [],
  );

  const resetResults = useCallback(() => {
    setResults([]);
    setFiles([]);
  }, []);

  const undoOperation = useCallback(async () => {
    resetResults();
  }, [resetResults]);

  return {
    isLoading,
    files,
    results,
    executeOperation,
    resetResults,
    undoOperation,
  };
}
