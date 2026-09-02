# PDF-Elite / Stirling-PDF: PDFium & Engine Deep Analysis

## 1. Executive Summary

PDF-Elite is a fork/extension of [Stirling-PDF](https://github.com/Stirling-Tools/Stirling-PDF), a locally-hosted, web-based PDF manipulation tool. The codebase is a **monorepo** with three main components:

| Component | Language | Purpose |
|-----------|----------|---------|
| **Backend** (app/) | Java 25 / Spring Boot 4.0.6 | PDF processing API server |
| **Frontend** (frontend/) | React 19 / TypeScript / Vite 7 | Browser-based PDF viewer & editor SPA |
| **AI Engine** (engine/) | Python 3.13 / FastAPI | AI reasoning/orchestration service |

**Key finding:** PDFium is used at **two distinct layers** — a **custom Java wrapper (`com.stirling:jpdfium:1.0.2`)** on the backend and a **WASM-based NPM package ecosystem (`@embedpdf/* ^2.14.4`)** on the frontend. They are **independent implementations** — the Java JPDFium is not directly called from the frontend, and the frontend EmbedPDF WASM is not used on the backend.

---

## 2. PDFium Version and Build Configuration

### 2.1 Java/JPDFium (Backend)

- **Artifact:** `com.stirling:jpdfium:1.0.2`
- **Group ID:** `com.stirling` (custom Stirling/PDF-Elite namespace, **not** a public Maven artifact)
- **Repository:** Published to a private Maven repo at `${MAVEN_PUBLIC_URL}/releases` (env-var-configured). Falls back to Maven Central when not set.
- **Scope:** `api` in the `:common` module (`app/common/build.gradle` line 44), meaning all downstream modules (:core, :proprietary, :saas) have compile-time access.
- **Platform natives:** Separate artifacts per OS/arch, all at version 1.0.2:
  - `com.stirling:jpdfium-natives-linux-x64:1.0.2`
  - `com.stirling:jpdfium-natives-linux-arm64:1.0.2`
  - `com.stirling:jpdfium-natives-darwin-x64:1.0.2`
  - `com.stirling:jpdfium-natives-darwin-arm64:1.0.2`
  - `com.stirling:jpdfium-natives-windows-x64:1.0.2`
- **Docker build:** Only the matching platform native is bundled: `-PjpdfiumPlatforms=linux-arm64` or `linux-x64` based on `TARGETARCH` (see Dockerfile line 51).
- **Configuration property:** `-PjpdfiumPlatforms=all|<csv>` (default: `all` for local builds). Invalid platforms cause a `GradleException`.

### 2.2 EmbedPDF / PDFium WASM (Frontend)

- **Package scope:** `@embedpdf/* ^2.14.4` — a comprehensive plugin-based PDFium WASM viewer framework.
- **No private registry required** — these are public NPM packages (not in package-lock.json at root level, but resolved from the npm registry).
- **Version:** All `@embedpdf` packages are pinned to `^2.14.4` (same version across all plugins).

### 2.3 Static Resource Directory

The `generatedFrontendPaths` in `app/core/build.gradle` includes `'pdfium'` and `'pdfjs'` in the list of paths copied from the frontend build to `src/main/resources/static/`. This means the **PDFium WASM binaries are served as static assets** from the Spring Boot application at runtime.

---

## 3. All PDFium API Usage

### 3.1 Java/JPDFium Usage (Backend)

**Critical finding:** Despite being declared as a dependency in `:common`, **no Java source files in the publicly visible repository directly import or reference JPDFium/PDFium classes.** All examined files (
`PdfJsonConversionService.java`, `PdfJsonCosMapper.java`, `PdfJsonFallbackFontService.java`, `PdfJsonFontService.java`, `PdfJsonImageService.java`, `PdfJsonMetadataService.java`, `PdfLazyLoadingService.java`, `PdfMetricsService.java`, `WorkspaceManager.java`, `SPDFApplication.java`, `ReplaceAndInvertColorService.java`) use **Apache PDFBox 3.0.7** for all PDF operations.

**Possible explanations:**
1. The JPDFium usage is in **proprietary-only code** within `app/proprietary/src/main/java/stirling/software/proprietary/` (which we couldn't fully enumerate — the directory was accessible but returned empty).
2. It may be used for **rendering thumbnails** or **page previews** in a service not yet visible in the tree (the `service/misc` directory only contained `ReplaceAndInvertColorService.java`).
3. It could be used via **reflection** or a **dynamic factory** that resolves at runtime.
4. The `CustomPDFDocumentFactory.java` mentioned in the task was **not found** — it either doesn't exist in this fork or was renamed.

### 3.2 Frontend @embedpdf Usage (Browser)

The `@embedpdf` package ecosystem is **extensively used** as the primary PDF rendering and interaction engine:

**Installed packages (all `^2.14.4`):**

| Package | Purpose |
|---------|---------|
| `@embedpdf/core` | Core PDFium WASM engine bindings |
| `@embedpdf/engines` | Rendering engine abstraction layer |
| `@embedpdf/models` | TypeScript type definitions |
| `@embedpdf/plugin-annotation` | Annotation creation/highlight/underline/strikeout/notes |
| `@embedpdf/plugin-attachment` | File attachment management |
| `@embedpdf/plugin-bookmark` | PDF outline/bookmark navigation |
| `@embedpdf/plugin-document-manager` | Document lifecycle management |
| `@embedpdf/plugin-export` | PDF export/save functionality |
| `@embedpdf/plugin-history` | Undo/redo history stack |
| `@embedpdf/plugin-interaction-manager` | Tool/mode switching and interaction state |
| `@embedpdf/plugin-pan` | Pan/drag navigation |
| `@embedpdf/plugin-print` | Print support |
| `@embedpdf/plugin-redaction` | Redaction tool |
| `@embedpdf/plugin-render` | Page rendering pipeline |
| `@embedpdf/plugin-rotate` | Page rotation |
| `@embedpdf/plugin-scroll` | Scroll management |
| `@embedpdf/plugin-search` | Text search |
| `@embedpdf/plugin-selection` | Text selection layer |
| `@embedpdf/plugin-spread` | Single/dual page spread modes |
| `@embedpdf/plugin-thumbnail` | Thumbnail generation |
| `@embedpdf/plugin-tiling` | Tile-based rendering for large pages |
| `@embedpdf/plugin-viewport` | Viewport/zoom management |
| `@embedpdf/plugin-zoom` | Zoom control |

**Key frontend components using EmbedPDF:**
- `components/viewer/LocalEmbedPDF.tsx` — Primary EmbedPDF wrapper, hosts the `<Viewport>` (the single scroll owner)
- `components/viewer/EmbedPdfViewer.tsx` — Top-level viewer component
- `components/viewer/Viewer.tsx` — Routing layer
- `components/viewer/TextSelectionMenu.tsx` — Uses embedpdf `<SelectionLayer>`
- `workbench/viewer/ViewerShell.tsx` — Passes `EmbedPdfViewer` as children
- `workbench/viewer/ContextualToolbar.tsx` — Annotation/edit tool controls
- `workbench/viewer/OrganizeMode.tsx` — Page reorganization (uses PDF.js, not EmbedPDF, for thumbnails)

**EmbedPDF capabilities exercised:**
- Rendering: `<Viewport>` with `overflow:auto` as the scroll owner
- Text selection: `<SelectionLayer>` + `textSelection: true`
- Annotations: HIGHLIGHT, UNDERLINE, STRIKEOUT, FREETEXT (sticky notes), stamp annotations
- Bookmarks: `PdfBookmarkObject` tree, `CommentsSidebar`
- Zoom: `setZoomLevel`, `getZoomState`, pinch/ctrl+wheel via `useWheelZoom`
- Scroll: `scrollActions.scrollToPage()`, `getScrollState().totalPages`
- Document position persistence: localStorage key `pdf-elite:pos:${name}|${size}|${lastModified}`
- Tool registration: `ensureTool()`, `activateAnnotationTool()`, `setAnnotationStyle()`
- Highlight color: `pdf-elite:highlight-color` localStorage persistence

---

## 4. Rendering Pipeline Details

### 4.1 Frontend Rendering Architecture

The app uses **two independent PDF rendering engines**:

1. **EmbedPDF (PDFium WASM)** — Primary viewer for the main document:
   - Used for all interactive viewing, annotation, editing
   - Plugin-based architecture: render, viewport, zoom, scroll, selection, etc.
   - WASM-based, runs entirely in the browser
   - The `<Viewport>` component is the single scroll owner (`overflow:auto`); all parent containers clip (`overflow:hidden`)

2. **PDF.js (`pdfjs-dist ^5.4.149`)** — Secondary/thumbnail renderer:
   - Used for thumbnail generation (via Web Workers)
   - Used for page previews in Organize mode (outside the EmbedPDF provider tree)
   - Used for the compare view (`useProgressivePagePreviews` hook)
   - **Not** the primary viewing engine — that role belongs to EmbedPDF

### 4.2 Rendering Flow

```
HomePage.tsx
  → Viewer.tsx
    → ViewerShell.tsx (passes EmbedPdfViewer as children)
      → EmbedPdfViewer (registers tools, providers)
        → @embedpdf <Viewport> (PDFium WASM render)
        → @embedpdf <SelectionLayer> (text selection)
        → ContextualToolbar (annotation/edit controls)
```

### 4.3 Backend Rendering

The backend serves pre-rendered **static assets** from `src/main/resources/static/`:
- `static/pdfium/` — PDFium WASM binaries (served to the browser)
- `static/pdfjs/` — PDF.js worker files
- `static/fonts/*.ttf` — System fonts for PDF rendering
- Fonts are copied to `/usr/share/fonts/truetype/` in Docker and `fc-cache -f` is run

---

## 5. PDF-JSON Conversion Pipeline

### 5.1 Java Backend Pipeline

The PDF→JSON conversion is implemented in `PdfJsonConversionService.java` (a very large file, ~2500+ lines). It uses **Apache PDFBox exclusively** (no JPDFium):

- **Input:** PDF file (InputStream or Path)
- **Output:** Structured JSON representation of the PDF
- **Engine:** `PDFGraphicsStreamEngine` (PDFBox's content stream processor)
- **Supporting services:**
  - `PdfJsonCosMapper.java` — Maps PDFBox COS (PDF object) structures to JSON
  - `PdfJsonFallbackFontService.java` — Handles fonts that can't be resolved, assigns `FALLBACK_FONT_ID`
  - `PdfJsonFontService.java` (in `service/pdfjson/`) — Font metadata extraction
  - `PdfJsonImageService.java` — Image extraction and Base64 encoding
  - `PdfJsonMetadataService.java` — PDF metadata (title, author, dates, etc.)
  - `PdfLazyLoadingService.java` — Lazy loading of page resources
  - `JobOwnershipServiceImpl.java` — Job ownership tracking

### 5.2 Conversion Process

1. Load PDF with PDFBox `PDDocument.load()`
2. Extract document metadata (title, author, creation/mod dates, page count)
3. For each page, use `PDFGraphicsStreamEngine` to walk the content stream
4. Extract text, fonts, images, paths, and annotations
5. Map COS structures to typed JSON via `PdfJsonCosMapper`
6. Resolve fonts via `PdfJsonFontService`, falling back to `PdfJsonFallbackFontService`
7. Encode images as Base64
8. Handle Type 3 fonts (in `service/pdfjson/type3/` subdirectory)
9. Output structured JSON suitable for frontend rendering

### 5.3 Relationship to EmbedPDF

The PDF-JSON pipeline provides a **structured representation** of the PDF for the AI engine and backend analysis. The frontend EmbedPDF viewer handles its own rendering from the raw PDF binary — it does **not** consume the JSON output. The JSON is used by:
- The **AI Engine** (Python) for document understanding
- Backend services that need to inspect PDF structure without rendering

---

## 6. Python AI Engine Architecture

### 6.1 Overview

- **Entry point:** `stirling.api.app:app` (FastAPI)
- **Runtime:** Uvicorn with 4 workers (configurable via `STIRLING_ENGINE_WORKERS`)
- **Python version:** 3.13.8 (managed by `uv`)
- **Port:** 5001
- **Build tool:** `hatchling`

### 6.2 Package Structure

```
engine/src/stirling/
├── api/                          # HTTP layer
│   ├── app.py                   # FastAPI app, lifespan, middleware
│   ├── bootstrap.py             # App state construction
│   ├── dependencies.py          # DI providers (agents, services)
│   ├── engine_auth.py           # Shared secret middleware
│   ├── middleware.py             # UserId extraction middleware
│   └── routes/                  # Route modules
│       ├── orchestrator.py      # /api/v1/orchestrator (streaming NDJSON)
│       ├── pdf_edit.py          # /api/v1/pdf/edit
│       ├── pdf_question.py      # /api/v1/pdf/question
│       ├── config.py            # /api/v1/config (push/live config)
│       ├── agent_draft.py       # Agent draft endpoints
│       ├── execution.py         # Execution tracking
│       ├── document.py          # Document management
│       ├── ledger.py            # Operation ledger
│       ├── pdf_comments.py      # PDF comments
│       ├── agent_capabilities.py # Agent capability discovery
│       └── document_classifier.py # Document classification
├── agents/                      # AI reasoning modules
│   ├── OrchestratorAgent        # Main workflow planner
│   └── PdfEditAgent             # PDF editing agent
├── config/                      # Settings & config management
│   └── config_cache.py          # Shared cache for multi-worker config
├── contracts/                   # Request/response Pydantic models
├── documents/                   # Document persistence (SQLite + pgvector)
│   └── EmbeddingService         # Vector embeddings
├── models/                      # Shared model primitives
│   ├── tool_models.py           # Generated from Java OpenAPI spec
│   └── tool_io.py               # Generated tool I/O types
├── services/                    # Shared infrastructure
│   ├── runtime.py               # AppRuntime, model construction
│   ├── progress.py              # Progress emission
│   ├── tool_io_compat.py        # Tool chain validation
│   └── tracking.py              # PostHog analytics
└── __init__.py
```

### 6.3 Key Architecture Decisions

1. **The engine does NOT execute PDF operations directly.** It plans and interprets work; the Java backend executes the actual operations. The frontend calls the Python engine via Java as a proxy.
2. **Streaming NDJSON** from the orchestrator endpoint: progress events, result events, error events, and heartbeat events (every 10s) to keep connections alive.
3. **Typed contracts in, typed contracts out** — all communication uses Pydantic models.
4. **Config push system:** Supports live model switching across workers via a shared encrypted cache file (`config_cache.py`). A config cache watcher polls for changes.
5. **Document reaper:** Background task purges expired documents on an interval.
6. **AI framework:** `pydantic-ai` (1.99.x, pre-2.0) with structured outputs (`NativeOutput`).
7. **Multi-model support:** Configurable `smart_model` (complex reasoning) and `fast_model` (quick tasks). Works with any AI provider that supports structured outputs, including self-hosted models.
8. **Tool models are generated** from the Java OpenAPI spec (`SwaggerDoc.json`) via `scripts/generate_tool_models.py`.

### 6.4 Dependencies

| Dependency | Version | Purpose |
|------------|---------|---------|
| `fastapi` | >=0.116.0 | HTTP framework |
| `pydantic` | >=2.0.0 | Data validation |
| `pydantic-ai` | >=1.99.0,<2.0.0 | AI agent framework |
| `pydantic-ai-slim[voyageai]` | >=1.99.0,<2.0.0 | AI with VoyageAI embeddings |
| `pydantic-settings` | >=2.0.0 | Settings management |
| `uvicorn` | >=0.35.0 | ASGI server |
| `pgvector` | >=0.3.6 | PostgreSQL vector extension |
| `psycopg[binary,pool]` | >=3.2 | PostgreSQL driver |
| `sqlite-vec` | >=0.1.6 | SQLite vector extension (local dev) |
| `cryptography` | >=44.0.0 | Config encryption |
| `opentelemetry-sdk` | >=1.39.0 | Tracing/observability |
| `posthog` | >=3.0.0 | Product analytics |
| `python-dotenv` | >=1.2.1 | .env file loading |

### 6.5 AI Agent Routers (API)

| Router | Prefix | Purpose |
|--------|--------|---------|
| `orchestrator` | `/api/v1/orchestrator` | Main planning/streaming workflow |
| `pdf_edit` | `/api/v1/pdf/edit` | PDF editing operations |
| `pdf_question` | `/api/v1/pdf/question` | Q&A about PDF content |
| `agent_draft` | (tbd) | Draft agent responses |
| `execution` | (tbd) | Execution tracking |
| `document` | (tbd) | Document CRUD |
| `ledger` | (tbd) | Operation ledger |
| `pdf_comments` | (tbd) | PDF comment management |
| `agent_capabilities` | (tbd) | Agent capability discovery |
| `document_classifier` | (tbd) | Auto-classify documents |
| `config` | (tbd) | Push/live configuration |

All routers except `config` and `/health` require user identity via `X-User-Id` header.

---

## 7. Docker Build Process and Image Sizes

### 7.1 Embedded Dockerfile (`docker/embedded/Dockerfile`)

**Three-stage build:**

1. **Stage 1: `app-build`** (gradle:9.6.1-jdk25)
   - Installs Node.js 22.x and Task CLI
   - Builds Java + frontend with Gradle
   - JPDFium platform is set based on `TARGETARCH` (arm64 → linux-arm64, else linux-x64)
   - Build flags: `-PbuildWithFrontend=true -PjpdfiumPlatforms=$JPDFIUM_PLATFORM`

2. **Stage 2: `jar-extract`** (eclipse-temurin:25-jre-noble)
   - Extracts Spring Boot layered JAR

3. **Stage 3: Runtime** (stirlingtools/stirling-pdf-base:1.0.2)
   - Pre-built base image with LibreOffice, Calibre, Tesseract, OCRmyPDF
   - Copies application layers with `--link` for efficient layer caching
   - Installs fonts and runs `fc-cache -f`
   - Creates symlinks for `/logs`, `/configs`, `/customFiles`, `/pipeline`, `/storage`
   - JVM profiles: `balanced` (G1GC) and `performance` (ShenandoahGC generational)
   - Enables virtual threads (`-Dspring.threads.virtual.enabled=true`)
   - Health check: `curl` to `/api/v1/info/status` every 30s
   - Entrypoint: `tini -- /scripts/init.sh`

### 7.2 AI Engine Dockerfile (`engine/Dockerfile`)

- **Base:** `ghcr.io/astral-sh/uv:python3.13-bookworm-slim`
- Installs Task CLI, copies source, runs `uv sync --frozen --no-dev`
- Runs as `task engine:run` (4 Uvicorn workers)
- Port: 5001

### 7.3 Image Sizes

- **Base image** (`stirlingtools/stirling-pdf-base:1.0.2`): Pre-built, contains LibreOffice, Calibre, Tesseract, OCRmyPDF — likely 2-4 GB
- **Final embedded image**: Base + Spring Boot app layers (~200-400 MB) + JPDFium natives (~50-100 MB per platform)
- **Engine image**: Slim Python 3.13 + dependencies — likely 200-400 MB

---

## 8. CI/CD Pipeline

### 8.1 Workflow Architecture

The `build.yml` workflow is a **routing layer** that detects changed paths and dispatches to dedicated reusable workflows:

| Workflow | Trigger | Purpose |
|----------|---------|---------|
| `backend-build.yml` | `project` changes | Java build + test |
| `ai-engine.yml` | `engine` changes | Python quality gate (lint, typecheck, test) |
| `frontend-validation.yml` | `frontend` changes | TypeScript lint, build, test |
| `frontend-a11y.yml` | `frontend` changes | Accessibility audit (advisory, non-blocking) |
| `e2e-stubbed.yml` | `frontend` changes | Playwright E2E with stubs |
| `e2e-live.yml` | `frontend` changes | Playwright E2E against real backend |
| `docker-compose-tests.yml` | `project` changes | Docker integration tests |
| `test-build-docker.yml` | `project` + Dockerfile changes | Full Docker image build test |
| `tauri-build.yml` | `tauri` changes | Desktop app build (macOS + Windows) |
| `check-licence.yml` | `build` changes | License compliance |
| `check-openapi.yml` | `openapi` changes | OpenAPI spec freshness |
| `check-generated-models.yml` | `generated-models` changes | Verify tool_models.py is up-to-date |
| `db-migration-test.yml` | `project` changes | H2 schema migration against v2.0.0/v2.5.0/v2.10.0 fixtures |
| `push-docker.yml` | Push to main | Build and push Docker images |
| `build-enterprise.yml` | `proprietary` changes | Enterprise/proprietary features |
| `nightly.yml` | Schedule | Nightly builds with full OS matrix |

### 8.2 AI Engine CI (`ai-engine.yml`)

- Runs `task engine:check` (typecheck + lint + format-check + test)
- Uses `uv` with caching
- Posts PR comments on failure with fix instructions (`task engine:fix`)
- Removes comments on success

### 8.3 Branch Protection

- The `all-checks-passed` job is the **single status check** for branch protection
- Concurrency: Cancels in-progress jobs on new pushes to the same branch
- GHA hardening via `step-security/harden-runner` with egress audit

### 8.4 Docker Deployment

- `push-docker.yml`: Builds and pushes on main branch push
- `PR-Auto-Deploy-V2.yml`: Auto-deploys PR previews
- `deploy-on-v2-commit.yml`: Deploys on v2 commits
- Base image: `stirlingtools/stirling-pdf-base:1.0.2` (pre-built, pinned by SHA256)

---

## 9. Testing Infrastructure

### 9.1 Test Types

| Type | Tool | Location |
|------|------|----------|
| **Backend tests** | JUnit 5, ArchUnit | `app/core/src/test/`, `app/common/src/test/` |
| **Frontend tests** | Vitest, Playwright | `frontend/editor/` (vitest.config.ts) |
| **Engine tests** | pytest | `engine/tests/` |
| **E2E tests** | Playwright | Stubbed + live modes |
| **Docker integration** | Bash (`testing/test.sh`) | `testing/` |
| **Cucumber tests** | Cucumber | `testing/cucumber/` |
| **DB migration tests** | H2 fixtures | `.github/workflows/db-migration-test.yml` |
| **Accessibility** | Playwright a11y | `.github/workflows/frontend-a11y.yml` |

### 9.2 Testing Script (`testing/test.sh`)

- Finds project root by locating `build.gradle`
- Supports `--rerun-failed` and `--rerun "test1,test2"` modes
- Captures failure logs from Docker containers
- GHA-compatible output with `::group::` markers
- Reports stored in `testing/reports/`
- Tests multiple Docker variants (ultra-lite, standard, fat, security)

### 9.3 Quality Gate

- `task check` runs backend:check + frontend:check + engine:check
- `task check:all` adds frontend:check:all (full CI quality gate)
- Backend: Spotless (Google Java Format), ArchTest, SonarQube
- Frontend: ESLint, Stylelint, TypeScript strict, Vitest
- Engine: Ruff lint, Ruff format, Pyright typecheck, pytest

---

## 10. External Tool Dependencies and Detection

### 10.1 Backend External Tools

| Tool | Purpose | Detection |
|------|---------|-----------|
| **LibreOffice** | Document conversion (DOCX→PDF, etc.) | Docker base image, `SAL_TMP` env var |
| **qpdf** | PDF optimization | Referenced in docs |
| **tesseract** | OCR | Docker base image (full variant) |
| **OCRmyPDF** | OCR-processed PDF generation | Docker base image (full variant) |
| **Calibre** | E-book conversion | Docker base image (full variant) |
| **Java 25** | Runtime | Toolchain via foojay-resolver plugin |

### 10.2 Frontend External Dependencies

| Tool | Purpose |
|------|---------|
| **@embedpdf (PDFium WASM)** | Primary PDF viewer/renderer/editor |
| **pdfjs-dist ^5.4.149** | Secondary renderer (thumbnails, organize mode) |
| **@cantoo/pdf-lib ^2.5.3** | Client-side PDF manipulation |
| **React 19.2.8** | UI framework |
| **Mantine 8.3.1** | Component library |
| **TailwindCSS 4.1.13** | Styling |
| **Tauri 2.x** | Desktop app shell |
| **Vite 7.3.2** | Build tool |
| **Stripe** | Payment processing (SaaS) |
| **Supabase** | Backend-as-a-Service (SaaS) |
| **PostHog** | Analytics |
| **PeerJS** | Real-time collaboration |

---

## 11. Complete Dependency Tree

### 11.1 Java Backend

```
:stirling-pdf (app/core)
├── :common (app/common)
│   ├── org.apache.pdfbox:pdfbox:3.0.7
│   ├── org.apache.pdfbox:pdfbox-io:3.0.7
│   ├── org.apache.pdfbox:xmpbox:3.0.7
│   ├── org.apache.pdfbox:preflight:3.0.7
│   ├── com.stirling:jpdfium:1.0.2  ← CUSTOM PDFium wrapper
│   ├── com.stirling:jpdfium-natives-{platform}:1.0.2  ← per-arch native
│   ├── com.google.guava:guava:33.6.0-jre
│   ├── org.springframework.boot:spring-boot-starter-webmvc
│   ├── technology.tabula:1.0.5
│   ├── com.github.junrar:7.6.0
│   ├── org.simplejavamail:8.12.6
│   └── [other common libs]
├── [if proprietary] :proprietary (app/proprietary)
│   ├── :common (transitive)
│   ├── Spring Security + SAML
│   ├── Spring Data JPA
│   ├── Caffeine cache
│   ├── Bucket4j rate limiting
│   ├── Jinjava templating
│   └── Redis (optional)
├── [if saas] :saas (app/saas)
│   └── :proprietary (transitive)
├── org.springframework.boot:spring-boot-starter-jetty (with HTTP/2 + ALPN)
├── org.bouncycastle:bcprov-jdk18on:1.78
├── org.bouncycastle:bcpkix-jdk18on:1.78
├── org.verapdf:validation-model:1.30.2
├── org.apache.xmlgraphics:batik-bridge:1.19 (SVG)
├── com.twelvemonkeys.imageio:*:3.13.1 (image formats)
├── org.apache.poi:poi-ooxml:5.5.1
├── com.google.zxing:core:3.5.4
├── com.opencsv:opencsv:5.12.0
├── org.commonmark:commonmark:0.28.0
└── [build tools: Spotless, SonarQube, SwaggerHub]
```

### 11.2 Frontend

```
frontend/package.json
├── @embedpdf/* ^2.14.4 (22 packages — PDFium WASM framework)
├── pdfjs-dist ^5.4.149 (PDF.js secondary renderer)
├── @cantoo/pdf-lib ^2.5.3
├── react ^19.2.8 / react-dom ^19.2.8
├── @mantine/core ^8.3.1 + mantine-dates, dropzone, hooks
├── @mui/material ^9.0.0 + icons
├── @tailwindcss/postcss ^4.1.13
├── @tanstack/react-query ^5.101.4
├── react-router-dom ^7.9.1
├── i18next ^25.10.10 + react-i18next
├── axios ^1.15.0
├── jszip ^3.10.1
├── signature_pad ^5.0.4
├── @tauri-apps/api ^2.10.1 + plugins (desktop)
├── @stripe/react-stripe-js ^4.0.2 (SaaS)
├── @supabase/supabase-js ^2.47.13 (SaaS)
├── posthog-js ^1.268.0
├── recharts ^3.7.0
├── d3 ^7.9.0
├── [dev: Vite 7, TypeScript 7, Playwright, Storybook 9, Puppeteer, Vitest 3]
└── [dev: ESLint 10, Prettier 3, Stylelint 17, Ruff]
```

### 11.3 Python AI Engine

```
engine/pyproject.toml
├── fastapi >=0.116.0
├── pydantic >=2.0.0
├── pydantic-ai >=1.99.0,<2.0.0
├── pydantic-ai-slim[voyageai] >=1.99.0,<2.0.0
├── pydantic-settings >=2.0.0
├── uvicorn >=0.35.0
├── pgvector >=0.3.6
├── psycopg[binary,pool] >=3.2
├── sqlite-vec >=0.1.6
├── cryptography >=44.0.0
├── opentelemetry-sdk >=1.39.0
├── posthog >=3.0.0
├── python-dotenv >=1.2.1
└── [dev: pytest, pyright, ruff, datamodel-code-generator]
```

---

## 12. Key Findings and Observations

### 12.1 JPDFium Is a Proprietary Custom Artifact

The `com.stirling:jpdfium:1.0.2` is **not** a public Maven Central artifact. It's published under the Stirling `com.stirling` group ID to a private Maven repository. This is likely a JNA-based Java wrapper around PDFium native libraries, custom-built for the Stirling-PDF/PDF-Elite project. No source code for this wrapper was found in the public repository.

### 12.2 PDFium Usage Is Primarily Frontend

The primary PDFium usage is on the **frontend** via the `@embedpdf` WASM packages, which provide a comprehensive PDF viewing, editing, and annotation experience. The backend JPDFium dependency exists but its actual usage is not visible in the public source code.

### 12.3 Dual Renderer Architecture

The frontend deliberately uses **two PDF renderers**: EmbedPDF (PDFium WASM) for the primary interactive viewer, and PDF.js for thumbnails and organize mode. This is because the OrganizeMode component lives **outside** the EmbedPDF provider tree and needs a renderer that doesn't require the EmbedPDF context.

### 12.4 AI Engine Is Purely a Reasoning Layer

The Python AI engine does **not** execute PDF operations. It:
- Plans operations (orchestrator agent)
- Provides PDF editing intelligence (pdf_edit agent)
- Answers questions about PDF content (pdf_question agent)
- Classifies documents
- Returns typed contracts that the Java backend executes

### 12.5 Bleeding-Edge Stack

The project uses post-2024 releases: Spring Boot 4.0.6, Jackson 3 (tools.jackson namespace), JDK 25, React 19, Vite 7, TypeScript 7. This is explicitly called out in CLAUDE.md as a risk for LLM-generated code.

### 12.6 Multi-Flavor Build System

Three build flavors via `STIRLING_FLAVOR`:
- **core**: Minimal PDF operations only
- **proprietary** (default): Full features including security
- **saas**: Cloud SaaS with Supabase + Stripe

Each flavor determines which Gradle modules are included and which frontend layer is built.

---

## 13. File Manifest (Files Analyzed)

| File | Path |
|------|------|
| Root build.gradle | `/build.gradle` |
| Core build.gradle | `/app/core/build.gradle` |
| Common build.gradle | `/app/common/build.gradle` |
| Proprietary build.gradle | `/app/proprietary/build.gradle` |
| Settings.gradle | `/settings.gradle` |
| Taskfile.yml | `/Taskfile.yml` |
| Engine Taskfile | `/.taskfiles/engine.yml` |
| Frontend package.json | `/frontend/package.json` |
| Engine pyproject.toml | `/engine/pyproject.toml` |
| Embedded Dockerfile | `/docker/embedded/Dockerfile` |
| Engine Dockerfile | `/engine/Dockerfile` |
| Engine Dockerfile.dev | `/engine/Dockerfile.dev` |
| SPDFApplication.java | `/app/core/.../SPDFApplication.java` |
| PdfJsonConversionService.java | `/app/core/.../service/PdfJsonConversionService.java` |
| PdfJsonCosMapper.java | `/app/core/.../service/PdfJsonCosMapper.java` |
| PdfJsonFallbackFontService.java | `/app/core/.../service/PdfJsonFallbackFontService.java` |
| PdfJsonFontService.java | `/app/core/.../service/pdfjson/PdfJsonFontService.java` |
| PdfJsonImageService.java | `/app/core/.../service/pdfjson/PdfJsonImageService.java` |
| PdfJsonMetadataService.java | `/app/core/.../service/pdfjson/PdfJsonMetadataService.java` |
| PdfLazyLoadingService.java | `/app/core/.../service/pdfjson/PdfLazyLoadingService.java` |
| PdfMetricsService.java | `/app/core/.../service/PdfMetricsService.java` |
| Engine app.py | `/engine/src/stirling/api/app.py` |
| Engine orchestrator.py | `/engine/src/stirling/api/routes/orchestrator.py` |
| Engine pdf_edit.py | `/engine/src/stirling/api/routes/pdf_edit.py` |
| Engine services/__init__.py | `/engine/src/stirling/services/__init__.py` |
| AI Engine CI | `/.github/workflows/ai-engine.yml` |
| Build CI | `/.github/workflows/build.yml` |
| testing/test.sh | `/testing/test.sh` |
| AGENTS.md | `/AGENTS.md` |
| DeveloperGuide.md | `/DeveloperGuide.md` |
| FUNCTIONALITY_INVENTORY.md | `/FUNCTIONALITY_INVENTORY.md` |
