import { useState, useCallback } from "react";
import type { AutomationConfig } from "@app/types/automation";

/**
 * Shape of an automation that can be imported via JSON file upload.
 * Matches the serialisation format written by the export flow.
 */
export interface ImportableAutomation {
  name: string;
  description?: string;
  icon?: string;
  operations: AutomationConfig["operations"];
  version?: string;
}

export interface SavedAutomationsHook {
  automations: AutomationConfig[];
  isLoading: boolean;
  save: (
    config: Omit<AutomationConfig, "id" | "createdAt" | "updatedAt">,
  ) => Promise<AutomationConfig>;
  update: (id: string, config: Partial<AutomationConfig>) => Promise<void>;
  remove: (id: string) => Promise<void>;
  importAutomation: (
    importable: ImportableAutomation,
  ) => Promise<AutomationConfig>;
  reload: () => Promise<void>;
}

/**
 * Manages the user's saved automation workflows using the automation storage service.
 */
export function useSavedAutomations(): SavedAutomationsHook {
  const [automations, setAutomations] = useState<AutomationConfig[]>([]);
  const [isLoading, setIsLoading] = useState(false);

  const now = () => new Date().toISOString();

  const reload = useCallback(async () => {
    setIsLoading(true);
    try {
      const { automationStorage } =
        await import("@app/services/automationStorage");
      const loaded = await automationStorage.getAll();
      setAutomations(loaded);
    } catch {
      // storage not available — remain empty
    } finally {
      setIsLoading(false);
    }
  }, []);

  const save = useCallback(
    async (
      config: Omit<AutomationConfig, "id" | "createdAt" | "updatedAt">,
    ) => {
      const full: AutomationConfig = {
        ...config,
        id: crypto.randomUUID(),
        createdAt: now(),
        updatedAt: now(),
      };
      try {
        const { automationStorage } =
          await import("@app/services/automationStorage");
        await automationStorage.save(full);
      } catch {
        /* ignore */
      }
      setAutomations((prev) => [...prev, full]);
      return full;
    },
    [],
  );

  const update = useCallback(
    async (id: string, patch: Partial<AutomationConfig>) => {
      try {
        const { automationStorage } =
          await import("@app/services/automationStorage");
        await automationStorage.update(id, { ...patch, updatedAt: now() });
      } catch {
        /* ignore */
      }
      setAutomations((prev) =>
        prev.map((a) =>
          a.id === id ? { ...a, ...patch, updatedAt: now() } : a,
        ),
      );
    },
    [],
  );

  const remove = useCallback(async (id: string) => {
    try {
      const { automationStorage } =
        await import("@app/services/automationStorage");
      await automationStorage.remove(id);
    } catch {
      /* ignore */
    }
    setAutomations((prev) => prev.filter((a) => a.id !== id));
  }, []);

  const importAutomation = useCallback(
    async (importable: ImportableAutomation) => {
      return save({
        name: importable.name,
        description: importable.description ?? "",
        icon: importable.icon ?? "",
        operations: importable.operations,
      });
    },
    [save],
  );

  return {
    automations,
    isLoading,
    save,
    update,
    remove,
    importAutomation,
    reload,
  };
}
