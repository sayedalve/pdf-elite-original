import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export enum Orientation {
  PORTRAIT = "PORTRAIT",
  LANDSCAPE = "LANDSCAPE",
}

export enum PageSize {
  KEEP = "KEEP",
  A0 = "A0",
  A1 = "A1",
  A2 = "A2",
  A3 = "A3",
  A4 = "A4",
  A5 = "A5",
  A6 = "A6",
  LETTER = "LETTER",
  LEGAL = "LEGAL",
}

export type ScaleMode = "fit" | "fill" | "stretch" | "custom";

export interface AdjustPageScaleParameters extends BaseParameters {
  targetPageSize: PageSize;
  orientation: Orientation;
  scaleMode: ScaleMode;
  /** Custom width in points (only used when scaleMode === "custom") */
  customWidth?: number;
  /** Custom height in points (only used when scaleMode === "custom") */
  customHeight?: number;
  /** Margin in points to apply around content */
  margin?: number;
  scaleFactor?: number;
}

export const defaultParameters: AdjustPageScaleParameters = {
  targetPageSize: PageSize.A4,
  orientation: Orientation.PORTRAIT,
  scaleMode: "fit",
};

export type AdjustPageScaleParametersHook =
  BaseParametersHook<AdjustPageScaleParameters>;

export const useAdjustPageScaleParameters = (): AdjustPageScaleParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "scale-pages" });
