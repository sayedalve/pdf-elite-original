import type { BaseParameters } from "@app/types/parameters";

export interface RemovePasswordParameters extends BaseParameters {
  /** The password used to unlock the encrypted PDF. */
  password: string;
}

export const defaultParameters: RemovePasswordParameters = {
  password: "",
};
