import { useEffect, useRef } from "react";
import { useScroll } from "@embedpdf/plugin-scroll/react";
import { useViewer } from "@app/contexts/ViewerContext";
import { useActiveDocumentId } from "@app/components/viewer/useActiveDocumentId";
import { useDocumentReady } from "@app/components/viewer/hooks/useDocumentReady";
import { pageMemoryService } from "@app/services/pageMemoryService";

/**
 * Connects the PDF scroll plugin to the shared ViewerContext.
 */
export function ScrollAPIBridge() {
  const activeDocumentId = useActiveDocumentId();
  const documentReady = useDocumentReady();

  // Don't render the inner component until we have a valid document ID and document is ready
  if (!activeDocumentId || !documentReady) {
    return null;
  }

  return <ScrollAPIBridgeInner documentId={activeDocumentId} />;
}

function ScrollAPIBridgeInner({ documentId }: { documentId: string }) {
  const { provides: scroll, state: scrollState } = useScroll(documentId);
  const { registerBridge, triggerImmediateScrollUpdate } = useViewer();

  // Keep scroll ref updated to avoid re-running effect when object reference changes
  const scrollRef = useRef(scroll);
  useEffect(() => {
    scrollRef.current = scroll;
  }, [scroll]);

  // Extract primitive values to avoid dependency on object references
  const currentPage = scrollState?.currentPage;
  const totalPages = scrollState?.totalPages;

  useEffect(() => {
    const currentScroll = scrollRef.current;
    if (
      currentScroll &&
      currentPage !== undefined &&
      totalPages !== undefined
    ) {
      const newState = {
        currentPage,
        totalPages,
      };

      // Trigger immediate update for responsive UI
      triggerImmediateScrollUpdate(newState.currentPage, newState.totalPages);

      // Save to persistent storage if page > 0
      if (documentId && currentPage > 0) {
        pageMemoryService.savePage(documentId, currentPage);
      }

      registerBridge("scroll", {
        state: newState,
        api: currentScroll,
      });
    }

    return () => {
      registerBridge("scroll", null);
    };
  }, [
    currentPage,
    totalPages,
    registerBridge,
    triggerImmediateScrollUpdate,
    documentId,
  ]);

  // Restore saved page on initial load
  const hasRestoredPage = useRef(false);
  useEffect(() => {
    if (!scroll || hasRestoredPage.current) return;

    // Only attempt to restore if we have a valid scroll API and documentId
    if (documentId) {
      const savedPage = pageMemoryService.getPage(documentId);
      if (savedPage && savedPage > 1) {
        // EmbedPDF scroll API is 1-indexed for scrollToPage
        scroll.scrollToPage({ pageNumber: savedPage });
      }
      hasRestoredPage.current = true;
    }
  }, [scroll, documentId]);

  return null;
}
