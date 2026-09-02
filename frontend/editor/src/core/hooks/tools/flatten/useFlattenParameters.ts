import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface FlattenParameters extends BaseParameters {
  flattenOnlyForms?: boolean;
  renderDpi?: number;
}

export const defaultParameters: FlattenParameters = {};

export type FlattenParametersHook = BaseParametersHook<FlattenParameters>;

export const useFlattenParameters = (): FlattenParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "flatten" });
