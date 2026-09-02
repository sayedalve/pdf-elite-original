import { useCallback } from "react";
import { zipFileService } from "@app/services/zipFileService";

export interface ToolResources {
  /**
   * Extract files from a ZIP blob, respecting user auto-unzip preferences.
   * Falls back to returning the ZIP as-is when extraction fails.
   */
  extractZipFiles: (blob: Blob) => Promise<File[]>;
}

/**
 * Provides shared resource utilities (e.g., ZIP extraction) to tool operation hooks.
 * Centralises the logic so individual tools don't re-implement it.
 */
export function useToolResources(): ToolResources {
  const extractZipFiles = useCallback(async (blob: Blob): Promise<File[]> => {
    try {
      const zipFile = new File([blob], `result_${Date.now()}.zip`, {
        type: "application/zip",
      });

      const result = await zipFileService.extractAllFiles(zipFile);

      if (result.success && result.extractedFiles.length > 0) {
        return result.extractedFiles;
      }

      // Fallback: return the ZIP as-is
      return [zipFile];
    } catch {
      // If extraction fails, return the original blob as a file
      return [
        new File([blob], `result_${Date.now()}.zip`, {
          type: "application/zip",
        }),
      ];
    }
  }, []);

  return { extractZipFiles };
}
