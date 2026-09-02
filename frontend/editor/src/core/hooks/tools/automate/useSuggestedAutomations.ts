import { useMemo } from "react";
import type { SuggestedAutomation } from "@app/types/automation";

/**
 * Returns a list of pre-built suggested automations shown to the user when
 * they open the Automate tool without any existing workflows.
 *
 * The list is currently empty in the core layer; the proprietary layer
 * can shadow this hook to provide curated suggestions.
 */
export function useSuggestedAutomations(): SuggestedAutomation[] {
  return useMemo(() => [], []);
}
