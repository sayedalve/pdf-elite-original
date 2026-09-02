# Rendering Logic Comparison

This document details the rendering architectures of the reference implementations, specifically Okular, and how they apply to PDF Elite (Phase 6).

## 1. Okular's Quad-Tree Tile Cache
**Strongest Reference:** Okular
**Location:** `src/core/tilesmanager.cpp`, `src/core/tilesmanager_p.h`

**Concept:**
Okular implements an incredibly robust, deeply mature rendering architecture built on a Quad-Tree `TileNode` structure.
* **Page Virtualization & Tiling:** Instead of rendering an entire page at high zoom, Okular divides the page into a 4x4 base grid of tiles.
* **Quad-Tree Splitting (`splitBigTiles`):** If a tile exceeds a predefined memory limit (`TILES_MAXSIZE`), it recursively splits into 4 smaller children tiles. This ensures that memory consumption stays flat even at 10,000% zoom.
* **Tile Priority & Ranking:** The `rankTiles` function orders rendering requests by evaluating a tile's `dirty` state and its geometric `distance` from the current visible viewport. Tiles currently on-screen are processed instantly, while off-screen tiles are prefetched in the background.
* **Progressive Rendering:** A low-resolution parent tile is shown while the high-resolution children tiles are asynchronously generated.

## 2. Current PDF Elite Architecture
PDF Elite currently relies on `RenderWorker` and `TileCache`.
* **Strengths:** Opening, basic scrolling, and initial rendering are already highly optimized and stable. We must strictly avoid regressing these.
* **Weaknesses (Assumed):** Likely relies on a flat grid or full-page rendering at lower zoom levels, which can cause memory spikes at extreme zooms or stuttering when panning rapidly.

## 3. Proposed Improvements for PDF Elite
We will selectively adapt Okular's tile management strategies to patch weaknesses in PDF Elite's `RenderWorker`, without replacing the pipeline blindly.

* **Memory-Bound Splitting:** We will integrate Okular's concept of `TILES_MAXSIZE`. If our `TileCache` detects a requested tile bitmap exceeds a byte threshold, it should transparently partition the request into a quad-tree to prevent massive contiguous memory allocations.
* **Distance-Based Priority Queue:** We will upgrade our `RenderWorker`'s job queue to rank tiles based on distance from the exact visible `ViewportRect`. 
* **Render Cancellation:** If a fast scroll occurs, tiles that move significantly out of the viewport radius must be forcefully evicted from the active `RenderWorker` queue before they steal CPU cycles from the newly visible tiles.

## Conclusion
We will preserve PDF Elite's existing `RenderWorker` threading model and `PdfViewer` UI handling, as it is already working well. We will carefully graft Okular's Quad-Tree memory limits and distance-based priority ranking into our existing `TileCache` to guarantee 60fps scrolling and infinite zoom stability.
