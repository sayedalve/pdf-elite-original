import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface RemoveAnnotationsParameters extends BaseParameters {}

export const defaultParameters: RemoveAnnotationsParameters = {};

export type RemoveAnnotationsParametersHook =
  BaseParametersHook<RemoveAnnotationsParameters>;

export const useRemoveAnnotationsParameters =
  (): RemoveAnnotationsParametersHook =>
    useBaseParameters({ defaultParameters, endpointName: "flatten" });
