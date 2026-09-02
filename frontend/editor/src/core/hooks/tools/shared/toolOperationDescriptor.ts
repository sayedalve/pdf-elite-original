/**
 * toolOperationDescriptor.ts
 *
 * Typed wrappers that connect a tool's operation config to the policy/pipeline
 * system. Each descriptor bundles the operation config with optional parameter
 * mapping helpers so the policy wizard can build/parse wire steps without
 * duplicating logic.
 */

import type {
  ErasedToolParams,
  ToolOperationConfig,
} from "@app/hooks/tools/shared/toolOperationTypes";

// ─── Types ────────────────────────────────────────────────────────────────────

/**
 * A self-contained description of a tool that the policy/pipeline system can
 * dispatch. Carries the operation config plus helpers for converting between
 * UI parameters and the backend wire format.
 */
export interface ToolOperationDescriptor<TParams = ErasedToolParams> {
  /** Unique stable key used to identify this operation in wire policies. */
  operationId: string;
  /** Human-readable label (falls back to operationId if omitted). */
  label?: string;
  /** The tool's operation configuration (endpoint, form builder, etc.). */
  operationConfig: ToolOperationConfig<TParams>;
  /** Convert UI parameters to the backend wire step's parameters object. */
  toWireParams?: (params: TParams) => ErasedToolParams;
  /** Convert a backend wire step's parameters back to UI parameters. */
  fromWireParams?: (wireParams: ErasedToolParams) => TParams;
}

// ─── Factory ──────────────────────────────────────────────────────────────────

/**
 * Creates a typed ToolOperationDescriptor for a given tool.
 *
 * Usage:
 * ```ts
 * export const myToolDescriptor = describeToolOperation({
 *   operationId: "my-tool",
 *   operationConfig: myToolOperationConfig,
 * });
 * ```
 */
export function describeToolOperation<TParams = ErasedToolParams>(
  descriptor: ToolOperationDescriptor<TParams>,
): ToolOperationDescriptor<TParams> {
  return descriptor;
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

/**
 * Extract the wire parameters from a descriptor, applying toWireParams if
 * present, or returning the parameters as-is.
 */
export function toWireParams<TParams>(
  descriptor: ToolOperationDescriptor<TParams>,
  params: TParams,
): ErasedToolParams {
  return descriptor.toWireParams
    ? descriptor.toWireParams(params)
    : (params as ErasedToolParams);
}

/**
 * Reconstruct UI parameters from wire step parameters using fromWireParams,
 * or return the parameters as-is if no conversion is defined.
 */
export function fromWireParams<TParams>(
  descriptor: ToolOperationDescriptor<TParams>,
  wireParams: ErasedToolParams,
): TParams {
  return descriptor.fromWireParams
    ? descriptor.fromWireParams(wireParams)
    : (wireParams as unknown as TParams);
}
