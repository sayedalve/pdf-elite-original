import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface CompressParameters extends BaseParameters {
  compressionMethod?: string;
  compressionLevel?: number;
  fileSizeValue?: string;
  fileSizeUnit?: "KB" | "MB";
  grayscale?: boolean;
  linearize?: boolean;
  lineArt?: boolean;
  lineArtThreshold?: number;
  lineArtEdgeLevel?: 1 | 2 | 3;
}

export const defaultParameters: CompressParameters = {
  compressionMethod: "quality",
  compressionLevel: 5,
  fileSizeUnit: "MB",
  lineArtThreshold: 50,
  lineArtEdgeLevel: 2,
};

export type CompressParametersHook = BaseParametersHook<CompressParameters>;

export const useCompressParameters = (): CompressParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "compress-pdf" });
