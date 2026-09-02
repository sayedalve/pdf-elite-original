import { PDFElite_SAAS_BACKEND_API_URL } from "@app/constants/connection";
import type { PolicyExecutionTarget } from "@app/services/policyPipeline";

/**
 * Desktop: a policy run's outputs live on the backend that executed it.
 */
export function getPolicyOutputBaseUrl(target: PolicyExecutionTarget): string {
  if (target === "saas") {
    return (PDFElite_SAAS_BACKEND_API_URL ?? "").replace(/\/$/, "");
  }
  return "";
}
