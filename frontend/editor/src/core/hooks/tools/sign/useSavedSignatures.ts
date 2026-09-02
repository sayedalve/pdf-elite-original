import { useState, useCallback } from "react";
import type { StorageType } from "@app/services/signatureStorageService";
import type {
  SavedSignature,
  SavedSignaturePayload,
  SavedSignatureType,
  SignatureScope,
} from "@app/types/signature";

export type { SavedSignature, SavedSignaturePayload, SavedSignatureType };

// ─── Types ────────────────────────────────────────────────────────────────────

/** The result returned after adding a new signature. */
export interface AddSignatureResult {
  signature?: SavedSignature;
  isAtCapacity: boolean;
  success: boolean;
  reason?: string;
}

// ─── Hook ────────────────────────────────────────────────────────────────────

export interface SavedSignaturesHook {
  signatures: SavedSignature[];
  isLoading: boolean;
  isAtCapacity: boolean;
  maxLimit: number;
  storageType: StorageType | null;
  addSignature: (
    payload: SavedSignaturePayload,
    label?: string,
    scope?: SignatureScope,
  ) => Promise<AddSignatureResult>;
  deleteSignature: (signature: SavedSignature) => Promise<void>;
  updateSignatureLabel: (id: string, label: string) => Promise<void>;
  reload: () => Promise<void>;
  byTypeCounts: Record<SavedSignatureType, number>;
  isAdmin: boolean;
}

const MAX_SIGNATURES = 10;

/**
 * Manages the user's saved signatures using the signature storage service.
 */
export function useSavedSignatures(): SavedSignaturesHook {
  const [signatures, setSignatures] = useState<SavedSignature[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [storageType, setStorageType] = useState<StorageType | null>(null);

  const reload = useCallback(async () => {
    setIsLoading(true);
    try {
      const { signatureStorageService } =
        await import("@app/services/signatureStorageService");
      const loaded = await signatureStorageService.loadSignatures();
      setSignatures(loaded);
      setStorageType(await signatureStorageService.getStorageType());
    } catch {
      // storage unavailable — remain empty
    } finally {
      setIsLoading(false);
    }
  }, []);

  const addSignature = useCallback(
    async (
      payload: SavedSignaturePayload,
      label?: string,
      scope?: SignatureScope,
    ): Promise<AddSignatureResult> => {
      const signature: SavedSignature = {
        ...payload,
        id: crypto.randomUUID(),
        label: label || "Signature",
        scope: scope || "localStorage",
        createdAt: Date.now(),
        updatedAt: Date.now(),
      } as SavedSignature;
      try {
        const { signatureStorageService } =
          await import("@app/services/signatureStorageService");
        await signatureStorageService.saveSignature(signature);
      } catch {
        /* ignore */
      }
      const next = [...signatures, signature];
      setSignatures(next);
      return {
        signature,
        isAtCapacity: next.length >= MAX_SIGNATURES,
        success: true,
      };
    },
    [signatures],
  );

  const updateSignatureLabel = useCallback(
    async (id: string, label: string) => {
      try {
        // Temporary implementation until storage service supports update
      } catch {
        /* ignore */
      }
      setSignatures((prev) =>
        prev.map((s) => (s.id === id ? { ...s, label } : s)),
      );
    },
    [],
  );

  const deleteSignature = useCallback(async (signature: SavedSignature) => {
    try {
      const { signatureStorageService } =
        await import("@app/services/signatureStorageService");
      await signatureStorageService.deleteSignature(signature.id);
    } catch {
      /* ignore */
    }
    setSignatures((prev) => prev.filter((s) => s.id !== signature.id));
  }, []);

  const byTypeCounts = signatures.reduce(
    (acc, sig) => {
      acc[sig.type] = (acc[sig.type] || 0) + 1;
      return acc;
    },
    { canvas: 0, image: 0, text: 0 } as Record<SavedSignatureType, number>,
  );

  return {
    signatures,
    isLoading,
    isAtCapacity: signatures.length >= MAX_SIGNATURES,
    maxLimit: MAX_SIGNATURES,
    storageType,
    addSignature,
    deleteSignature,
    updateSignatureLabel,
    reload,
    byTypeCounts,
    isAdmin: false,
  };
}
