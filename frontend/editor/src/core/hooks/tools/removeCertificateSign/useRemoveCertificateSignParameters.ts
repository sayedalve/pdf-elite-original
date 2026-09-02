import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface RemoveCertificateSignParameters extends BaseParameters {}

export const defaultParameters: RemoveCertificateSignParameters = {};

export type RemoveCertificateSignParametersHook =
  BaseParametersHook<RemoveCertificateSignParameters>;

export const useRemoveCertificateSignParameters =
  (): RemoveCertificateSignParametersHook =>
    useBaseParameters({
      defaultParameters,
      endpointName: "remove-certificate-sign",
    });
