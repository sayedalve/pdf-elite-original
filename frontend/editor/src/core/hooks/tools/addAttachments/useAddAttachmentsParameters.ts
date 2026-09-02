import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";

export interface AddAttachmentsParameters extends BaseParameters {
  attachments?: File[];
  convertToPdfA3b?: boolean;
}

export const defaultParameters: AddAttachmentsParameters = {};

export type AddAttachmentsParametersHook =
  BaseParametersHook<AddAttachmentsParameters>;

export const useAddAttachmentsParameters = (): AddAttachmentsParametersHook =>
  useBaseParameters({ defaultParameters, endpointName: "add-attachments" });
