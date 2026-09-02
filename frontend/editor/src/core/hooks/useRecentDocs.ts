/* eslint-disable */
import { useState, useCallback, useEffect } from "react";
import type { RecentDoc } from "../workbench/home/RecentFiles";
import { useIndexedDB } from "@app/contexts/IndexedDBContext";
import { useFileActions } from "@app/contexts/FileContext";

const STORAGE_KEY = "pdf-elite:recent-docs";
const MAX_RECENT = 50;

export function useRecentDocs() {
  const [docs, setDocs] = useState<RecentDoc[]>([]);
  const { loadLeafMetadata, deleteFile } = useIndexedDB();
  const fileActions = useFileActions();

  // Load from IndexedDB on mount
  useEffect(() => {
    loadLeafMetadata().then((stubs) => {
      // Map PDFEliteFileStub to RecentDoc
      // Deduplicate by name, keeping only the most recent version
      const uniqueDocs = new Map<string, RecentDoc>();
      for (const stub of stubs) {
        const doc: RecentDoc = {
          id: stub.id as string,
          name: stub.name,
          path: stub.name, // Use name as path since web doesn't have real paths
          size: (stub.size / (1024 * 1024)).toFixed(2) + " MB",
          sizeBytes: stub.size,
          modified: new Date(stub.lastModified).toLocaleDateString(),
          modifiedTs: stub.lastModified,
          pages: stub.processedFile?.totalPages || 0,
          starred: false,
        };
        // If it exists, keep the one with a newer timestamp
        if (uniqueDocs.has(doc.name)) {
          const existing = uniqueDocs.get(doc.name)!;
          if (doc.modifiedTs > existing.modifiedTs) {
            uniqueDocs.set(doc.name, doc);
          }
        } else {
          uniqueDocs.set(doc.name, doc);
        }
      }

      const recentDocs = Array.from(uniqueDocs.values()).sort(
        (a, b) => b.modifiedTs - a.modifiedTs,
      ); // Sort by most recent
      setDocs(recentDocs);
    });
  }, [loadLeafMetadata]);

  // Keep addOrUpdate for compatibility, though actual files are added via FileContext
  const addOrUpdate = useCallback(
    (file: { name: string; path: string; sizeBytes: number; size: string }) => {
      // No-op since FileContext automatically persists to IndexedDB
    },
    [],
  );

  const remove = useCallback(
    async (id: string) => {
      await deleteFile(id as any);
      setDocs((prev) => prev.filter((d) => d.id !== id));
    },
    [deleteFile],
  );

  const toggleStar = useCallback((id: string) => {
    setDocs((prev) =>
      prev.map((d) => (d.id === id ? { ...d, starred: !d.starred } : d)),
    );
  }, []);

  const updateLastPosition = useCallback(
    (id: string, page: number, zoom: number) => {
      setDocs((prev) =>
        prev.map((d) =>
          d.id === id ? { ...d, lastPage: page, lastZoom: zoom } : d,
        ),
      );
    },
    [],
  );

  return { docs, addOrUpdate, remove, toggleStar, updateLastPosition };
}
