/* eslint-disable */
import { useCallback, useEffect, useState } from "react";

type DocState = {
  page: number;
  zoom: number;
  scrollTop?: number;
};

const KEY = (id: string) => `pdf-elite:doc-state:${id}`;

export function useDocumentState(docId: string) {
  const [state, setState] = useState<DocState>(() => {
    try {
      const raw = localStorage.getItem(KEY(docId));
      if (raw) return JSON.parse(raw);
    } catch {}
    return { page: 1, zoom: 1.2 };
  });

  const save = useCallback(
    (next: Partial<DocState>) => {
      setState((prev) => {
        const merged = { ...prev, ...next };
        try {
          localStorage.setItem(KEY(docId), JSON.stringify(merged));
        } catch {}
        return merged;
      });
    },
    [docId],
  );

  useEffect(() => {
    try {
      const raw = localStorage.getItem(KEY(docId));
      if (raw) setState(JSON.parse(raw));
    } catch {}
  }, [docId]);

  return {
    state,
    save,
    setPage: (p: number) => save({ page: p }),
    setZoom: (z: number) => save({ zoom: z }),
  };
}
