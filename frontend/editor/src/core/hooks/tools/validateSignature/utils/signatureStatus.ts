/**
 * Signature validation status values returned by the backend.
 */
export type SignatureStatus =
  | "VALID"
  | "INVALID"
  | "UNKNOWN"
  | "EXPIRED"
  | "REVOKED"
  | "NOT_YET_VALID"
  | "SELF_SIGNED";

/** Map a raw backend status string to a canonical SignatureStatus. */
export function normalizeSignatureStatus(
  raw: string | undefined,
): SignatureStatus {
  const upper = (raw ?? "").toUpperCase();
  const valid: SignatureStatus[] = [
    "VALID",
    "INVALID",
    "UNKNOWN",
    "EXPIRED",
    "REVOKED",
    "NOT_YET_VALID",
    "SELF_SIGNED",
  ];
  return valid.includes(upper as SignatureStatus)
    ? (upper as SignatureStatus)
    : "UNKNOWN";
}

/** Returns true when the signature status indicates overall validity. */
export function isValidStatus(status: SignatureStatus): boolean {
  return status === "VALID";
}

/** Returns a CSS colour token name for the given signature status. */
export function statusColour(
  status: SignatureStatus,
): "green" | "red" | "yellow" | "gray" {
  switch (status) {
    case "VALID":
      return "green";
    case "INVALID":
    case "REVOKED":
    case "EXPIRED":
      return "red";
    case "NOT_YET_VALID":
    case "SELF_SIGNED":
      return "yellow";
    default:
      return "gray";
  }
}
