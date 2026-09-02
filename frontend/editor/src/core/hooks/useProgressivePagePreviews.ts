import { useCallback, useEffect, useRef, useState } from "react";
import { pdfWorkerManager } from "@app/services/pdfWorkerManager";
import type { PDFDocumentProxy } from "pdfjs-dist/legacy/build/pdf.mjs";
import { PagePreview } from "@app/types/compare";

const DISPLAY_SCALE = 1;
const BATCH_SIZE = 10; // Render 10 pages at a time

const getDevicePixelRatio = () =>
  typeof window !== "undefined" ? window.devicePixelRatio : 1;

interface ProgressivePagePreviewsOptions {
  file: File | null;
  enabled: boolean;
  cacheKey: number | null;
  visiblePageRange?: { start: number; end: number }; // 0-based page indices
  scale?: number; // Optional custom scale (defaults to high-res DPR-based)
}

interface ProgressivePagePreviewsState {
  pages: PagePreview[];
  loading: boolean;
  totalPages: number;
}

export const useProgressivePagePreviews = ({
  file,
  enabled,
  cacheKey,
  visiblePageRange,
  scale,
}: ProgressivePagePreviewsOptions) => {
  const [state, setState] = useState<ProgressivePagePreviewsState>({
    pages: [],
    loading: false,
    totalPages: 0,
  });

  const pdfRef = useRef<PDFDocumentProxy | null>(null);

  // Dedup bookkeeping lives in REFS (not state) so the loader can read/update it
  // synchronously without changing function identity on every page. This is the
  // crux of the blank-thumbnail fix: previously loadPageRange depended on
  // loadedPages/loadingPages STATE, so every render recreated it, which re-ran
  // the init effect, whose cleanup destroyed the PDF document mid-render — each
  // in-flight page.render() then threw, the error was swallowed, and the grid
  // stayed blank. With refs, renderPageBatch and loadPageRange are stable and
  // the document is only torn down on a genuine file change or unmount.
  const loadedRef = useRef<Set<number>>(new Set()); // 0-based indices rendered OK
  const loadingRef = useRef<Set<number>>(new Set()); // 0-based indices in flight

  // Two independent abort controllers. The init load is aborted ONLY when the
  // file/enabled/cacheKey changes or the component unmounts; the visible-range
  // load is aborted whenever the visible range changes. Keeping them separate
  // means a visible-range change never kills the initial batch and vice-versa.
  const initAbortRef = useRef<AbortController | null>(null);
  const visibleAbortRef = useRef<AbortController | null>(null);

  // Stable: render a set of 1-based page numbers to data URLs. (This logic was
  // always correct — the earlier bug was the document being destroyed under it.)
  const renderPageBatch = useCallback(
    async (
      pdf: PDFDocumentProxy,
      pageNumbers: number[],
      signal: AbortSignal,
    ): Promise<PagePreview[]> => {
      const previews: PagePreview[] = [];
      const dpr = getDevicePixelRatio();
      const renderScale = scale ?? Math.max(2, Math.min(3, dpr * 2));

      for (const pageNumber of pageNumbers) {
        if (signal.aborted) break;

        try {
          const page = await pdf.getPage(pageNumber);
          const displayViewport = page.getViewport({ scale: DISPLAY_SCALE });
          const renderViewport = page.getViewport({ scale: renderScale });
          const canvas = document.createElement("canvas");
          const context = canvas.getContext("2d");

          canvas.width = Math.round(renderViewport.width);
          canvas.height = Math.round(renderViewport.height);

          if (!context) {
            page.cleanup();
            continue;
          }

          // Fill white first so transparent PDF backgrounds don't serialize as
          // black in the data URL.
          context.fillStyle = "white";
          context.fillRect(0, 0, canvas.width, canvas.height);

          await page.render({
            canvasContext: context,
            viewport: renderViewport,
            canvas,
            background: "rgba(255,255,255,1)",
          }).promise;

          previews.push({
            pageNumber,
            width: Math.round(displayViewport.width),
            height: Math.round(displayViewport.height),
            rotation: (page.rotate || 0) % 360,
            url: canvas.toDataURL(),
          });

          page.cleanup();
          canvas.width = 0;
          canvas.height = 0;
        } catch (error) {
          // A destroyed/aborted document throws here; only log genuine failures.
          if (!signal.aborted) {
            console.error(
              `[progressive-pages] failed to render page ${pageNumber}:`,
              error,
            );
          }
        }
      }

      return previews;
    },
    [],
  );

  // Stable loader. Reads page bounds from the live pdfRef and dedups via refs,
  // so its only dependency is the stable renderPageBatch. Because it never gets
  // a new identity, it never re-triggers the init effect.
  const loadPageRange = useCallback(
    async (startPage: number, endPage: number, signal: AbortSignal) => {
      const pdfDoc = pdfRef.current;
      if (!pdfDoc) return;

      const totalPages = pdfDoc.numPages;
      const from = Math.max(0, startPage);
      const to = Math.min(totalPages - 1, endPage);
      if (from > to) return;

      // Reserve pages synchronously (no await between the check and the add) so
      // concurrent callers — the init batch and the visible-range batch — never
      // render the same page twice.
      const pagesToLoad: number[] = [];
      for (let i = from; i <= to; i++) {
        if (!loadedRef.current.has(i) && !loadingRef.current.has(i)) {
          loadingRef.current.add(i);
          pagesToLoad.push(i + 1); // 1-based for pdf.getPage
        }
      }
      if (pagesToLoad.length === 0) return;

      try {
        const previews = await renderPageBatch(pdfDoc, pagesToLoad, signal);

        if (!signal.aborted && previews.length > 0) {
          for (const preview of previews) {
            loadedRef.current.add(preview.pageNumber - 1);
          }
          setState((prev) => {
            const newPages = [...prev.pages];
            for (const preview of previews) {
              // Guard against a duplicate entry if a page rendered twice.
              if (newPages.some((p) => p.pageNumber === preview.pageNumber)) {
                continue;
              }
              const insertIndex = newPages.findIndex(
                (p) => p.pageNumber > preview.pageNumber,
              );
              if (insertIndex === -1) newPages.push(preview);
              else newPages.splice(insertIndex, 0, preview);
            }
            return { ...prev, pages: newPages };
          });
        }
      } catch (error) {
        if (!signal.aborted) {
          console.error(
            "[progressive-pages] failed to load page batch:",
            error,
          );
        }
      } finally {
        // Always release the reservation so aborted/failed pages can be retried
        // on a later pass (a lingering loading mark would block them forever).
        for (const p of pagesToLoad) {
          loadingRef.current.delete(p - 1);
        }
      }
    },
    [renderPageBatch],
  );

  // Initialize the PDF document and load the first batch. Deps are limited to
  // the true document identity (file/enabled/cacheKey) plus the stable
  // loadPageRange, so page-loading state churn no longer re-runs this effect.
  useEffect(() => {
    if (!file || !enabled) {
      initAbortRef.current?.abort();
      visibleAbortRef.current?.abort();
      loadedRef.current = new Set();
      loadingRef.current = new Set();
      if (pdfRef.current) {
        pdfWorkerManager.destroyDocument(pdfRef.current);
        pdfRef.current = null;
      }
      setState({ pages: [], loading: false, totalPages: 0 });
      return;
    }

    let cancelled = false;
    const initAbort = new AbortController();
    initAbortRef.current = initAbort;

    // Fresh document => reset all per-document bookkeeping.
    loadedRef.current = new Set();
    loadingRef.current = new Set();
    setState({ pages: [], loading: true, totalPages: 0 });

    const initialize = async () => {
      try {
        const arrayBuffer = await file.arrayBuffer();
        const pdf = await pdfWorkerManager.createDocument(arrayBuffer, {
          disableAutoFetch: true,
          disableStream: true,
        });

        // If this effect was torn down while the document was opening (file
        // change or StrictMode double-invoke), discard the new document instead
        // of adopting it — otherwise it would render against a doc nobody owns.
        if (cancelled || initAbort.signal.aborted) {
          pdfWorkerManager.destroyDocument(pdf);
          return;
        }

        pdfRef.current = pdf;
        const totalPages = pdf.numPages;
        setState((prev) => ({ ...prev, totalPages, loading: false }));

        const firstBatchEnd = Math.min(BATCH_SIZE - 1, totalPages - 1);
        await loadPageRange(0, firstBatchEnd, initAbort.signal);
      } catch (error) {
        if (!cancelled) {
          console.error("[progressive-pages] failed to initialize PDF:", error);
          setState({ pages: [], loading: false, totalPages: 0 });
        }
      }
    };

    initialize();

    return () => {
      cancelled = true;
      initAbort.abort();
      visibleAbortRef.current?.abort();
      if (pdfRef.current) {
        pdfWorkerManager.destroyDocument(pdfRef.current);
        pdfRef.current = null;
      }
      loadedRef.current = new Set();
      loadingRef.current = new Set();
    };
  }, [file, enabled, cacheKey, loadPageRange]);

  // Load pages as the visible range changes (with a small buffer). Keyed on the
  // primitive start/end so an unchanged range with a new object identity does
  // not re-fire. loadPageRange is stable, so this settles immediately.
  useEffect(() => {
    if (!visiblePageRange || state.totalPages === 0 || !pdfRef.current) return;

    const { start, end } = visiblePageRange;
    const startPage = Math.max(0, start - 5);
    const endPage = Math.min(state.totalPages - 1, end + 5);

    visibleAbortRef.current?.abort();
    const abortController = new AbortController();
    visibleAbortRef.current = abortController;

    void loadPageRange(startPage, endPage, abortController.signal);

    return () => {
      abortController.abort();
    };
    // Keyed on primitive bounds (not the object) to avoid identity churn.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [
    visiblePageRange?.start,
    visiblePageRange?.end,
    state.totalPages,
    loadPageRange,
  ]);

  // Final safety net: abort in-flight work and destroy the document on unmount.
  // (The init effect's own cleanup already handles file/enabled/cacheKey changes.)
  useEffect(() => {
    return () => {
      initAbortRef.current?.abort();
      visibleAbortRef.current?.abort();
      if (pdfRef.current) {
        pdfWorkerManager.destroyDocument(pdfRef.current);
        pdfRef.current = null;
      }
    };
  }, []);

  return {
    pages: state.pages,
    loading: state.loading,
    totalPages: state.totalPages,
    loadedPages: loadedRef.current,
    loadingPages: loadingRef.current,
  };
};

export type UseProgressivePagePreviewsReturn = ReturnType<
  typeof useProgressivePagePreviews
>;
