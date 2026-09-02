import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface UnlockPdfFormsParameters extends BaseParameters {}

export const defaultParameters: UnlockPdfFormsParameters = {};

export type UnlockPdfFormsParametersHook =
  BaseParametersHook<UnlockPdfFormsParameters>;

export const useUnlockPdfFormsParameters = (): UnlockPdfFormsParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "unlock-pdf-forms" });
