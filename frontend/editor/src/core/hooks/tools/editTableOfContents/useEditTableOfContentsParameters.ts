import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface EditTableOfContentsParameters extends BaseParameters {}

export const defaultParameters: EditTableOfContentsParameters = {};

export type EditTableOfContentsParametersHook =
  BaseParametersHook<EditTableOfContentsParameters>;

export const useEditTableOfContentsParameters =
  (): EditTableOfContentsParametersHook =>
    useBaseParameters({
      defaultParameters,
      endpointName: "edit-table-of-contents",
    });
