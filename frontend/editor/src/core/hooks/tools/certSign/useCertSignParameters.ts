import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface CertSignParameters extends BaseParameters {
  showSignature?: boolean;
  reason?: string;
  location?: string;
  name?: string;
  pageNumber?: number;
  showLogo?: boolean;
  pkcs11LibraryPath?: string;
  password?: string;
  pkcs11Slot?: number;
  alias?: string;
  certType?: string;
  signMode?: string;
  privateKeyFile?: File | null;
  certFile?: File | null;
  p12File?: File | null;
  jksFile?: File | null;
}

export const defaultParameters: CertSignParameters = {};

export type CertSignParametersHook = BaseParametersHook<CertSignParameters>;

export const useCertSignParameters = (): CertSignParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "cert-sign" });
