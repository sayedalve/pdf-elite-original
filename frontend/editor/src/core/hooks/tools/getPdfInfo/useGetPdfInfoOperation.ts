import { useState, useCallback } from "react";
import apiClient from "@app/services/apiClient";

// ─── Types ────────────────────────────────────────────────────────────────────

export interface PdfInfo {
  fileName?: string;
  pageCount?: number;
  fileSize?: number;
  pdfVersion?: string;
  title?: string;
  author?: string;
  subject?: string;
  keywords?: string;
  creator?: string;
  producer?: string;
  creationDate?: string;
  modificationDate?: string;
  encrypted?: boolean;
  [key: string]: string | number | boolean | undefined;
}

export interface GetPdfInfoOperationHook {
  isLoading: boolean;
  files: File[];
  results: PdfInfo[];
  executeOperation: (
    params: Record<string, unknown>,
    files: File[],
  ) => Promise<void>;
  resetResults: () => void;
  undoOperation: () => Promise<void>;
}

// ─── Hook ────────────────────────────────────────────────────────────────────

export function useGetPdfInfoOperation(): GetPdfInfoOperationHook {
  const [isLoading, setIsLoading] = useState(false);
  const [files, setFiles] = useState<File[]>([]);
  const [results, setResults] = useState<PdfInfo[]>([]);

  const executeOperation = useCallback(
    async (_params: Record<string, unknown>, inputFiles: File[]) => {
      if (inputFiles.length === 0) return;
      setIsLoading(true);
      try {
        const formData = new FormData();
        formData.append("fileInput", inputFiles[0]);
        const response = await apiClient.post(
          "/api/v1/misc/get-info-on-pdf",
          formData,
        );
        const info: PdfInfo = (response.data as PdfInfo) ?? {};
        setResults([info]);
        // Produce a JSON file so components can call findFileByExtension(".json")
        const jsonBlob = new Blob([JSON.stringify(info, null, 2)], {
          type: "application/json",
        });
        const jsonFile = new File([jsonBlob], "pdf-info.json");
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
