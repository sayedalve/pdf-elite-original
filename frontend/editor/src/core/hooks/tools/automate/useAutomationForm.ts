/* eslint-disable */
import { useState, useCallback } from "react";
import type {
  AutomationConfig,
  AutomationOperation,
  AutomationTool,
} from "@app/types/automation";

export interface AutomationFormState {
  name: string;
  description: string;
  icon: string;
  operations: AutomationTool[];
}

export interface AutomationFormActions {
  setName: (name: string) => void;
  setDescription: (description: string) => void;
  setIcon: (icon: string) => void;
  addOperation: (tool: AutomationTool) => void;
  removeOperation: (index: number) => void;
  moveOperation: (from: number, to: number) => void;
  updateOperation: (index: number, updates: Partial<AutomationTool>) => void;
  reset: (initial?: Partial<AutomationFormState>) => void;
  buildConfig: () => Omit<AutomationConfig, "id" | "createdAt" | "updatedAt">;
  isValid: boolean;
}

export type AutomationFormHook = AutomationFormState & AutomationFormActions;

const defaultState: AutomationFormState = {
  name: "",
  description: "",
  icon: "",
  operations: [],
};

/**
 * Manages form state for creating and editing automation workflows.
 */
export function useAutomationForm(
  initial?: Partial<AutomationFormState>,
): AutomationFormHook {
  const [name, setNameState] = useState(initial?.name ?? defaultState.name);
  const [description, setDescriptionState] = useState(
    initial?.description ?? defaultState.description,
  );
  const [icon, setIconState] = useState(initial?.icon ?? defaultState.icon);
  const [operations, setOperations] = useState<AutomationTool[]>(
    initial?.operations ?? defaultState.operations,
  );

  const setName = useCallback((v: string) => setNameState(v), []);
  const setDescription = useCallback((v: string) => setDescriptionState(v), []);
  const setIcon = useCallback((v: string) => setIconState(v), []);

  const addOperation = useCallback((tool: AutomationTool) => {
    setOperations((prev) => [...prev, tool]);
  }, []);

  const removeOperation = useCallback((index: number) => {
    setOperations((prev) => prev.filter((_, i) => i !== index));
  }, []);

  const moveOperation = useCallback((from: number, to: number) => {
    setOperations((prev) => {
      const next = [...prev];
      const [item] = next.splice(from, 1);
      next.splice(to, 0, item);
      return next;
    });
  }, []);

  const updateOperation = useCallback(
    (index: number, updates: Partial<AutomationTool>) => {
      setOperations((prev) =>
        prev.map((op, i) => (i === index ? { ...op, ...updates } : op)),
      );
    },
    [],
  );

  const reset = useCallback((resetTo?: Partial<AutomationFormState>) => {
    setNameState(resetTo?.name ?? defaultState.name);
    setDescriptionState(resetTo?.description ?? defaultState.description);
    setIconState(resetTo?.icon ?? defaultState.icon);
    setOperations(resetTo?.operations ?? defaultState.operations);
  }, []);

  const buildConfig = useCallback(
    (): Omit<AutomationConfig, "id" | "createdAt" | "updatedAt"> => ({
      name,
      description,
      icon,
      operations: operations.map((op) => ({
        operation: op.operation,
        parameters: op.parameters || {},
      })),
    }),
    [name, description, icon, operations],
  );

  const isValid = name.trim().length > 0 && operations.length > 0;

  return {
    name,
    description,
    icon,
    operations,
    setName,
    setDescription,
    setIcon,
    addOperation,
    removeOperation,
    moveOperation,
    updateOperation,
    reset,
    buildConfig,
    isValid,
  };
}
