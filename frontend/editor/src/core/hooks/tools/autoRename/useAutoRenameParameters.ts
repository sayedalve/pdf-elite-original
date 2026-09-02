import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface AutoRenameParameters extends BaseParameters {}

export const defaultParameters: AutoRenameParameters = {};

export type AutoRenameParametersHook = BaseParametersHook<AutoRenameParameters>;

export const useAutoRenameParameters = (): AutoRenameParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "auto-rename" });
