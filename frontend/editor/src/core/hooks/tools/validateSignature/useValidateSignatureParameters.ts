import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface ValidateSignatureParameters extends BaseParameters {}

export const defaultParameters: ValidateSignatureParameters = {};

export type ValidateSignatureParametersHook =
  BaseParametersHook<ValidateSignatureParameters>;

export const useValidateSignatureParameters =
  (): ValidateSignatureParametersHook =>
    useBaseParameters({
      defaultParameters,
      endpointName: "validate-signature",
    });
