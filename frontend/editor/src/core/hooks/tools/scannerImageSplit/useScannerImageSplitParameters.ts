import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface ScannerImageSplitParameters extends BaseParameters {
  angle_threshold?: number;
  tolerance?: number;
  min_area?: number;
  min_contour_area?: number;
  border_size?: number;
}

export const defaultParameters: ScannerImageSplitParameters = {};

export type ScannerImageSplitParametersHook =
  BaseParametersHook<ScannerImageSplitParameters>;

export const useScannerImageSplitParameters =
  (): ScannerImageSplitParametersHook =>
    useBaseParameters({
      defaultParameters,
      endpointName: "extract-image-scans",
    });
