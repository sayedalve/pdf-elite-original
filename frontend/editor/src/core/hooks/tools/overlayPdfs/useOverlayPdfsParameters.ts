import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export type OverlayMode =
  | "FixedRepeatOverlay"
  | "SequentialRepeatOverlay"
  | "OneTimeOverlay"
  | "CycleOverlay";

export interface OverlayPdfsParameters extends BaseParameters {
  overlayMode: OverlayMode;
  /** Whether the overlay is placed on top (true) or behind (false) the base PDF. */
  overlayOnTop: boolean;
  /** Page rotation to apply to overlay pages in degrees. */
  overlayRotation?: number;
  overlayPosition?: 0 | 1;
  overlayFiles?: File[];
  counts?: number[];
}

export const defaultParameters: OverlayPdfsParameters = {
  overlayMode: "FixedRepeatOverlay",
  overlayOnTop: true,
};

export type OverlayPdfsParametersHook =
  BaseParametersHook<OverlayPdfsParameters>;

export const useOverlayPdfsParameters = (): OverlayPdfsParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "overlay-pdfs" });
