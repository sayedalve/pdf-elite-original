import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface AddWatermarkParameters extends BaseParameters {
  convertPDFToImage?: boolean;
  watermarkType?: "text" | "image";
  watermarkImage?: File;
  customColor?: string;
  alphabet?: string;
  watermarkText?: string;
  fontSize?: number;
  rotation?: number;
  opacity?: number;
  widthSpacer?: number;
  heightSpacer?: number;
}

export const defaultParameters: AddWatermarkParameters = {
  convertPDFToImage: false,
  watermarkType: "text",
};

export type AddWatermarkParametersHook =
  BaseParametersHook<AddWatermarkParameters>;

export const useAddWatermarkParameters = (): AddWatermarkParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "add-watermark" });
