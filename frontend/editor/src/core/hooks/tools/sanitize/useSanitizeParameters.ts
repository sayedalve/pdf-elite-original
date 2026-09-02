import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface SanitizeParameters extends BaseParameters {}

export const defaultParameters: SanitizeParameters = {};

export type SanitizeParametersHook = BaseParametersHook<SanitizeParameters>;

export const useSanitizeParameters = (): SanitizeParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "sanitize-pdf" });
