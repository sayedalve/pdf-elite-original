import { useState, useCallback } from "react";

// Minimum shape every tool parameters hook must satisfy.
export interface BaseParametersHook<TParams> {
  parameters: TParams;
  updateParameter: <K extends keyof TParams>(key: K, value: TParams[K]) => void;
  setParameters: (params: TParams) => void;
  resetParameters: () => void;
  validateParameters: () => boolean;
}

export interface UseBaseParametersOptions<TParams> {
  defaultParameters: TParams;
  /** Human-readable endpoint name used for analytics / debugging only. */
  endpointName?: string;
  /** Returns true when the current parameters are valid enough to execute. */
  validateFn?: (params: TParams) => boolean;
}

/**
 * Generic hook that manages the mutable UI state for a tool's parameters.
 * Each tool hooks extends this by spreading the base hook and adding
 * domain-specific helpers.
 */
export function useBaseParameters<TParams>(
  options: UseBaseParametersOptions<TParams>,
): BaseParametersHook<TParams> {
  const { defaultParameters, validateFn } = options;

  const [parameters, setParameters] = useState<TParams>(() => ({
    ...defaultParameters,
  }));

  const updateParameter = useCallback(
    <K extends keyof TParams>(key: K, value: TParams[K]) => {
      setParameters((prev) => ({ ...prev, [key]: value }));
    },
    [],
  );

  const resetParameters = useCallback(() => {
    setParameters({ ...defaultParameters });
  }, [defaultParameters]);

  const validateParameters = useCallback(() => {
    if (!validateFn) return true;
    return validateFn(parameters);
  }, [validateFn, parameters]);

  return {
    parameters,
    updateParameter,
    setParameters,
    resetParameters,
    validateParameters,
  };
}
