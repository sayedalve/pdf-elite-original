import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export type SignatureMode = "draw" | "type" | "upload" | "saved";
export type SignaturePlacement = "click" | "drag";

export interface SignParameters extends BaseParameters {
  mode: SignatureMode;
  placement: SignaturePlacement;
  x: number;
  y: number;
  width: number;
  height: number;
  pageIndex: number;
  signatureDataUrl?: string;
  signerName?: string;
  /** Font family for typed signatures. */
  fontFamily?: string;
  /** Font size for typed signatures. */
  fontSize?: number;
  /** Text color for typed signatures. */
  textColor?: string;
  /** The type of signature: 'draw' | 'type' | 'upload' | 'saved'. */
  signatureType?: string;
}

export const DEFAULT_PARAMETERS: SignParameters = {
  mode: "draw",
  placement: "click",
  x: 0.1,
  y: 0.8,
  width: 0.3,
  height: 0.1,
  pageIndex: 0,
  fontFamily: "Helvetica",
  fontSize: 16,
  textColor: "#000000",
  signatureType: "draw",
};

/** @deprecated Use DEFAULT_PARAMETERS instead. */
export const defaultParameters: SignParameters = DEFAULT_PARAMETERS;

export type SignParametersHook = BaseParametersHook<SignParameters>;

export const useSignParameters = (): SignParametersHook =>
  useBaseParameters({
    defaultParameters: DEFAULT_PARAMETERS,
    endpointName: "cert-sign",
  });
