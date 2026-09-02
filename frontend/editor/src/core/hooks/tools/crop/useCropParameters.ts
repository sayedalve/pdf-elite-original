/* eslint-disable */
import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface CropParameters extends BaseParameters {
  cropArea?: string | any;
}

export const defaultParameters: CropParameters = {};

export type CropParametersHook = BaseParametersHook<CropParameters>;

export const useCropParameters = (): CropParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "crop" });
