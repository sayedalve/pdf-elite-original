import { useState, useCallback, useRef } from "react";
import apiClient from "@app/services/apiClient";
import { processResponse } from "@app/utils/toolResponseProcessor";
import {
  type SingleFileToolOperationConfig,
  type MultiFileToolOperationConfig,
  type CustomToolOperationConfig,
  type ToolOperationConfig,
  type ErasedToolParams,
} from "@app/hooks/tools/shared/toolOperationTypes";
import { useFileContext } from "@app/contexts/FileContext";
import { createChildStub } from "@app/contexts/file/fileActions";
import {
  PDFEliteFile,
  createFileId,
  createPDFEliteFile,
} from "@app/types/fileContext";

// Re-export config types so consumers can import them from one place.
export type {
  SingleFileToolOperationConfig,
  MultiFileToolOperationConfig,
  CustomToolOperationConfig,
  ToolOperationConfig,
  ErasedToolParams,
} from "@app/hooks/tools/shared/toolOperationTypes";

// ─── ToolType ────────────────────────────────────────────────────────────────

/** Discriminant that drives the automation executor's dispatch logic. */
export enum ToolType {
  singleFile = "singleFile",
  multiFile = "multiFile",
  custom = "custom",
}

// ─── CustomProcessorResult ───────────────────────────────────────────────────

export interface CustomProcessorResult {
  files: File[];
  consumedAllInputs?: boolean;
}

// ─── ToolOperationHook ───────────────────────────────────────────────────────

/**
 * Shape returned by every `useXxxOperation()` hook.
 * ReviewToolStep and createToolFlow consume this interface.
 */
export interface ToolOperationHook<TParams = unknown> {
  /** Result files produced by the last successful operation. */
  files: File[];
  /** Thumbnail data URLs aligned with `files` (null while generating). */
  thumbnails: (string | null)[];
  /** True while an API request is in flight. */
  isLoading: boolean;
  /** Human-readable error from the last failed operation. */
  errorMessage: string | null;
  /** Current status label (empty string when idle). */
  status: string;
  /** True while thumbnails are still being generated. */
  isGeneratingThumbnails: boolean;
  /** 0–100 progress value for the current operation, or null when indeterminate. */
  progress: number | null;
  /** Blob URL for downloading the result, or null. */
  downloadUrl: string | null;
  /** Suggested filename for the download. */
  downloadFilename: string;
  /** Local file-system path (desktop builds only), or null. */
  downloadLocalPath: string | null;
  /** FileContext IDs of the output files (for save / undo support). */
  outputFileIds: string[];
  /** True when the cloud backend will process this request. */
  willUseCloud?: boolean | null;
  /** Run the tool operation. */
  executeOperation: (
    params: TParams,
    files: File[],
    inputPDFEliteFiles?: PDFEliteFile[],
  ) => Promise<void>;
  /** Clear result files and reset to idle. */
  resetResults: () => void;
  /** Dismiss the current error message. */
  clearError: () => void;
  /** Cancel an in-flight operation (best-effort). */
  cancelOperation: () => void;
  /** Undo the last operation, restoring the original files. */
  undoOperation: () => Promise<void>;
}

// ─── Factory functions ────────────────────────────────────────────────────────

/** Build a single-file operation config with the toolType discriminant pre-set. */
export function defineSingleFileTool<TParams = ErasedToolParams>(
  config: Omit<SingleFileToolOperationConfig<TParams>, "toolType">,
): SingleFileToolOperationConfig<TParams> {
  return {
    ...config,
    toolType: ToolType.singleFile,
  } as SingleFileToolOperationConfig<TParams>;
}

/** Build a multi-file operation config with the toolType discriminant pre-set. */
export function defineMultiFileTool<TParams = ErasedToolParams>(
  config: Omit<MultiFileToolOperationConfig<TParams>, "toolType">,
): MultiFileToolOperationConfig<TParams> {
  return {
    ...config,
    toolType: ToolType.multiFile,
  } as MultiFileToolOperationConfig<TParams>;
}

/** Build a custom-processor operation config with the toolType discriminant pre-set. */
export function defineCustomTool<TParams = ErasedToolParams>(
  config: Omit<CustomToolOperationConfig<TParams>, "toolType">,
): CustomToolOperationConfig<TParams> {
  return {
    ...config,
    toolType: ToolType.custom,
  } as CustomToolOperationConfig<TParams>;
}

// ─── useToolOperation ─────────────────────────────────────────────────────────

/**
 * Core React hook that executes a tool operation and manages all UI state
 * (loading, results, errors, thumbnails, download URL).
 *
 * Every `useXxxOperation()` hook delegates to this one after building its
 * static `ToolOperationConfig` with `defineSingleFileTool` / `defineMultiFileTool`
 * / `defineCustomTool`.
 */
export function useToolOperation<TParams>(
  config: ToolOperationConfig<TParams>,
): ToolOperationHook<TParams> {
  const [isLoading, setIsLoading] = useState(false);
  const [errorMessage, setErrorMessage] = useState<string | null>(null);
  const [files, setFiles] = useState<File[]>([]);
  const [thumbnails, setThumbnails] = useState<(string | null)[]>([]);
  const [downloadUrl, setDownloadUrl] = useState<string | null>(null);
  const [downloadFilename, setDownloadFilename] = useState<string>("");
  const [isGeneratingThumbnails, setIsGeneratingThumbnails] = useState(false);
  const [status, setStatus] = useState<string>("");
  const [progress, setProgress] = useState<number | null>(null);
  const abortRef = useRef<AbortController | null>(null);

  const fileContext = useFileContext();
  const { consumeFiles, undoConsumeFiles, selectors } = fileContext || {};

  const clearError = useCallback(() => setErrorMessage(null), []);

  const resetResults = useCallback(() => {
    setFiles([]);
    setThumbnails([]);
    setDownloadUrl(null);
    setDownloadFilename("");
    setErrorMessage(null);
    setStatus("");
    setProgress(null);
  }, []);

  const [outputFileIds, setOutputFileIds] = useState<string[]>([]);
  const [inputPDFEliteFileStubs, setInputPDFEliteFileStubs] = useState<any[]>(
    [],
  );

  const cancelOperation = useCallback(() => {
    abortRef.current?.abort();
    setIsLoading(false);
    setStatus("");
  }, []);

  const undoOperation = useCallback(async () => {
    if (
      undoConsumeFiles &&
      outputFileIds.length > 0 &&
      inputPDFEliteFileStubs.length > 0
    ) {
      await undoConsumeFiles([], inputPDFEliteFileStubs, outputFileIds as any);
      setOutputFileIds([]);
      setInputPDFEliteFileStubs([]);
    }
    resetResults();
  }, [undoConsumeFiles, outputFileIds, inputPDFEliteFileStubs, resetResults]);

  const storeResults = useCallback(
    async (resultFiles: File[], inputPDFEliteFiles?: PDFEliteFile[]) => {
      setFiles(resultFiles);

      // Generate a simple download URL for single-file results
      if (resultFiles.length === 1) {
        const url = URL.createObjectURL(resultFiles[0]);
        setDownloadUrl(url);
        setDownloadFilename(resultFiles[0].name);
      } else if (resultFiles.length > 1) {
        // Multiple files — no single download URL (use individual file URLs)
        setDownloadUrl(null);
        setDownloadFilename("");
      }

      // Update FileContext for robust save/undo chaining
      if (
        consumeFiles &&
        selectors &&
        inputPDFEliteFiles &&
        inputPDFEliteFiles.length > 0
      ) {
        const newPDFEliteFiles = resultFiles.map((f) =>
          createPDFEliteFile(f, createFileId()),
        );

        // Determine parents (using first input as primary parent for simplicty in multi-file operations)
        const primaryInputId = inputPDFEliteFiles[0].fileId;
        const parentStub = selectors.getPDFEliteFileStub(primaryInputId);

        if (parentStub) {
          const stubs = newPDFEliteFiles.map((sf) => {
            return createChildStub(
              parentStub,
              {
                toolId: config.operationType as any,
                timestamp: Date.now(),
              },
              sf,
            );
          });

          const inputFileIds = inputPDFEliteFiles.map((f) => f.fileId);
          const originalInputStubs = inputFileIds
            .map((id) => selectors.getPDFEliteFileStub(id))
            .filter(Boolean);

          try {
            const newIds = await consumeFiles(
              inputFileIds,
              newPDFEliteFiles,
              stubs,
            );
            setOutputFileIds(newIds);
            setInputPDFEliteFileStubs(originalInputStubs);
          } catch (e) {
            console.error("[useToolOperation] Failed to consume files:", e);
          }
        }
      }

      // Generate blank thumbnails placeholders (real thumbnails generated elsewhere)
      setIsGeneratingThumbnails(true);
      setThumbnails(resultFiles.map(() => null));
      setIsGeneratingThumbnails(false);
    },
    [consumeFiles, selectors, config.operationType],
  );

  const executeOperation = useCallback(
    async (
      params: TParams,
      inputFiles: File[],
      inputPDFEliteFiles?: PDFEliteFile[],
    ): Promise<void> => {
      if (isLoading) return;

      setIsLoading(true);
      setErrorMessage(null);
      setStatus("processing");
      setProgress(null);
      resetResults();

      const abortController = new AbortController();
      abortRef.current = abortController;

      try {
        let resultFiles: File[];

        if (config.toolType === ToolType.custom && config.customProcessor) {
          // ── Custom processor ──
          const result = await config.customProcessor(params, inputFiles);
          resultFiles = result.files;
        } else if (config.toolType === ToolType.multiFile) {
          // ── Multi-file: one request with all files ──
          const endpoint =
            typeof config.endpoint === "function"
              ? config.endpoint(params)
              : config.endpoint;

          const formData = (
            config as MultiFileToolOperationConfig<TParams>
          ).buildFormData(params, inputFiles);

          const response = await apiClient.post(endpoint, formData, {
            responseType: "blob",
          });

          // Use custom response handler if provided, otherwise processResponse
          const responseHandler = (
            config as MultiFileToolOperationConfig<TParams> & {
              responseHandler?: any;
            }
          ).responseHandler;
          if (responseHandler) {
            resultFiles = await responseHandler(response.data, inputFiles);
          } else {
            resultFiles = await processResponse(
              response.data,
              inputFiles,
              (config as MultiFileToolOperationConfig<TParams>).filePrefix,
            );
          }
        } else {
          // ── Single-file: one request per file ──
          const singleConfig = config as SingleFileToolOperationConfig<TParams>;
          const endpoint =
            typeof singleConfig.endpoint === "function"
              ? singleConfig.endpoint(params)
              : singleConfig.endpoint;

          resultFiles = [];
          for (const file of inputFiles) {
            const formData = singleConfig.buildFormData(params, file);
            const response = await apiClient.post(endpoint, formData, {
              responseType: "blob",
            });

            const responseHandler = singleConfig.responseHandler;
            if (responseHandler) {
              const processed = await responseHandler(response.data, [file]);
              resultFiles.push(...processed);
            } else {
              const processed = await processResponse(
                response.data,
                [file],
                singleConfig.filePrefix,
              );
              resultFiles.push(...processed);
            }
          }
        }

        await storeResults(resultFiles, inputPDFEliteFiles);
        setStatus("done");
      } catch (err: unknown) {
        if (abortController.signal.aborted) return;

        let message = "An error occurred.";
        if (config.getErrorMessage) {
          message = config.getErrorMessage(err);
        } else if (err instanceof Error) {
          message = err.message;
        }
        setErrorMessage(message);
        setStatus("error");
      } finally {
        setIsLoading(false);
        setProgress(null);
        abortRef.current = null;
      }
    },
    [config, isLoading, resetResults, storeResults],
  );

  return {
    files,
    thumbnails,
    isLoading,
    errorMessage,
    status,
    isGeneratingThumbnails,
    progress,
    downloadUrl,
    downloadFilename,
    downloadLocalPath: null,
    outputFileIds,
    willUseCloud: null,
    executeOperation,
    resetResults,
    clearError,
    cancelOperation,
    undoOperation,
  };
}
