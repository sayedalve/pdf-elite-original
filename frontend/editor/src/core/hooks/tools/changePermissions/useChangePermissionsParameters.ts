import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface ChangePermissionsParameters extends BaseParameters {}

export const defaultParameters: ChangePermissionsParameters = {};

export type ChangePermissionsParametersHook =
  BaseParametersHook<ChangePermissionsParameters>;

export const useChangePermissionsParameters =
  (): ChangePermissionsParametersHook =>
    useBaseParameters({ defaultParameters, endpointName: "add-password" });
