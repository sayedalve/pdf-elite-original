/**
 * toolAutomation.ts
 *
 * Types and utilities that bridge the tool registry with the pipeline / automation
 * execution system. This module is the single source of truth for the shape of a
 * "working step" (an in-progress pipeline step with its current parameter state).
 */

import type {
  ErasedToolParams,
  ToolOperationConfig,
} from "@app/hooks/tools/shared/toolOperationTypes";
import type { ToolRegistryCatalog } from "@app/contexts/ToolRegistryContext";

// ─── ExecutableTool ───────────────────────────────────────────────────────────

/**
 * A registry entry that has enough information for the pipeline builder to
 * dispatch it: it has an operation config and knows which API endpoint(s) it calls.
 */
export interface ExecutableTool {
  id: string;
  operation: string;
  name: string;
  description?: string;
  icon?: string;
  operationConfig?: ToolOperationConfig<ErasedToolParams>;
  /** Endpoints this tool may call, used by the IO compatibility checker. */
  endpoints?: string[];
  subcategoryId?: string;
}

// ─── WorkingToolStep ──────────────────────────────────────────────────────────

/**
 * An in-flight pipeline step, carrying the tool identity and the user's current
 * parameter values. This is the unit stored in the PipelineBuilder's local state.
 */
export interface WorkingToolStep {
  /** Tool ID from the registry, or null for integration steps. */
  toolId: string | null;
  /** API operation string (matches the backend's pipeline step `operation` field). */
  operation: string;
  /** Current user-edited parameter values. */
  parameters: ErasedToolParams;
  /** True when the user has explicitly configured this step (vs. keeping defaults). */
  configured: boolean;
  /** Uploaded file, if this step requires a static input file. */
  uploadedFile?: File | null;
}

// ─── Factory ──────────────────────────────────────────────────────────────────

/**
 * Create a blank WorkingToolStep for a tool, pre-filled with the tool's default
 * parameters (if any).
 */
export function newWorkingToolStep(tool: ExecutableTool): WorkingToolStep {
  const defaults = tool.operationConfig?.defaultParameters ?? {};
  return {
    toolId: tool.id,
    operation: tool.operation,
    parameters: { ...defaults },
    configured: false,
    uploadedFile: null,
  };
}

// ─── Queries ──────────────────────────────────────────────────────────────────

/**
 * Returns true when the step still needs the user to configure it before the
 * pipeline can run. A step is considered configured when either:
 * - it has been explicitly marked configured, or
 * - its operation config reports valid parameters.
 */
export function stepNeedsConfiguring(
  step: WorkingToolStep,
  tool?: ExecutableTool,
): boolean {
  if (step.configured) return false;
  const validateFn = tool?.operationConfig?.validateParams;
  if (validateFn) return !validateFn(step.parameters);
  return false;
}

/**
 * Returns true when the step requires an uploaded file to be provided by the
 * user (i.e. it is not a pure document-processing step that uses the pipeline's
 * flowing document as input).
 */
export function stepRequiresUpload(
  _step: WorkingToolStep,
  _tool?: ExecutableTool,
): boolean {
  // For now, no standard tool requires a separate upload in the pipeline context.
  // Proprietary or integration steps can override this logic.
  return false;
}

// ─── Serialisation ────────────────────────────────────────────────────────────

export interface SerializedToolStep {
  toolId: string | null;
  operation: string;
  parameters: ErasedToolParams;
}

/** Convert a WorkingToolStep to the format expected by the backend. */
export function serializeToolStep(step: WorkingToolStep): SerializedToolStep {
  return {
    toolId: step.toolId,
    operation: step.operation,
    parameters: step.parameters,
  };
}

/** Reconstruct a WorkingToolStep from a serialised backend step. */
export function deserializeToolStep(
  serialized: SerializedToolStep,
): WorkingToolStep {
  return {
    toolId: serialized.toolId,
    operation: serialized.operation,
    parameters: serialized.parameters ?? {},
    configured: Object.keys(serialized.parameters ?? {}).length > 0,
    uploadedFile: null,
  };
}

// ─── Registry helpers ─────────────────────────────────────────────────────────

/**
 * Extract all ExecutableTool entries from the full tool registry catalog.
 * Only tools that have an operationConfig (i.e. are automatable) are returned.
 */
export function getExecutableTools(
  catalog: ToolRegistryCatalog,
): ExecutableTool[] {
  const { regularTools } = catalog;
  return Object.entries(regularTools)
    .filter(([, entry]) => entry.operationConfig !== undefined)
    .map(([id, entry]) => ({
      id,
      operation: entry.operationConfig?.operationType ?? id,
      name: entry.name ?? id,
      description: entry.description,
      operationConfig: entry.operationConfig,
      endpoints: (entry.operationConfig as any)?.endpoints,
    }));
}
