import React from "react";

// Erased (untyped) form of tool parameters — used at registry and executor boundaries.
export type ErasedToolParams = any;

// Why the cursor is on the execute button but it's disabled.
export type ExecuteDisabledReason =
  | "endpointUnavailable"
  | "filesLoading"
  | "noFiles"
  | "invalidParams"
  | "viewerMode"
  | null;

// Props passed to the per-tool automation settings component.
export interface ToolAutomationSettingsProps<TParams = ErasedToolParams> {
  parameters: TParams;
  onParameterChange?: <K extends keyof TParams>(
    key: K,
    value: TParams[K],
  ) => void;
  onChange?: (params: Partial<TParams>) => void;
  disabled?: boolean;
}

// Config for tools that post one file at a time.
export interface SingleFileToolOperationConfig<TParams = ErasedToolParams> {
  toolType: "singleFile";
  /** Static endpoint string or a function that picks the endpoint from params. */
  endpoint: string | ((params: TParams) => string);
  /** All endpoints this tool may use (for registry queries). */
  endpoints?: string[];
  buildFormData: (params: TParams, file: File) => FormData;
  toApiParams?: (params: TParams) => ErasedToolParams;
  fromApiParams?: (apiParams: ErasedToolParams) => Partial<TParams>;
  operationType: string;
  defaultParameters?: TParams;
  validateParams?: (params: TParams) => boolean;
  filePrefix?: string;
  preserveBackendFilename?: boolean;
  customProcessor?: undefined;
  responseHandler?: (blob: Blob, files: File[]) => Promise<File[]>;
  getErrorMessage?: (error: unknown) => string;
}

// Config for tools that post all files in a single request.
export interface MultiFileToolOperationConfig<TParams = ErasedToolParams> {
  toolType: "multiFile";
  endpoint: string | ((params: TParams) => string);
  endpoints?: string[];
  buildFormData: (params: TParams, files: File[]) => FormData;
  toApiParams?: (params: TParams) => ErasedToolParams;
  fromApiParams?: (apiParams: ErasedToolParams) => Partial<TParams>;
  operationType: string;
  defaultParameters?: TParams;
  validateParams?: (params: TParams) => boolean;
  filePrefix?: string;
  preserveBackendFilename?: boolean;
  customProcessor?: undefined;
  getErrorMessage?: (error: unknown) => string;
}

// Config for tools with bespoke processing logic (not a single REST call).
export interface CustomToolOperationConfig<TParams = ErasedToolParams> {
  toolType: "custom";
  operationType: string;
  defaultParameters?: TParams;
  validateParams?: (params: TParams) => boolean;
  customProcessor: (
    params: TParams,
    files: File[],
  ) => Promise<{ files: File[]; consumedAllInputs?: boolean }>;
  endpoint?: undefined;
  buildFormData?: undefined;
  toApiParams?: (params: TParams) => ErasedToolParams;
  fromApiParams?: (apiParams: ErasedToolParams) => Partial<TParams>;
  filePrefix?: string;
  getErrorMessage?: (error: unknown) => string;
}

export type ToolOperationConfig<TParams = ErasedToolParams> =
  | SingleFileToolOperationConfig<TParams>
  | MultiFileToolOperationConfig<TParams>
  | CustomToolOperationConfig<TParams>;

/**
 * Erase the TParams type parameter so the config can be stored in a registry
 * keyed by tool ID without carrying the concrete parameter type.
 */
export function asRegistryConfig<TParams>(
  config: ToolOperationConfig<TParams>,
): ToolOperationConfig<ErasedToolParams> {
  return config as unknown as ToolOperationConfig<ErasedToolParams>;
}

/**
 * Wrap a lazily-loaded per-tool settings component so its props match the
 * erased registry type without needing a cast at every call site.
 */
export function lazySettings<TParams>(
  component: React.ComponentType<ToolAutomationSettingsProps<TParams>>,
): React.ComponentType<ToolAutomationSettingsProps<ErasedToolParams>> {
  return component as unknown as React.ComponentType<
    ToolAutomationSettingsProps<ErasedToolParams>
  >;
}
