import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface AdjustContrastParameters extends BaseParameters {
  red?: number;
  green?: number;
  blue?: number;
  contrast?: number;
  brightness?: number;
  saturation?: number;
}

export const defaultParameters: AdjustContrastParameters = {};

export type AdjustContrastParametersHook =
  BaseParametersHook<AdjustContrastParameters>;

export const useAdjustContrastParameters = (): AdjustContrastParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "scanner-effect" });
