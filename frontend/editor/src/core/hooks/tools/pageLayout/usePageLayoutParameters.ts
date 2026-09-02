import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export type PageLayoutMode = "2x1" | "2x2" | "3x2" | "3x3" | "1x2" | "booklet";

export interface PageLayoutParameters extends BaseParameters {
  mode?: string;
  rows?: number;
  cols?: number;
  pagesPerSheet: number;
  pageLayoutMode: PageLayoutMode;
  automaticRotation: boolean;
}

export const defaultParameters: PageLayoutParameters = {
  pagesPerSheet: 2,
  pageLayoutMode: "2x1",
  automaticRotation: true,
};

export type PageLayoutParametersHook = BaseParametersHook<PageLayoutParameters>;

export const usePageLayoutParameters = (): PageLayoutParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "pdf-to-page-layout" });
