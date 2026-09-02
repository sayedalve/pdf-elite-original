import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface InsertBlankPagesParameters extends BaseParameters {
  /** Where to insert relative to each selected page: "before" or "after". */
  position: number;
  /** How many blank pages to insert at each insertion point. */
  count: number;
  /** Page size for the blank pages (e.g. "A4", "Letter", "Same as page"). */
  pageSize: string;
}

export const defaultParameters: InsertBlankPagesParameters = {
  position: 0,
  count: 1,
  pageSize: "SamePage",
};

export type InsertBlankPagesParametersHook =
  BaseParametersHook<InsertBlankPagesParameters>;

export const useInsertBlankPagesParameters =
  (): InsertBlankPagesParametersHook => {
    return useBaseParameters({
      defaultParameters,
      endpointName: "insert-blank-pages",
    });
  };
