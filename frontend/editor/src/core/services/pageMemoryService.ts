/**
 * pageMemoryService — persists and restores the last-viewed page for each document.
 *
 * Uses localStorage with a compact key derived from the document's cache key.
 * Storage is capped to avoid unbounded growth.
 */

const STORAGE_KEY = "PDFElitepdf_page_memory";
const MAX_ENTRIES = 200;

interface PageMemoryStore {
  [cacheKey: string]: {
    page: number; // last page number (1-based)
    zoom?: number; // zoom percentage or special mode enum
  };
}

function loadStore(): PageMemoryStore {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw) return JSON.parse(raw) as PageMemoryStore;
  } catch {
    // Ignore parse errors
  }
  return {};
}

function saveStore(store: PageMemoryStore): void {
  try {
    // Evict oldest entries if over cap (keep the most-recently-set ones)
    const keys = Object.keys(store);
    if (keys.length > MAX_ENTRIES) {
      const evict = keys.slice(0, keys.length - MAX_ENTRIES);
      for (const k of evict) {
        delete store[k];
      }
    }
    localStorage.setItem(STORAGE_KEY, JSON.stringify(store));
  } catch {
    // Ignore write errors (private browsing, quota exceeded, etc.)
  }
}

class PageMemoryService {
  /**
   * Persist the last-viewed page for a document.
   * Only saves pages > 1 to avoid redundant writes.
   */
  savePage(cacheKey: string, page: number): void {
    if (!cacheKey || page < 1) return;
    const store = loadStore();
    const existing = store[cacheKey] || { page: 1 };

    if (page === 1 && !existing.zoom) {
      delete store[cacheKey];
    } else {
      store[cacheKey] = { ...existing, page };
    }
    saveStore(store);
  }

  /**
   * Persist the last-used zoom level for a document.
   */
  saveZoom(cacheKey: string, zoom: number): void {
    if (!cacheKey) return;
    const store = loadStore();
    const existing = store[cacheKey] || { page: 1 };
    store[cacheKey] = { ...existing, zoom };
    saveStore(store);
  }

  /**
   * Retrieve the last-viewed page for a document, or 1 if not found.
   */
  getPage(cacheKey: string): number {
    if (!cacheKey) return 1;
    const store = loadStore();
    return store[cacheKey]?.page ?? 1;
  }

  /**
   * Retrieve the last-used zoom level for a document, or undefined if not found.
   */
  getZoom(cacheKey: string): number | undefined {
    if (!cacheKey) return undefined;
    const store = loadStore();
    return store[cacheKey]?.zoom;
  }

  /**
   * Clear memory for a specific document (e.g. after it's deleted).
   */
  clearPage(cacheKey: string): void {
    if (!cacheKey) return;
    const store = loadStore();
    delete store[cacheKey];
    saveStore(store);
  }
}

export const pageMemoryService = new PageMemoryService();
