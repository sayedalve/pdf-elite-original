/* eslint-disable */
import { useCallback, useMemo, useState } from "react";
import { useViewScopedFiles } from "@app/hooks/tools/shared/useViewScopedFiles";
import { PDFEliteFile } from "@app/types/fileContext";

// Shape returned by every tool's useBaseTool() call.
export interface BaseToolReturn<TParams, TOperation> {
  selectedFiles: PDFEliteFile[];
  hasFiles: boolean;
  hasResults: boolean;
  settingsCollapsed: boolean;
  endpointEnabled: boolean;
  endpointLoading: boolean;
  params: TParams;
  operation: TOperation;
  handleExecute: () => Promise<void>;
  handleSettingsReset: () => void;
  handleThumbnailClick: (file: File) => void;
  handleUndo: () => Promise<void>;
}

export interface BaseToolOptions {
  /** Minimum number of files required to enable execution. */
  minFiles?: number;
  /** When true, the tool ignores the current viewer scope and uses all files. */
  ignoreViewerScope?: boolean;
}

/**
 * Shared base hook for all tool components.
 * Wires together the parameters hook and operation hook with file selection,
 * settings collapse, and result tracking.
 */
export function useBaseTool<
  TParams extends { validateParameters: () => boolean; parameters: any },
  TOperation extends {
    isLoading: boolean;
    files: File[];
    executeOperation: (params: any, files: File[]) => Promise<void>;
    resetResults: () => void;
    undoOperation: () => Promise<void>;
  },
>(
  toolId: string,
  useParametersHook: () => TParams,
  useOperationHook: () => TOperation,
  _props: unknown,
  options: BaseToolOptions = {},
): BaseToolReturn<TParams, TOperation> {
  const { minFiles = 1 } = options;

  const params = useParametersHook();
  const operation = useOperationHook();
  const viewFiles = useViewScopedFiles();

  const [settingsCollapsed, setSettingsCollapsed] = useState(false);

  const selectedFiles = useMemo(
    () => viewFiles.slice(0, Math.max(viewFiles.length, minFiles)),
    [viewFiles],
  );

  const hasFiles = selectedFiles.length >= minFiles;
  const hasResults = operation.files.length > 0;

  const handleExecute = useCallback(async () => {
    if (!hasFiles) return;
    setSettingsCollapsed(true);
    // PDFEliteFile extends File, so we can pass selectedFiles directly
    const rawFiles = selectedFiles;
    await operation.executeOperation(params.parameters, rawFiles);
  }, [hasFiles, operation, params.parameters, selectedFiles]);

  const handleSettingsReset = useCallback(() => {
    operation.resetResults();
    setSettingsCollapsed(false);
  }, [operation]);

  const handleThumbnailClick = useCallback((_file: File) => {
    // No-op base implementation
  }, []);

  const handleUndo = useCallback(async () => {
    await operation.undoOperation();
    setSettingsCollapsed(false);
  }, [operation]);

  return {
    selectedFiles,
    hasFiles,
    hasResults,
    settingsCollapsed,
    endpointEnabled: hasFiles && params.validateParameters(),
    endpointLoading: operation.isLoading,
    params,
    operation,
    handleExecute,
    handleSettingsReset,
    handleThumbnailClick,
    handleUndo,
  };
}
