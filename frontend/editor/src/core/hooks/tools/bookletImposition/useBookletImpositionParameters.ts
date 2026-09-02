import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface BookletImpositionParameters extends BaseParameters {
  doubleSided?: boolean;
  duplexPass?: string;
  spineLocation?: string;
  addBorder?: boolean;
  addGutter?: boolean;
  gutterSize?: number;
  flipOnShortEdge?: boolean;
}

export const defaultParameters: BookletImpositionParameters = {};

export type BookletImpositionParametersHook =
  BaseParametersHook<BookletImpositionParameters>;

export const useBookletImpositionParameters =
  (): BookletImpositionParametersHook =>
    useBaseParameters({
      defaultParameters,
      endpointName: "booklet-imposition",
    });
