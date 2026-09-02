/**
 * Type safety declarations to prevent file.name/UUID confusion
 */

import { FileId, PDFEliteFile } from "@app/types/fileContext";

declare global {
  namespace FileIdSafety {
    // Mark functions that should never accept file.name as parameters
    type SafeFileIdFunction<T extends (...args: any[]) => any> = T extends (
      ...args: infer P
    ) => infer _R
      ? P extends readonly [string, ...any[]]
        ? never // Reject string parameters in first position for FileId functions
        : T
      : T;

    // Mark functions that should only accept PDFEliteFile, not regular File
    type PDFEliteFileOnlyFunction<T extends (...args: any[]) => any> =
      T extends (...args: infer P) => infer _R
        ? P extends readonly [File, ...any[]]
          ? never // Reject File parameters in first position for PDFEliteFile functions
          : T
        : T;

    // Utility type to enforce PDFEliteFile usage
    type RequirePDFEliteFile<T> = T extends File ? PDFEliteFile : T;
  }

  // Extend Window interface for debugging
  interface Window {
    __FILE_ID_DEBUG?: boolean;
  }
}

// Augment FileContext types to prevent bypassing PDFEliteFile
declare module "../contexts/FileContext" {
  export interface StrictFileContextActions {
    pinFile: (file: PDFEliteFile) => void; // Must be PDFEliteFile
    unpinFile: (file: PDFEliteFile) => void; // Must be PDFEliteFile
    addFiles: (
      files: File[],
      options?: { insertAfterPageId?: string },
    ) => Promise<PDFEliteFile[]>; // Returns PDFEliteFile
    consumeFiles: (
      inputFileIds: FileId[],
      outputFiles: File[],
    ) => Promise<PDFEliteFile[]>; // Returns PDFEliteFile
  }

  export interface StrictFileContextSelectors {
    getFile: (id: FileId) => PDFEliteFile | undefined; // Returns PDFEliteFile
    getFiles: (ids?: FileId[]) => PDFEliteFile[]; // Returns PDFEliteFile[]
    isFilePinned: (file: PDFEliteFile) => boolean; // Must be PDFEliteFile
  }
}

export {};
