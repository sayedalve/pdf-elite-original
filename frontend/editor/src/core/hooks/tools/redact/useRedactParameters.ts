import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export type RedactMode = "automatic" | "manual" | "text" | "regex" | "color";

export interface RedactParameters extends BaseParameters {
  mode: RedactMode;
  wordsToRedact?: string[];
  searchText?: string;
  useRegex?: boolean;
  useColor?: boolean;
  wholeWordOnly?: boolean;
  caseSensitive?: boolean;
  redactColor?: string;
  customPadding?: number;
  convertPDFToImage?: boolean;
}

export const defaultParameters: RedactParameters = {
  mode: "automatic",
  wordsToRedact: [],
  searchText: "",
  useRegex: false,
  wholeWordOnly: false,
  caseSensitive: false,
  convertPDFToImage: false,
};

export type RedactParametersHook = BaseParametersHook<RedactParameters>;

export const useRedactParameters = (): RedactParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "redact" });
