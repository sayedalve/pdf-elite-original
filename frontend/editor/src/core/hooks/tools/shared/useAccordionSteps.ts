import { useState, useCallback } from "react";

/**
 * Manages the open/collapsed state for multi-step accordion tool layouts.
 * Returns helpers that let the tool component track which step the user is
 * currently on and collapse previous steps when they proceed.
 */
export interface AccordionStepsHook {
  /** The index (0-based) of the currently active / expanded step. */
  activeStep: number;
  /** Open the step at the given index. */
  openStep: (index: number) => void;
  /** Mark the given step as complete and advance to the next one. */
  completeStep: (index: number) => void;
  /** Reset all steps back to the initial (step 0) state. */
  resetSteps: () => void;
  /** Returns true when the step at the given index is currently expanded. */
  isStepOpen: (index: number) => boolean;
  /** Returns true when the step at the given index has been completed. */
  isStepComplete: (index: number) => boolean;
}

export function useAccordionSteps(totalSteps: number): AccordionStepsHook {
  const [activeStep, setActiveStep] = useState(0);
  const [completedSteps, setCompletedSteps] = useState<Set<number>>(new Set());

  const openStep = useCallback((index: number) => {
    setActiveStep(index);
  }, []);

  const completeStep = useCallback(
    (index: number) => {
      setCompletedSteps((prev) => {
        const next = new Set(prev);
        next.add(index);
        return next;
      });
      // Advance to the next step if not already past the end
      setActiveStep((prev) => Math.min(prev + 1, totalSteps - 1));
    },
    [totalSteps],
  );

  const resetSteps = useCallback(() => {
    setActiveStep(0);
    setCompletedSteps(new Set());
  }, []);

  const isStepOpen = useCallback(
    (index: number) => index === activeStep,
    [activeStep],
  );

  const isStepComplete = useCallback(
    (index: number) => completedSteps.has(index),
    [completedSteps],
  );

  return {
    activeStep,
    openStep,
    completeStep,
    resetSteps,
    isStepOpen,
    isStepComplete,
  };
}
