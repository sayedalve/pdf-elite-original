# PDF Elite — Frontend React/TypeScript Source Analysis

## 1. Component Hierarchy and Rendering Pipeline

### Entry Points

- **`index.html`** → loads `src/index.tsx` as a module entry; applies the user's color-scheme preference via a blocking inline script (reads `localStorage`, sets `data-theme`, `data-app-theme`, `data-accent`, `data-mantine-color-scheme` on `<html>` before first paint, preventing FOUC).

- **`src/index.tsx`** → Imports Mantine CSS, Tailwind CSS, global design tokens. Patches the DOM for browser translators. Eagerly compiles the Pdfium WASM module via `requestIdleCallback`/`setTimeout` on page load. Creates a `ReactDOM.createRoot`, wraps `<App />` in `<React.StrictMode>`, `<ColorSchemeScript />`, and `<BrowserRouter>`.

- **`src/proprietary/App.tsx`** (resolved via `@app/*` → `./src/desktop/*`, `./src/proprietary/*`, `./src/core/*`) → A `<Routes>` component with two routes:
  1. `/workflow/sign/:token` → Public sign-in page (minimal providers: `PreferencesProvider` + `ThemeProvider`).
  2. `*` (catch-all) → The main editor: `<AppProviders>` → `<AppLayout>` → `<Workbench />`.

### Rendering Tree

```
StrictMode
  ColorSchemeScript
  BrowserRouter
    App
      Suspense (LoadingFallback)
        Routes
          Route */workflow/sign/:token → PublicRouteProviders → ParticipantView
          Route * → AppProviders → AppLayout → Workbench
```

#### AppProviders (19 nested providers)

The provider stack, from outermost to innermost:

```
PreferencesProvider
  ThemeProvider
    ErrorBoundary
      BannerProvider
        AppConfigProvider (fetches server config, retry logic)
          PosthogTrackingInitializer (analytics)
          ScarfTrackingInitializer (download tracking)
          AppConfigLoader
          ServerDefaultsSync
          UpdateStartupPopup
          FileContextProvider (IndexedDBProvider inside)
            FolderProvider
              AppInitializer (useAppInitialization)
              BrandingAssetManager
              ToolRegistryProvider
                NavigationProvider
                  FilesModalProvider
                    ToolWorkflowProvider
                      HotkeyProvider
                        SidebarProvider
                          ViewerProvider
                            PageEditorProvider
                              SignatureProvider
                                SigningOverlayProvider
                                  RedactionProvider
                                    FormFillProvider
                                      AnnotationProvider
                                        WorkbenchBarProvider
                                          TourOrchestrationProvider
                                            AdminTourOrchestrationProvider
                                              FolderFileContextProvider
                                                {children} → Workbench
```

#### AppLayout

A flex column (`height: 100vh`) that renders the banner at top, the children (Workbench) in the remaining space (`flex: 1, minHeight: 0`), plus `NavigationWarningModal` and `LoginAgreementModal` as global overlays.

#### Workbench

The Workbench component (imported from `@app/components/layout/Workbench`) is the main application shell. Based on `NavigationContext`, it renders either the home/tools interface or the viewer interface.

---

## 2. State Management Architecture

### FileContext (Central State)

The **FileContext** is the most critical state container. It manages PDF files for the multi-tool workflow.

- **Implementation**: `useReducer` with a `fileContextReducer` and `initialFileContextState`.
- **Data model**: A normalized entity map (`state.files.byId: Record<FileId, StirlingFileStub>`) + ordered ID array (`state.files.ids: FileId[]`). File `Blob` objects are stored in a `useRef<Map<FileId, File>>` to avoid React re-renders.
- **Split context pattern**: `FileStateContext` (read-only state + selectors) and `FileActionsContext` (action dispatchers) are separate providers to minimize re-renders.
- **Selectors**: Memoized `createFileSelectors(stateRef, filesRef)` provide stable accessor functions that never cause re-renders (empty dependency array).
- **Key hooks**: `useFileState()`, `useFileActions()`, `useCurrentFile()`, `useFileSelection()`, `useFileManagement()`, `useFileUI()`, `useAllFiles()`, `useSelectedFiles()`, `useFileContext()`.
- **Persistence**: Optional `IndexedDBProvider` wraps the inner context when `enablePersistence=true`. Files can be stored/restored from IndexedDB. Bumping a revision counter triggers sync.
- **File chaining**: `consumeFiles()` replaces input files with output files (maintaining parent-child stub relationships), enabling tool chaining (split → merge → compress → view) without reloading.
- **Undo**: `undoConsumeFiles()` restores original files from IndexedDB.
- **Encrypted PDF handling**: A queue-based system (`encryptedQueue`, `activeEncryptedFileId`) manages password prompts for encrypted files. An `EncryptedPdfUnlockModal` is rendered as a portal child.

### ViewerContext

Manages all viewer state and acts as a bridge registry for the EmbedPDF viewer.

- **UI state**: Sidebar visibility (thumbnails, bookmarks, attachments, layers, comments), search interface visibility, annotation mode, active file ID/index, PDF render mode (normal/dark/sepia).
- **Bridge pattern**: A `ViewerBridgeRegistry` (ref-based) stores refs to bridge components (ZoomAPIBridge, ScrollAPIBridge, etc.). Each bridge registers itself and exposes its API. The context provides getter functions (`getScrollState()`, `getZoomState()`, etc.) that read state directly from bridge refs — avoiding React state updates for high-frequency viewer events.
- **Immediate notifiers**: A `useImmediateNotifier` hook creates synchronous callback sets for zoom, scroll, spread, pan, and search updates, allowing bridges to push state changes without going through React's render cycle.
- **Action creators**: `createViewerActions()` builds scroll, zoom, pan, selection, spread, rotation, search, export, bookmark, attachment, and print actions that call through to EmbedPDF plugin APIs.

### Other Key Contexts

| Context | Purpose |
|---------|---------|
| **NavigationContext** | Current view (home/viewer/tool), page routing, workbench state |
| **ToolRegistryContext** | Registry of all available tools, metadata, categories |
| **ToolWorkflowContext** | Active tool selection, tool execution flow |
| **HotkeyContext** | Customizable keyboard shortcuts per tool |
| **SidebarContext** | Left sidebar visibility, panel view mode (toolPicker/toolContent), reader mode |
| **PreferencesContext** | User preferences (theme, language, hidden tools, etc.) |
| **AppConfigContext** | Server configuration, feature flags, capabilities |
| **AnnotationContext** | Annotation state for the viewer |
| **SignatureContext** | Digital signature management |
| **RedactionContext** | Redaction operation state |
| **PageEditorContext** | Page editing mode state |
| **SigningOverlayContext** | Signing workflow overlay state |
| **BannerContext** | App-level banner/notification display |
| **WorkbenchBarContext** | Workbench toolbar state |
| **WorkspaceContext** | Workspace-level state management |
| **IndexedDBContext** | IndexedDB access abstraction |
| **FolderContext** | Folder/file organization |
| **FolderFileContext** | File-folder association state |
| **FileManagerContext** | File manager modal state |
| **FilesModalContext** | File picker modal state |
| **FilesPageContext** | Files page state |
| **SaaSTeamContext** | SaaS team/organization state |
| **CommentAuthorContext** | Comment author identity |
| **TourOrchestrationContext** | Guided tour/orchestration |
| **AdminTourOrchestrationContext** | Admin-specific tour orchestration |
| **UnsavedChangesContext** | Unsaved changes tracking |
| **ToolActionsContext** | Tool action callbacks (desktop: sign-in prompt on unavailable tools) |

---

## 3. Tool System Architecture

### Tool Registry

- **`ToolRegistryProvider`** → **`ToolRegistryContext`**: Maintains a map of `ToolId → ToolRegistryEntry`. Each entry contains tool metadata (name, description, category, icon, endpoint, etc.). Tools are organized by `ToolCategoryId` in a taxonomy (`toolsTaxonomy`).

### Tool Workflow

- **`ToolWorkflowContext`**: Manages the active tool selection. `handleToolSelect(toolId)` navigates to the tool's interface. `ToolWorkflowProvider` wraps the children and exposes the registry plus selection handlers.

### Tool Operation Hooks (Pattern)

Every tool follows a consistent pattern:

1. **Operation Config** (static, defined at module level):
   ```ts
   export const compressOperationConfig = defineSingleFileTool<ErasedToolParams>({
     operationType: "compress",
     endpoint: "/api/v1/misc/compress-pdf",
     buildFormData: (params, file) => objectToFormData(params, { fileInput: file }),
   });
   ```

2. **Hook** (wraps `useToolOperation`):
   ```ts
   export const useCompressOperation = () => {
     const { t } = useTranslation();
     return useToolOperation<ErasedToolParams>({
       ...compressOperationConfig,
       getErrorMessage: createStandardErrorHandler(t("compress.error.failed")),
     });
   };
   ```

3. **`useToolOperation<TParams>`** is the universal executor. It handles:
   - **Three tool types**: `singleFile` (one request per file), `multiFile` (one request with all files), `custom` (custom processor function).
   - **UI state**: `isLoading`, `errorMessage`, `status`, `progress`, `files`, `thumbnails`, `downloadUrl`, `downloadFilename`.
   - **File chaining**: Calls `consumeFiles()` to update FileContext with output files and parent-child stubs.
   - **Undo support**: `undoOperation()` restores input files via `undoConsumeFiles()`.
   - **Cancellation**: `AbortController` support via `cancelOperation()`.

### Tool Categories (40+ tools)

```
addAttachments, addPassword, addWatermark, adjustContrast, adjustPageScale,
autoRename, autoRotate, automate, bookletImposition, certSign,
changeMetadata, changePermissions, compress, convert, crop,
editTableOfContents, extractImages, extractPages, flatten,
getPdfInfo, insertBlankPages, merge, ocr, overlayPdfs,
pageLayout, redact, removeAnnotations, removeBlanks, removeImage,
removePages, removePassword, reorganizePages, repair, replaceColor,
replaceImage, rotate, sanitize, scannerImageSplit, sign,
singleLargePage, split, timestampPdf, unlockPdfForms, validateSignature
```

### Tool Automation

- `toolAutomation.ts` provides automation support for chaining tool operations.
- `useAccordionSteps.ts` provides step-based UI for multi-step tool flows.
- `useBaseParameters.ts` and `useViewScopedFiles.ts` provide shared parameter/file handling.

---

## 4. PDF Rendering Architecture

### Primary Engine: EmbedPDF with Pdfium WASM

The viewer uses **`@embedpdf/core`** with the **`@embedpdf/engines`** Pdfium engine (NOT PDF.js for the main viewer). This is a WASM-based PDF renderer.

```ts
const { engine, isLoading, error } = usePdfiumEngine({ wasmUrl: pdfiumWasmUrl });
```

- **WASM precompilation**: `wasmPrecompiler.ts` eagerly compiles the Pdfium WASM module via `WebAssembly.compileStreaming()` during idle time (using `requestIdleCallback`), so it's ready when the user opens a PDF.
- **PDF.js is used as a secondary engine** via `PDFWorkerManager` for metadata extraction, thumbnail generation, and operations that don't need the full viewer.

### Plugin Architecture

LocalEmbedPDF registers ~20 EmbedPDF plugins:

| Plugin | Purpose |
|--------|---------|
| `DocumentManagerPluginPackage` | Manages PDF document lifecycle, initial URL/scale (96/72 DPI conversion) |
| `ViewportPluginPackage` | Viewport sizing, gap between pages (3.5rem) |
| `ScrollPluginPackage` | Scroll handling, page tracking |
| `RenderPluginPackage` | Page rendering (respects `withForms`, `withAnnotations` flags) |
| `InteractionManagerPluginPackage` | Pointer/touch event routing (required by zoom, selection) |
| `SelectionPluginPackage` | Text/area selection (marquee disabled, tolerance factor 3) |
| `HistoryPluginPackage` | Undo/redo for annotations |
| `AnnotationPluginPackage` | Annotation tools (highlight, ink, sticky note, etc.) |
| `RedactionPluginPackage` | Redaction markup |
| `PanPluginPackage` | Pan/scroll gesture (default mode: "never", NOT "mobile" to avoid blocking text selection) |
| `ZoomPluginPackage` | Zoom levels (FitWidth default, 0.2x–5.0x range) |
| `TilingPluginPackage` | Tile-based rendering (768px tiles, 5px overlap, 1 extra ring) |
| `SpreadPluginPackage` | Dual-page layout |
| `SearchPluginPackage` | Text search |
| `ThumbnailPluginPackage` | Page thumbnail generation |
| `BookmarkPluginPackage` | PDF outline/bookmarks |
| `AttachmentPluginPackage` | PDF attachments |
| `RotatePluginPackage` | Page rotation |
| `ExportPluginPackage` | PDF download |
| `PrintPluginPackage` | Print support |

### Canvas/Tiling Rendering

The `TilingPluginPackage` with `tileSize: 768, overlapPx: 5, extraRings: 1` implements a virtual-tile rendering system. The `RenderPluginPackage` renders PDF pages to canvas tiles. The `ViewportPluginPackage` manages which tiles are visible and should be rendered. This is NOT WebGL — it's standard HTML Canvas 2D rendering via the Pdfium WASM engine.

---

## 5. Viewer Display (Scroll, Zoom, Page Navigation)

### Viewer Component Hierarchy

```
Viewer (route component)
  → ViewerShell (tabs, toolbar, sidebar layout)
    → TabBar (multi-document tabs)
    → ContextualToolbar (mode-dependent toolbar)
    → ViewerLeftRail (mode selection sidebar)
    → ViewerCenter
      → EmbedPdfViewer → LocalEmbedPDF → <EmbedPDF engine={engine} plugins={plugins}>
        → Viewport
          → Scroller
            → TilingLayer (tile rendering)
            → SelectionLayer
            → LinkLayer
            → CustomSearchLayer
            → SignaturePreviewLayer
            → SignatureFieldOverlay
            → FormFieldOverlay
            → ButtonAppearanceOverlay
            → StickyNoteHoverLayer
            → TextSelectionHandler → TextSelectionMenu
            → RedactionSelectionMenu
            → AnnotationSelectionMenu
            → RedactionPendingTracker
            → RulerOverlay
      → CommentMode (absolute positioned panel)
      → EditMode (absolute positioned panel)
    → RightUtilityPanel (bookmarks, search results, document info)
```

### Scroll

- Handled by `ScrollPluginPackage` + `ScrollAPIBridge`.
- `ViewerContext` exposes `scrollActions.scrollToPage(page)`, `getScrollState()` → `{currentPage, totalPages}`.
- Immediate scroll updates via `registerImmediateScrollUpdate(callback)` for responsive page counters.
- `PageMemoryService` persists last-viewed page per document to `localStorage` (capped at 200 entries).

### Zoom

- Handled by `ZoomPluginPackage` + `ZoomAPIBridge`.
- Default: `ZoomMode.FitWidth`. Range: 0.2x–5.0x.
- Actions: `zoomIn()`, `zoomOut()`, `requestZoom("fit-width"|"fit-page"|"actual-size")`, `setZoomLevel(scale)`.
- Immediate zoom updates via `registerImmediateZoomUpdate(callback)`.
- Page memory also saves zoom level per document.

### Page Navigation

- `scrollActions.scrollToPage(page)` for programmatic navigation.
- `scrollActions.nextPage()` / `scrollActions.previousPage()`.
- Tab-based navigation for multi-document viewing via `activeFileId` / `activeFileIndex`.

---

## 6. Text Selection and Search

### Text Selection

- `SelectionPluginPackage` + `SelectionAPIBridge`.
- `TextSelectionHandler` component manages text selection UI.
- `TextSelectionMenu` appears on text selection with options (copy, highlight, etc.).
- Selection state accessible via `getSelectionState()` → `{hasSelection, ...}`.
- `selectionActions` provide programmatic selection control.

### Search

- `SearchPluginPackage` + `SearchAPIBridge`.
- `CustomSearchLayer` renders search highlights on the PDF.
- `SearchInterface` component provides the search UI.
- `ViewerContext.searchActions` provides `search(query)`, `next()`, `previous()`, `clear()`, `goToResult(index)`.
- Search state: `getSearchState()` → `{results: SearchMatch[], activeIndex: number}`.
- Immediate search updates via `registerImmediateSearchUpdate(callback)`.
- Ctrl+F in `ViewerShell` opens search mode with a 300ms debounce.
- The `ViewerShell` renders a search interface in the `ContextualToolbar` with next/prev navigation and match count.

---

## 7. Editing UI Components

### Viewer Modes

The `ViewerShell` supports multiple modes:
- **View mode** (`"view"`): Default PDF viewing.
- **Organize mode** (`"organize"`): Page reorganization via `OrganizeMode` component.
- **Comment mode** (`"comment"`): Annotation comments via `CommentMode` (absolute positioned panel).
- **Edit mode** (`"edit"`): Page editing via `EditMode` (absolute positioned panel).
- **Search mode** (`"search"`): Activated by Ctrl+F, shows search bar.

### Annotations

- `AnnotationAPIBridge` connects to `@embedpdf/plugin-annotation`.
- Annotation types registered: Highlight, Ink, StickyNote, Underline, StrikeOut, Link, FreeText, Square, Circle, Line, Polygon, Polyline.
- `AnnotationSelectionMenu` appears on annotation selection.
- `ViewerAnnotationControls` provides annotation tool controls.
- `AnnotationTypeButtons` renders the annotation type picker.
- `AnnotationMenuButtons` renders per-annotation action buttons.

### Redaction

- `RedactionAPIBridge` + `RedactionPluginPackage`.
- `RedactionPendingTracker` tracks pending redactions.
- `RedactionSelectionMenu` appears on redaction mark selection.

### Form Fill

- `FormFieldOverlay` and `ButtonAppearanceOverlay` for interactive form fields.
- `FormFillProvider` wraps the context.

---

## 8. Thumbnail System

- `ThumbnailPluginPackage` generates thumbnails via the EmbedPDF engine.
- `ThumbnailAPIBridge` exposes the thumbnail API to `ViewerContext.getThumbnailAPI()`.
- `ThumbnailSidebar` renders the page thumbnail sidebar (toggled via `ViewerContext.toggleThumbnailSidebar()`).
- **`ThumbnailGenerationService`** (service layer, used outside the viewer):
  - Uses Pdfium WASM (`pdfiumService`) for rendering, NOT PDF.js.
  - Caches up to 10 PDF documents with LRU eviction.
  - 1GB session thumbnail cache with size-based eviction.
  - Batch processing with configurable parallelism.
  - Returns data URLs for thumbnails.

---

## 9. Sidebar System

### Left Rail

`ViewerLeftRail` renders the left sidebar with mode selection:
- Tool picker / tool content view (controlled by `SidebarContext.leftPanelView`).
- Page navigation (thumbnails).

### Left Sidebars (Viewer Context)

Each sidebar is independently toggleable via `ViewerContext`:
- **Thumbnail sidebar**: `isThumbnailSidebarVisible`, `ThumbnailSidebar` component.
- **Bookmark sidebar**: `isBookmarkSidebarVisible`, `BookmarkSidebar` component.
- **Attachment sidebar**: `isAttachmentSidebarVisible`, `AttachmentSidebar` component.
- **Layer sidebar**: `isLayerSidebarVisible`, `LayerSidebar` component.
- **Comments sidebar**: `isCommentsSidebarVisible`, `CommentsSidebar` component.

### Right Utility Panel

`RightUtilityPanel` renders the right side panel with:
- Search results list.
- Document bookmarks (outline tree).
- Document info (title, page count).
- Collapsible via `rightCollapsed` state.

### Sidebar Context

`SidebarContext` manages:
- `sidebarsVisible` (boolean) — global sidebar visibility.
- `leftPanelView` (`"toolPicker"` | `"toolContent"`) — which panel is shown.
- `readerMode` (boolean) — hides sidebars for focused reading.
- DOM refs for `quickAccessRef` and `toolPanelRef`.

---

## 10. Toolbar and Menu System

### Contextual Toolbar

`ContextualToolbar` changes appearance based on the active mode:
- **View mode**: Hand/select tools, zoom controls.
- **Search mode**: Search input, next/prev, match count, close.
- **Annotate mode**: Annotation tool buttons, highlight colors.
- **Other modes**: Mode-specific controls.

### Viewer Toolbar

`PdfViewerToolbar` provides the main viewer toolbar with zoom, page navigation, and mode switching.

### Workbench Bar

`WorkbenchBarContext` provides the workbench toolbar state. `useViewerWorkbenchBarButtons` generates viewer-specific toolbar buttons.

---

## 11. Dark Theme Implementation

### Multi-Layer Theming

1. **Blocking inline script** in `index.html`: Reads `localStorage("PDF Elitepdf_preferences")`, applies `data-theme`, `data-app-theme`, `data-accent`, `data-mantine-color-scheme` before first paint.

2. **`ThemeProvider`** (from `@app/components/shared/ThemeProvider`): Mantine-based theme provider that manages the color scheme.

3. **`PreferencesContext`** → `preferencesService`: Persists theme preference (`"light"`, `"dark"`, `"system"`).

4. **CSS variables**: The UI uses CSS custom properties (`--app-bg`, `--tab-bar-bg`, `--border`, `--text-primary`, `--text-secondary`, `--accent`, `--surface-hover`, `--surface-elevated`, `--surface-card`, `--workspace-paper-bg`, `--page-shadow`, `--destructive`, `--success`, `--text-inverse`, `--text-tertiary`, `--font-sans`). These are set by design tokens (`design-tokens.css`).

5. **PDF render mode** (separate from app theme): `ViewerContext.pdfRenderMode` cycles through `"normal" → "dark" → "sepia"`, applied as a CSS filter on rendered PDF canvas tiles. This is viewer-only and never modifies the PDF.

---

## 12. Keyboard Shortcuts

### HotkeyContext

- Stores bindings as `Partial<Record<ToolId, HotkeyBinding>>`.
- **Default bindings**: Quick Access tools (RECOMMENDED_TOOLS category) get `Cmd+Option+1-9` (Mac) or `Ctrl+Alt+1-9` (Windows).
- **Customization**: Users can reassign shortcuts. Custom bindings stored in `localStorage("stirlingpdf.hotkeys")`.
- **Conflict detection**: `isBindingAvailable(binding, excludeToolId)` checks for conflicts.
- **Pause/resume**: `pauseHotkeys()` / `resumeHotkeys()` for modal/input contexts.
- **Target filtering**: Ignores events from `input`, `textarea`, `[contenteditable]`, `[role="textbox"]`.
- **Global listener**: `window.addEventListener("keydown", handler, true)` with capture phase.

### Built-in Shortcuts

- **Ctrl+F**: Opens search (handled in `ViewerShell`).
- **Escape**: Closes search → resets temp tool → exits current mode.
- **Ctrl+S**: Desktop save shortcut (via `SaveShortcutListener`).
- **Tool-specific**: Defined per tool via the hotkey system.

---

## 13. Desktop-Specific Code (Tauri Integration)

### Architecture

The `src/desktop/` directory provides the Tauri desktop layer. It follows a **code layering** pattern:

- `@app/*` resolves to `./src/desktop/*` first, then `./src/proprietary/*`, then `./src/core/*`.
- Desktop-specific files override core files via path alias precedence.

### Desktop AppProviders

`desktop__AppProviders.tsx` wraps the proprietary `AppProviders` with:
- **Backend lifecycle**: Starts/stops/monitors the bundled Java backend via `TauriBackendService`.
- **Connection modes**: `local`, `saas`, `selfhosted` — with automatic mode switching.
- **Auth flow**: JWT-based auth with automatic fallback to local mode on expiry.
- **Window management**: Tauri window created with `visible: false`, revealed after first paint via `getCurrentWindow().show()`.
- **Auto-update**: Desktop update popup on startup.
- **Endpoint preloading**: Pre-caches availability for 10 common tool endpoints.
- **Onboarding**: First-launch setup wizard.
- **Key remounting**: Increments `appKey` on connection mode change to force provider tree remount (avoids Windows WebView2 freeze).

### Tauri Backend Service

- `TauriBackendService` (singleton) manages a bundled Java backend:
  - Invokes `start_backend` / `get_backend_port` via Tauri `invoke()`.
  - Uses `127.0.0.1` (not `localhost`) to avoid IPv6 resolution issues on macOS.
  - Health monitoring via polling `/api/v1/config/app-config`.
  - Auto-recovery: Up to 3 restart attempts on backend failure.
  - Status transitions: `stopped` → `starting` → `healthy` → `unhealthy`.

### Operation Router (Desktop)

`OperationRouter` determines where a tool operation executes:
- **Local mode**: Routes to bundled backend at `127.0.0.1:{port}`.
- **SaaS mode**: Checks if local backend supports the endpoint first; falls back to SaaS cloud.
- **Self-hosted mode**: Routes to remote server; falls back to local if server is offline.
- **Cloud-only endpoints**: Team, auth, billing, AI, policy endpoints always route to SaaS backend.
- **Capability checking**: `endpointAvailabilityService.isEndpointSupportedLocally()` queries the backend's capability manifest.

### Desktop Services

| Service | Purpose |
|---------|---------|
| `tauriBackendService` | Backend lifecycle, health monitoring, port discovery |
| `operationRouter` | Local/remote routing, capability-based fallback |
| `connectionModeService` | Connection mode management (local/saas/selfhosted) |
| `authService` | JWT authentication, token management |
| `endpointAvailabilityService` | Endpoint capability checking and caching |
| `selfHostedServerMonitor` | Remote server health monitoring |
| `desktopUpdateService` | Auto-update checking and installation |
| `localFileSaveService` | Native file save dialogs via Tauri |
| `fileDialogService` | Native file open/save dialogs |
| `nativePrintService` | Native print support |
| `platformService` | Platform-specific utilities |
| `desktopNotificationService` | OS notification support |
| `tauriHttpClient` | Tauri HTTP client for API calls |
| `tauriLocalProxy` | Local proxy for API routing |
| `saasAppConfigService` | SaaS app configuration |
| `billing` | Billing/PAYG integration |

### Desktop Hooks

| Hook | Purpose |
|------|---------|
| `useBackendInitializer` | Initializes backend monitoring on app load |
| `useBackendHealth` | Subscribes to backend health status |
| `useAppInitialization` | Full app initialization flow |
| `useFirstLaunchCheck` | Detects first launch for onboarding |
| `useDesktopUpdatePopup` | Manages update popup state |
| `useOpenWindowFiles` | Handles files opened via OS file association |
| `useOpenedFile` | Processes a single opened file |
| `useSaveShortcut` | Ctrl+S save handler |
| `useExitWarning` | Warns on unsaved changes before exit |
| `useNewWindowShortcut` | Ctrl+N new window (multi-window support) |
| `useViewerKeyCommand` | Viewer-specific keyboard commands |
| `useSaaSMode` | SaaS mode detection |
| `useSelfHostedAuth` | Self-hosted authentication |
| `useToolCloudStatus` | Whether a tool routes to cloud |
| `useWillUseCloud` | Combined cloud routing check |
| `useEndpointConfig` | Backend endpoint configuration |
| `useAiEngineEnabled` | AI engine feature flag |
| `useGroupEnabled` | Group collaboration feature |
| `useSharingEnabled` | File sharing feature |
| `useMultiWindowSupported` | Multi-window capability check |

### Tauri Plugins Used

- `@tauri-apps/plugin-dialog` — Native file dialogs.
- `@tauri-apps/plugin-fs` — File system access.
- `@tauri-apps/plugin-http` — HTTP requests (used for backend health checks).
- `@tauri-apps/plugin-notification` — OS notifications.
- `@tauri-apps/plugin-shell` — Shell commands.

---

## 14. API Communication Patterns

### API Client

- **Axios instance** (`apiClient`) with `withCredentials: true`.
- Base URL from `getApiBaseUrl()` (dynamic — changes based on desktop connection mode).
- **Interceptors**: Request/response interceptors for auth (JWT injection), error handling (`httpErrorHandler`), backend readiness gating.
- **Error handling**: `httpErrorHandler` shows toast notifications for API errors. Individual requests can suppress toasts via `suppressErrorToast`.

### Tool API Pattern

All tools communicate via REST endpoints following the pattern:

```
POST /api/v1/{category}/{operation}
Content-Type: multipart/form-data
```

Categories: `general`, `convert`, `misc`, `security`, `filter`, `multi-tool`, `ui-data`.

Responses are typically `responseType: "blob"` (PDF file downloads). The `processResponse()` utility converts blob responses into `File` objects.

### Desktop API Routing

In desktop mode, the `operationRouter` dynamically changes the Axios base URL per-request based on:
1. Current connection mode.
2. Whether the endpoint is supported locally.
3. Whether the endpoint is cloud-only.

---

## 15. Memory Management

### File Lifecycle Manager

`FileLifecycleManager` (referenced in FileContext) handles:
- PDF.js document cleanup (`destroy()` on `PDFDocumentProxy`).
- Blob URL revocation (`URL.revokeObjectURL()`).
- Scheduled cleanup with configurable delays.
- Full cleanup on context unmount (`lifecycleManager.destroy()`).

### PDF.js Worker Manager

- Singleton `PDFWorkerManager` limits concurrent workers to 10 (configurable 1–15).
- Proper lifecycle: `createDocument()` → `destroyDocument()`.
- Queue-based waiting when limit is reached.
- `emergencyCleanup()` for force-destroying all documents.
- Worker URL set once globally via `GlobalWorkerOptions.workerSrc`.

### Blob URL Management

- Blob URLs are tracked via `lifecycleManager.trackBlobUrl()`.
- `URL.revokeObjectURL()` called in cleanup effects (e.g., `LocalEmbedPDF` returns a cleanup function).
- `fileStableKey` prevents blob URL churn when FileContext produces new File references for the same content.

### Thumbnail Cache

- 1GB session cache with LRU eviction.
- 10 cached PDF documents with LRU eviction.
- Thumbnail size tracking for accurate eviction.

### Page Memory Service

- `localStorage`-based, capped at 200 entries.
- Evicts oldest entries when over cap.
- Handles quota exceeded errors silently.

### IndexedDB

- `IndexedDBManager` handles all IndexedDB operations.
- Migration support for schema changes.
- Used for file persistence, tool results, and undo support.

---

## 16. Import/Export Functionality

### Import

- **File upload**: Drag-and-drop via Mantine `@mantine/dropzone`, file input, or Tauri native dialogs.
- **File processing**: `addFiles()` in FileContext handles ZIP extraction (with confirmation for large ZIPs), encrypted PDF detection, thumbnail generation, and metadata extraction.
- **Multiple files**: Supported. Files are normalized to `StirlingFile` objects with unique IDs.
- **OS file association**: Desktop `useOpenWindowFiles` / `useOpenedFile` handles files opened via double-click in the OS.

### Export

- **Direct download**: `ExportPluginPackage` + `ExportAPIBridge`.
- **Export policies**: `enforceExportPolicies()` checks and applies server-side export policies before download/print. May rewrite the in-editor file.
- **Print**: `PrintPluginPackage` + `PrintAPIBridge`. Desktop: `nativePrintService`. Print also goes through policy enforcement.
- **Save**: Desktop: `localFileSaveService` uses Tauri native save dialogs. `useSaveShortcut` handles Ctrl+S.
- **Operation results**: After tool execution, `pdfExportService` handles downloading results.

---

## 17. Clipboard Implementation

- Text selection in the viewer is handled by `SelectionPluginPackage`.
- `TextSelectionHandler` and `TextSelectionMenu` provide the UI for text operations.
- The selection bridge (`SelectionAPIBridge`) connects to the EmbedPDF selection plugin.
- Standard browser clipboard APIs are used for copy/paste operations.
- `DocumentPermissionsAPIBridge` checks `canCopyContents` permission before allowing clipboard operations.

---

## 18. Error Handling Patterns

### API Errors

- **Global interceptor**: `apiClient` has a response error interceptor that calls `handleHttpError()`.
- **Toast notifications**: `alert({ alertType, title, body, durationMs, expandable, isPersistentPopup })` from `@app/components/toast`.
- **Suppressed errors**: Individual requests can set `suppressErrorToast: true` to handle errors in-component.
- **Error utils**: `httpErrorHandler`, `httpErrorUtils`, `errorUtils`, `specialErrorToasts`, `processingErrorHandler`.

### Tool Errors

- `useToolOperation` catches errors, sets `errorMessage` state, and calls `config.getErrorMessage()` for human-readable messages.
- `createStandardErrorHandler()` provides a standard error message factory per tool.
- `handlePasswordError()` for encrypted PDF password errors.

### React Errors

- `ErrorBoundary` wraps the entire app (inside `AppProviders`).
- `error-catcher.js` script loaded before React for global error catching.

### Desktop Errors

- Backend failure: Toast + auto-recovery (up to 3 restart attempts) + persistent popup on final failure.
- Backend timeout: 45-second grace period before showing retry button.
- Connection mode errors: JWT expiry → automatic fallback to local mode + sign-in prompt.

---

## 19. Dependencies (from package.json)

### Core Framework
- `react` 19.2.8, `react-dom` 19.2.8, `react-router-dom` 7.9.1
- TypeScript 6.0.2 (via `@typescript/native`), Vite 7.3.2

### PDF Libraries
- `@embedpdf/core` 2.14.4, `@embedpdf/engines` 2.14.4, `@embedpdf/models` 2.14.4
- `@embedpdf/plugin-*` 2.14.4 (16 plugins)
- `pdfjs-dist` 5.4.149 (secondary engine for metadata/thumbnails)
- `@cantoo/pdf-lib` 2.5.3

### UI Libraries
- `@mantine/core` 8.3.1, `@mantine/dates` 8.3.1, `@mantine/dropzone` 8.3.1, `@mantine/hooks` 8.3.1
- `@mui/material` 9.0.0, `@mui/icons-material` 9.2.0
- `@emotion/react` 11.14.0, `@emotion/styled` 11.14.1
- `lucide-react` 1.31.0, `@iconify/react` 6.0.2
- `tailwindcss` 4.1.13, `@tailwindcss/postcss` 4.1.13

### Desktop
- `@tauri-apps/api` 2.10.1, `@tauri-apps/cli` 2.9.6
- `@tauri-apps/plugin-dialog` 2.7.0, `@tauri-apps/plugin-fs` 2.5.0, `@tauri-apps/plugin-http` 2.5.7, `@tauri-apps/plugin-notification` 2.3.3, `@tauri-apps/plugin-shell` 2.3.5

### Data/State
- `@tanstack/react-query` 5.101.4, `@tanstack/react-virtual` 3.14.9
- `axios` 1.15.0
- `zustand` (not listed — state is all React Context)

### Utilities
- `i18next` 25.10.10, `react-i18next` 16.6.6, `i18next-browser-languagedetector` 8.2.0
- `jszip` 3.10.1, `d3` 7.9.0, `recharts` 3.7.0
- `react-rnd` 10.5.2 (draggable/resizable), `react-easy-crop` 5.5.6
- `signature_pad` 5.0.4, `pixelmatch` 7.1.0
- `@atlaskit/pragmatic-drag-and-drop` 1.7.7, `@dnd-kit/core` 6.3.1
- `qrcode.react` 4.2.0, `react-markdown` 9.0.3, `remark-gfm` 4.0.1

### Analytics/Billing
- `@posthog/react` 1.8.2, `posthog-js` 1.268.0
- `@stripe/react-stripe-js` 4.0.2, `@stripe/stripe-js` 7.9.0
- `@supabase/supabase-js` 2.47.13
- `peerjs` 1.5.5 (P2P real-time collaboration)

### Testing/Dev
- `vitest` 3.2.4, `@testing-library/react` 16.3.0, `playwright` 1.55.0, `puppeteer` 24.25.0
- `storybook` 9.1.20, `msw` 2.14.6

---

## 20. Performance Patterns

### WASM Precompilation

- Pdfium WASM module is eagerly compiled via `WebAssembly.compileStreaming()` during idle time.
- The compiled module is stored as a promise and used when the viewer initializes.

### Tile-Based Rendering

- `TilingPluginPackage` with 768px tiles and 5px overlap enables efficient rendering of large PDFs.
- Only visible tiles are rendered (virtualization via ViewportPlugin).
- Extra rings (1) pre-render tiles slightly outside the viewport for smooth scrolling.

### Virtual Scrolling

- `@tanstack/react-virtual` is available for list virtualization (thumbnails, file lists, etc.).
- The EmbedPDF Scroller handles PDF page virtualization internally.

### Worker Pool

- `PDFWorkerManager` limits concurrent PDF.js workers to 10.
- Queue-based waiting prevents resource exhaustion.

### Memoization and Refs

- `useMemo` with empty deps for stable selectors and callbacks.
- `useRef` for File objects (avoiding React state), bridge registries, and lifecycle managers.
- Split context pattern (state/actions) minimizes re-renders.

### Lazy Loading

- React `Suspense` wrapping the entire app.
- Route-level code splitting (implied by React Router usage).
- `@embedpdf/core` dynamically loads the Pdfium WASM engine.

### Connection Mode Caching

- `endpointAvailabilityService.preloadEndpoints()` pre-caches tool availability for 10 common endpoints.
- Avoids per-tool capability checks on first use.

### PDF.js Configuration

- `disableAutoFetch: true`, `disableStream: true` — prevents loading the entire PDF upfront.
- `isEvalSupported: false` — suppresses non-critical warnings.
- `stopAtErrors: false` — continues rendering even with page errors.

---

## 21. Architecture Summary

```
┌─────────────────────────────────────────────────────────────────┐
│                        index.html                               │
│  (Blocking theme script, error-catcher.js)                      │
└──────────────────────┬──────────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────────┐
│                     src/index.tsx                                │
│  (Mantine CSS, i18n, WASM precompilation, React root)           │
└──────────────────────┬──────────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────────┐
│            App.tsx (proprietary)                                 │
│  Routes: /workflow/sign/:token → ParticipantView               │
│          * → AppProviders → AppLayout → Workbench                │
└──────────────────────┬──────────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────────┐
│                  AppProviders                                    │
│  19 nested context providers (see §2)                           │
└──────────────────────┬──────────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────────┐
│                   Workbench                                      │
│  ┌──────────────┐  ┌──────────────────────────────────────────┐  │
│  │   Sidebar    │  │              Main Area                    │  │
│  │  (Tools,     │  │  ┌──────────────────────────────────┐    │  │
│  │   Quick      │  │  │         ViewerShell               │    │  │
│  │   Access)    │  │  │  ┌─────┐ ┌──────────┐ ┌────────┐ │    │  │
│  │              │  │  │  │Tabs │ │ Toolbar  │ │  Left  │ │    │  │
│  │              │  │  │  └─────┘ └──────────┘ │  Rail  │ │    │  │
│  │              │  │  │  ┌──────────────────┐ │        │ │    │  │
│  │              │  │  │  │   Viewer Center  │←│        │ │    │  │
│  │              │  │  │  │  (EmbedPDF +    │ │        │ │    │  │
│  │              │  │  │  │   Overlays)     │ │        │ │    │  │
│  │              │  │  │  └──────────────────┘ └────────┘ │    │  │
│  │              │  │  └──────────────────────────────────┘    │  │
│  └──────────────┘  └──────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### Key Design Decisions

1. **EmbedPDF over raw PDF.js**: The viewer uses a commercial plugin architecture (`@embedpdf/*`) with Pdfium WASM for rendering, not PDF.js directly. PDF.js is only used for metadata and thumbnails.

2. **Bridge pattern for viewer state**: Instead of React state for high-frequency viewer events (scroll, zoom), a ref-based bridge registry with immediate notifiers avoids unnecessary re-renders.

3. **File chaining via consume/undo**: Tools can be chained (split → merge → compress) without re-uploading. The FileContext tracks parent-child stub relationships for undo.

4. **Desktop-first with web fallback**: The app is primarily a Tauri desktop app with a bundled Java backend. Web mode exists but the architecture is optimized for local execution.

5. **Triple-path resolution** (`@app/*`): Desktop, proprietary (licensing/SaaS), and core (open-source) code layers are resolved via TypeScript path aliases, enabling clean separation of concerns.

6. **No external state library**: All state is managed through React Context + useReducer. No Redux, Zustand, or MobX. This keeps the dependency footprint small but means context nesting is deep (19 providers).

7. **Separate UI frameworks coexist**: Mantine (primary), MUI (secondary), and Tailwind CSS are all used. This suggests an evolving codebase with multiple design system migrations.
