import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface AddPasswordParameters extends BaseParameters {
  password?: string;
  ownerPassword?: string;
  keyLength?: number;
}

export const defaultParameters: AddPasswordParameters = {
  password: "",
  ownerPassword: "",
  keyLength: 128,
};

export type AddPasswordParametersHook =
  BaseParametersHook<AddPasswordParameters>;

export const useAddPasswordParameters = (): AddPasswordParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "add-password" });
