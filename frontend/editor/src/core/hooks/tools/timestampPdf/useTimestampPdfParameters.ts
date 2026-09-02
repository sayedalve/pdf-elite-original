import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface TsaPreset {
  label: string;
  url: string;
}

/** Default TSA server presets shown when no backend config is present. */
export const FALLBACK_TSA_PRESETS: TsaPreset[] = [
  { label: "Freetsa (free)", url: "https://freetsa.org/tsr" },
  { label: "DigiCert (free)", url: "http://timestamp.digicert.com" },
  { label: "Sectigo", url: "http://timestamp.sectigo.com" },
  {
    label: "GlobalSign",
    url: "http://timestamp.globalsign.com/scripts/timstamp.dll",
  },
  {
    label: "VeriSign",
    url: "http://timestamp.verisign.com/scripts/timstamp.dll",
  },
];

export interface TimestampPdfParameters extends BaseParameters {
  /** TSA server URL used for the timestamp. */
  tsaUrl: string;
  tsaUsername?: string;
  tsaPassword?: string;
}

export const defaultParameters: TimestampPdfParameters = {
  tsaUrl: FALLBACK_TSA_PRESETS[0].url,
};

export type TimestampPdfParametersHook =
  BaseParametersHook<TimestampPdfParameters>;

export const useTimestampPdfParameters = (): TimestampPdfParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "timestamp-pdf" });
