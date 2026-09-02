import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface RemoveBlanksParameters extends BaseParameters {
  threshold?: number;
  whitePercent?: number;
  includeBlankPages?: boolean;
}

export const defaultParameters: RemoveBlanksParameters = {};

export type RemoveBlanksParametersHook =
  BaseParametersHook<RemoveBlanksParameters>;

export const useRemoveBlanksParameters = (): RemoveBlanksParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "remove-blanks" });
