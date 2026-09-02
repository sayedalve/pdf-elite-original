import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface OCRParameters extends BaseParameters {
  ocrType?: string;
  languages?: string[];
}

export const defaultParameters: OCRParameters = {};

export type OCRParametersHook = BaseParametersHook<OCRParameters>;

export const useOCRParameters = (): OCRParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "ocr-pdf" });
