import { useEffect, useState, useRef } from "react";
import { useSearch } from "@embedpdf/plugin-search/react";
import { useViewer } from "@app/contexts/ViewerContext";
import { useActiveDocumentId } from "@app/components/viewer/useActiveDocumentId";
import { useDocumentReady } from "@app/components/viewer/hooks/useDocumentReady";

interface SearchResult {
  pageIndex: number;
  rects: Array<{
    origin: { x: number; y: number };
    size: { width: number; height: number };
  }>;
}

/**
 * SearchAPIBridge - Updated for embedPDF v2.6.0
 * Connects the PDF search plugin to the shared ViewerContext.
 */
export function SearchAPIBridge() {
  const activeDocumentId = useActiveDocumentId();
  const documentReady = useDocumentReady();

  // Don't render the inner component until we have a valid document ID and document is ready
  if (!activeDocumentId || !documentReady) {
    return null;
  }

  return <SearchAPIBridgeInner documentId={activeDocumentId} />;
}

function SearchAPIBridgeInner({ documentId }: { documentId: string }) {
  const { provides: search } = useSearch(documentId);
  const { registerBridge, scrollActions, triggerImmediateSearchUpdate } =
    useViewer();

  // Keep search ref updated to avoid re-running effects when object reference changes
  const searchRef = useRef(search);
  const isSearchingRef = useRef(false);
  const lastScrolledIndexRef = useRef<number | null>(null);

  useEffect(() => {
    searchRef.current = search;
  }, [search]);

  const [localState, setLocalState] = useState({
    results: null as SearchResult[] | null,
    activeIndex: 0,
  });

  // Subscribe to search result changes from EmbedPDF
  const subscriptionRef = useRef<(() => void) | null>(null);

  useEffect(() => {
    // Cleanup previous subscription
    if (subscriptionRef.current) {
      subscriptionRef.current();
      subscriptionRef.current = null;
    }

    if (!search) return;

    subscriptionRef.current =
      search.onSearchResultStateChange?.((state: any) => {
        if (!state) return;

        let validResults = null;
        let newActiveIndex = state.activeResultIndex || 0;

        if (state.results) {
          validResults = [];
          const seen = new Set<string>();

          for (let i = 0; i < state.results.length; i++) {
            const res = state.results[i];

            // Filter hidden matches (zero width/height)
            const hasVisibleRect =
              res.rects &&
              res.rects.some((r: any) => r.size.width > 0 && r.size.height > 0);
            if (!hasVisibleRect) {
              if (i <= (state.activeResultIndex || 0) && newActiveIndex > 0)
                newActiveIndex--;
              continue;
            }

            // Filter duplicate matches (identical rounded coordinates)
            const key =
              res.pageIndex +
              ":" +
              res.rects
                .map(
                  (r: any) =>
                    `${Math.round(r.origin.x)},${Math.round(r.origin.y)},${Math.round(r.size.width)},${Math.round(r.size.height)}`,
                )
                .join("|");
            if (seen.has(key)) {
              if (i <= (state.activeResultIndex || 0) && newActiveIndex > 0)
                newActiveIndex--;
              continue;
            }

            seen.add(key);
            validResults.push({ ...res, originalIndex: i });
          }
        }

        const newState = {
          results: validResults,
          activeIndex: newActiveIndex + 1, // Convert to 1-based index
        };

        setLocalState((prevState) => {
          // Only update if state actually changed
          if (
            prevState.results !== newState.results ||
            prevState.activeIndex !== newState.activeIndex
          ) {
            return newState;
          }
          return prevState;
        });
      }) ?? null;

    return () => {
      if (subscriptionRef.current) {
        subscriptionRef.current();
        subscriptionRef.current = null;
      }
    };
  }, [search]);

  // Extract primitive values from localState to avoid object reference dependencies
  const localResults = localState.results;
  const localActiveIndex = localState.activeIndex;

  // Scroll to active result when it changes (for next/previous navigation)
  useEffect(() => {
    if (!localResults || localResults.length === 0) {
      lastScrolledIndexRef.current = null;
      return;
    }

    // Only scroll if the active index actually changed
    const activeResultIndex = localActiveIndex - 1; // Convert back to 0-based
    if (
      activeResultIndex >= 0 &&
      activeResultIndex < localResults.length &&
      lastScrolledIndexRef.current !== activeResultIndex
    ) {
      const activeResult = localResults[activeResultIndex];
      if (activeResult) {
        const pageNumber = activeResult.pageIndex + 1; // Convert to 1-based page number
        scrollActions.scrollToPage(pageNumber);
        lastScrolledIndexRef.current = activeResultIndex;
      }
    }
  }, [localResults, localActiveIndex, scrollActions]);

  // Register bridge whenever state changes
  useEffect(() => {
    const currentSearch = searchRef.current;
    if (currentSearch) {
      registerBridge("search", {
        state: { results: localResults, activeIndex: localActiveIndex },
        api: {
          search: async (query: string) => {
            // Prevent overlapping searches
            if (isSearchingRef.current) {
              return null;
            }

            if (!currentSearch?.startSearch || !currentSearch?.searchAllPages) {
              return null;
            }

            isSearchingRef.current = true;

            try {
              currentSearch.startSearch();
              const results = await currentSearch.searchAllPages(query);
              return results;
            } catch (error: any) {
              // Handle abort errors gracefully - these occur when searches overlap
              if (
                error?.type === "abort" ||
                error?.message?.includes("abort")
              ) {
                // Silently handle abort - this is expected when user types quickly
                return null;
              }
              console.error("Search failed:", error);
              return null;
            } finally {
              isSearchingRef.current = false;
            }
          },
          clear: () => {
            try {
              if (currentSearch?.stopSearch) {
                currentSearch.stopSearch();
              }
            } catch (error) {
              console.warn("Error stopping search:", error);
            }
            setLocalState({ results: null, activeIndex: 0 });
          },
          next: () => {
            try {
              if (localResults && localResults.length > 0) {
                const nextIndex =
                  (localActiveIndex - 1 + 1) % localResults.length;
                currentSearch?.goToResult?.(
                  (localResults[nextIndex] as any).originalIndex,
                );
              } else {
                currentSearch?.nextResult?.();
              }
            } catch (error) {
              console.warn("Error navigating to next result:", error);
            }
          },
          previous: () => {
            try {
              if (localResults && localResults.length > 0) {
                const prevIndex =
                  (localActiveIndex - 1 - 1 + localResults.length) %
                  localResults.length;
                currentSearch?.goToResult?.(
                  (localResults[prevIndex] as any).originalIndex,
                );
              } else {
                currentSearch?.previousResult?.();
              }
            } catch (error) {
              console.warn("Error navigating to previous result:", error);
            }
          },
          goToResult: (index: number) => {
            try {
              if (localResults && localResults[index]) {
                currentSearch?.goToResult?.(
                  (localResults[index] as any).originalIndex,
                );
              } else {
                currentSearch?.goToResult?.(index);
              }
            } catch (error) {
              console.warn("Error going to result:", error);
            }
          },
        },
      });
      // Phase 29: localResults/localActiveIndex just changed and the context
      // search ref registered above is now current, so notify subscribers. The
      // workbench ViewerShell re-reads getSearchState() on this signal, keeping
      // the visible match count and active index accurate immediately instead
      // of only refreshing as a side effect of an unrelated scroll/zoom tick.
      triggerImmediateSearchUpdate();
    }

    return () => {
      registerBridge("search", null);
    };
  }, [
    localResults,
    localActiveIndex,
    registerBridge,
    triggerImmediateSearchUpdate,
  ]);

  return null;
}
