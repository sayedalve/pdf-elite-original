import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface ReplaceColorParameters extends BaseParameters {
  replaceAndInvertOption?: string;
  highContrastColorCombination?: string;
  textColor?: string;
  backGroundColor?: string;
}

export const defaultParameters: ReplaceColorParameters = {};

export type ReplaceColorParametersHook =
  BaseParametersHook<ReplaceColorParameters>;

export const useReplaceColorParameters = (): ReplaceColorParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "replace-invert-pdf" });
