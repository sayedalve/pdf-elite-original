# PDF Elite (Stirling PDF Fork) — Java Spring Boot Backend Analysis

**Version**: 2.14.2 | **Spring Boot**: 4.0.6 | **PDFBox**: 3.0.7 | **Java Target**: 25 | **Servlet Container**: Jetty (HTTP/2 + ALPN)

---

## 1. Project Structure & Module Architecture

The project is a **multi-module Gradle build** with three primary modules:

| Module | Purpose | Spring Boot? |
|--------|---------|-------------|
| `:common` | Shared library — PDF utilities, services, models, configuration | No (library-only) |
| `:proprietary` | Security, OAuth2, SAML2, database, AI agents, premium features | No (library-only) |
| `:saas` | SaaS-specific features (Supabase auth, multi-tenant) | No (library-only) |
| `:core` (alias `:stirling-pdf`) | Main application — controllers, PDF/JSON conversion service | **Yes** (bootJar) |

**Scan base packages**: `stirling.software.SPDF`, `stirling.software.common`, `stirling.software.proprietary`, `stirling.software.saas`

**Build profile detection** at startup (in `SPDFApplication.getActiveProfile()`):
- If `stirling.software.saas.security.SupabaseSecurityConfig` is on classpath → profiles `["security", "saas"]`
- If `stirling.software.proprietary.security.configuration.SecurityConfiguration` is on classpath → profiles `["security"]`
- Otherwise → profile `["default"]`

The `DISABLE_ADDITIONAL_FEATURES` env var / gradle property disables the `:proprietary` module (free/community builds).

---

## 2. API Controllers & Endpoints

All API controllers live under `stirling.software.SPDF.controller.api` and use the custom `@AutoJobPostMapping` annotation (replaces standard `@PostMapping`) for automatic job tracking, resource weight management, and OpenAPI metadata.

### 2.1 Core API Controllers (app/core)

| Controller | Path | Endpoints |
|-----------|------|----------|
| **MergeController** | `/api/v1/general/merge-pdfs` | `POST /merge-pdfs` — merge multiple PDFs with jpdfium, optional TOC generation, cert signature removal |
| **EditTextController** | — | Edit text content in PDFs |
| **EditTableOfContentsController** | — | Edit PDF table of contents/bookmarks |
| **SplitPDFController** | — | Split PDF by fixed page ranges |
| **SplitPdfByChaptersController** | — | Split PDF by chapter/bookmark structure |
| **SplitPdfBySectionsController** | — | Split PDF by section breaks |
| **SplitPdfBySizeController** | — | Split PDF by target file size |
| **RearrangePagesPDFController** | — | Rearrange/reorder PDF pages |
| **RotationController** | — | Rotate PDF pages |
| **DuplicatePagesController** | `/api/v1/page/` | Duplicate specific pages |
| **FilterController** | — | 6 filter operations (grayscale, invert, sepia, etc.) via `@AutoJobPostMapping` |
| **FormFillController** | `/api/v1/form` | `POST /fields`, `/fields-with-coordinates`, `/extract-csv`, `/extract-xlsx`, `/modify-fields`, `/delete-fields`, `/fill` |
| **ExtractImagesController** | — | Extract images from PDF |
| **AutoRotateController** | — | Auto-rotate pages based on content orientation |
| **AutoSplitPdfController** | — | Automatic PDF splitting (barcode/blank page detection) |
| **BlankPageController** | — | Detect/remove blank pages |
| **OverlayImageController** | — | Overlay images onto PDF pages |
| **PageNumbersController** | — | Add page numbers |
| **RemoveImagesController** | — | Remove images from PDF |
| **ReplaceImageController** | — | Replace images in PDF |
| **AddCommentsController** | — | Add annotations/comments |
| **LoginDisclaimerController** | — | Login disclaimer management |
| **GetInfoOnPDF** | — | Comprehensive PDF info extraction (single large endpoint, ~1100 lines) — page dimensions, fonts, image stats, form fields, metadata, annotations, bookmarks, signatures |
| **ValidateSignatureController** | — | PDF digital signature validation |
| **VerifyPDFController** | — | PDF/A compliance verification (veraPDF) |
| **AllTextLineExtractor** | — | Extract all text lines from PDF |

### 2.2 Web Controllers (app/core)

| Controller | Purpose |
|-----------|---------|
| **ReactRoutingController** | SPA fallback routing for the React frontend |
| **MetricsController** | Exposes Prometheus/micrometer metrics |
| **UploadLimitService** | Runtime upload limit management |
| **SignatureImageController** | Serves signature images |
| **RobotsController** | Serves robots.txt |

### 2.3 Proprietary API Controllers

| Controller | Purpose |
|-----------|---------|
| **SignatureController** | Digital signing of PDFs |
| **AdminJobController** | Job queue management |
| **AiEngineController** | AI-powered PDF analysis |
| **AuditDashboardController** | Security audit dashboard |
| **AuditRestController** | REST audit log access |
| **ClassifyLabelController** | Document classification/labeling |
| **CreatePdfAgentController** | AI PDF creation agent |
| **MathAuditorAgentController** | Mathematical content auditor |
| **PdfCommentAgentController** | AI PDF commenting agent |
| **PortalApiKeysController** | API key management |
| **PortalDocumentsController** | Document management portal |
| **PortalInfraAuditController** | Infrastructure audit |
| **ProprietaryUIDataController** | UI configuration data |
| **FleetUsageController** | Fleet/license usage tracking |
| **UsageRestController** | Usage analytics REST API |

---

## 3. PDF Processing Services

### 3.1 Core Services (app/core)

| Service | Description |
|---------|-------------|
| **PdfJsonConversionService** | **(~2000 lines)** The centerpiece — bidirectional PDF↔JSON conversion for the web-based PDF editor. Extracts fonts, text, images, annotations, form fields, metadata into JSON; reconstructs PDFs from modified JSON with full round-trip fidelity. |
| **PdfJsonCosMapper** | Maps PDFBox COS (PDF internal object) structures to/from JSON representation |
| **PdfJsonFallbackFontService** | Provides fallback fonts when original fonts can't be embedded |
| **PdfJsonFontService** | Font handling for PDF-JSON pipeline (subset info, CID system, embedding) |
| **PdfJsonMetadataService** | Metadata handling (title, author, subject, keywords, dates) |
| **PdfJsonImageService** | Image extraction and re-embedding for JSON round-trip |
| **PdfLazyLoadingService** | Lazy loading of page images for large PDFs in the editor |
| **JobOwnershipServiceImpl** | Async job ownership tracking for the editor pipeline |
| **NoOpJobOwnershipService** | No-op implementation when job tracking is disabled |
| **ReplaceAndInvertColorService** | Color replacement and inversion operations |
| **VeraPDFService** | PDF/A validation via veraPDF library |
| **CertificateValidationService** | X.509 certificate chain validation |
| **HardwareKeyStoreService** | Hardware keystore integration for signing |
| **PdfSigningServiceImpl** | PDF digital signing implementation |
| **SharedSignatureService** | Shared signature resources |
| **AttachmentService** | PDF attachment management (embedded files) |
| **ApiDocService** | API documentation generation |
| **LanguageService** | Multi-language/i18n support |
| **MetricsAggregatorService** | Metrics aggregation for monitoring |
| **PdfMetricsService** | PDF-specific metrics collection |
| **WeeklyActiveUsersService** | WAU tracking filter |

### 3.2 Common Services (app/common)

| Service | Description |
|---------|-------------|
| **CustomPDFDocumentFactory** | **Smart PDF loading** with 3-tier memory strategy: small files (≤10 MB) → in-memory; medium → mixed mode; large → file-backed. Uses semaphore-bounded concurrency (`MAX_CONCURRENT_OPS = CPU cores`). Heap-aware with 30% free memory threshold. |
| **TempFileManager** | Tracked temporary file creation with configurable base directory, prefix, max age cleanup. Registers all temp files in `TempFileRegistry`. |
| **TaskManager** | Async task/job management |
| **JobExecutorService** | Background job execution |
| **JobQueue** | Job queuing system |
| **PostHogService** | Product analytics via PostHog |
| **PdfMetadataService** | PDF metadata read/write |
| **PdfAnnotationService** | PDF annotation management |
| **PdfSigningService** | PDF signing interface |
| **FileStorage** | File storage abstraction |
| **FileOrUploadService** | File/upload parameter handling |
| **SsrfProtectionService** | SSRF attack prevention |
| **ToolChainValidator** | Validates tool dependencies for features |
| **ToolIORegistry** | Registry for tool input/output metadata |
| **ToolMetadataService** | Tool capability metadata |
| **MobileScannerService** | Mobile document scanning integration |
| **LineArtConversionService** | Line art vector conversion |
| **LoginAgreementService** | Login agreement/terms management |
| **PersonalSignatureServiceInterface** | Personal signature management |
| **ServerCertificateServiceInterface** | Server certificate management |
| **ResourceMonitor** | System resource monitoring |
| **InternalApiClient** | Internal API communication |
| **LicenseServiceInterface** | License management (proprietary) |
| **UserServiceInterface** | User management interface |
| **TempFileCleanupService** | Scheduled temp file cleanup |

### 3.3 Key Utility Classes (app/common/util)

- **ProcessExecutor** — Centralized external process execution with semaphore-based concurrency, per-tool timeout configuration, command validation (null bytes, newlines, path traversal), and LibreOffice UnoServer pool integration
- **PDFService** / **PDFToFile** / **FileToPdf** — PDF conversion utilities
- **PdfUtils** / **PdfErrorUtils** / **PdfTextLocator** — PDF-specific helpers
- **ImageProcessingUtils** — Image manipulation
- **UnoServerPool** — Connection pool for LibreOffice UnoServer instances
- **TempFile** / **TempFileRegistry** / **TempFileManager** — Temp file lifecycle management
- **WebResponseUtils** — Standardized HTTP response builders
- **GeneralUtils** — File conversion, filename generation, pipeline extraction
- **FormUtils** / **GeneralFormCopyUtils** — PDF form field operations
- **OfficeDocumentSanitizer** — Sanitizes Office documents before conversion
- **CustomHtmlSanitizer** / **SvgSanitizer** — Input sanitization (OWASP)
- **EmlParser** / **EmlToPdf** — Email (EML/MSG) to PDF conversion
- **CbrUtils** / **CbzUtils** — Comic book archive handling
- **ZipExtractionUtils** — ZIP archive operations

---

## 4. PDFBox vs External Tools

### 4.1 PDFBox (Pure Java — Always Available)

The primary processing engine. Used for:
- **PDF loading** via `CustomPDFDocumentFactory` with 3-tier memory strategy
- **Text extraction** via `PDFTextStripper` subclass (`TextCollectingStripper`)
- **Page manipulation** — rotation, deletion, duplication, rearrangement
- **Form field handling** — extraction, modification, flattening
- **Metadata management** — read/write PDF metadata and XMP
- **Annotation handling** — comments, form widgets
- **Font handling** — font extraction, embedding, Type3 font conversion
- **Image embedding** — placing images onto PDF pages
- **Content stream generation** — regenerating page content from JSON
- **PDF/A preflight** — via `org.apache.pdfbox:preflight`
- **Signing** — digital signature creation/validation (with BouncyCastle)
- **Watermarking** — page numbers, overlays
- **PDF merging** — fallback (primary merge uses jpdfium)

**PDFBox version**: 3.0.7. Also uses: `pdfbox-io`, `xmpbox`, `preflight`, `pdfbox-examples`, `jbig2-imageio` (JBIG2 support)

### 4.2 JPDFium (Native PDFium — Always Available)

`com.stirling:jpdfium:1.0.2` with platform-specific natives (linux-x64, linux-arm64, darwin-x64, darwin-arm64, windows-x64).

Used for:
- **PDF merging** — primary merge engine (`PdfMerge.merge()`) with bookmark tree combination and TOC generation
- **Signature pre-validation** — checking for signature fields before flatten pass
- **PDF rendering** — likely for page thumbnail/preview generation

### 4.3 External Tools (Optional — Dependency-Checked at Startup)

The `ExternalAppDepConfig` runs parallel probes (using virtual threads) at `@PostConstruct` to detect available tools. Missing tools cause their feature groups to be **automatically disabled** via `EndpointConfiguration.disableGroup()`.

| Tool | Binary | Feature Groups | Used For |
|------|--------|---------------|----------|
| **Ghostscript** | `gs` | Ghostscript | PDF font normalization (PDF→JSON pipeline), PDF repair |
| **OCRmyPDF** | configurable path | OCRmyPDF | OCR processing (wraps Tesseract + Ghostscript) |
| **Tesseract** | `tesseract` | tesseract | Direct OCR text recognition |
| **LibreOffice** | configurable `sOfficePath` | LibreOffice | Office-to-PDF conversion (doc, docx, xls, xlsx, ppt, etc.) via `unoconvert` or direct `soffice` |
| **WeasyPrint** | configurable path | Weasyprint | HTML-to-PDF conversion (requires ≥ v58.0) |
| **qpdf** | `qpdf` | qpdf | PDF linearization, optimization, watermark removal (requires ≥ v12.0.0) |
| **ImageMagick** | `magick` | ImageMagick | Advanced image processing, format conversion |
| **Python + OpenCV** | `python3`/`python` | Python, OpenCV | Computer vision operations (blank page detection, auto-crop, etc.) |
| **pdftohtml** | `pdftohtml` | Pdftohtml | PDF to HTML conversion |
| **Calibre** | configurable path | Calibre | Ebook-to-PDF conversion |
| **Unoconvert** | configurable `unoconvPath` | Unoconvert | Alternative Office document converter |
| **rar** | `rar` | rar | CBR (comic book RAR) creation |
| **FFmpeg** | `ffmpeg` | — Disabled due to CVE concerns |

Each external tool has:
- **Configurable concurrency** via `ApplicationProperties.ProcessExecutor.SessionLimit` (per-tool semaphore limits)
- **Configurable timeouts** via `ApplicationProperties.ProcessExecutor.TimeoutMinutes` (per-tool timeout)
- **Command validation** in `ProcessExecutor.validateCommand()` — blocks null bytes, newlines, path traversal
- **Process tree cleanup** — descendants destroyed on timeout
- **LibreOffice UnoServer pool** — connection pooling for persistent LibreOffice server instances

---

## 5. PDF-JSON Document Model & Conversion Pipeline

### 5.1 Model Hierarchy (`model/json/`)

```
PdfJsonDocument
├── PdfJsonMetadata (title, author, subject, keywords, creator, producer, dates)
├── xmpMetadata: String (Base64-encoded XMP packet)
├── lazyImages: boolean
├── List<PdfJsonFont>
│   ├── id, name, family, type (TrueType, Type1, Type3, CIDFont, etc.)
│   ├── PdfJsonFontCidSystemInfo (registry, ordering, supplement)
│   ├── base64Data (embedded font subset)
│   ├── PdfJsonFontConversionCandidate / PdfJsonFontConversionStatus
│   └── PdfJsonFontType3Glyph (for Type3 fonts)
├── List<PdfJsonPage>
│   ├── pageNumber, width, height, rotation
│   ├── List<PdfJsonTextElement>
│   │   ├── text, fontId, fontSize, fontSizeInPt, fontMatrixSize
│   │   ├── characterSpacing, wordSpacing, spaceWidth, horizontalScaling
│   │   ├── leading, rise, x, y, width, height
│   │   ├── textMatrix: float[6], fillColor, strokeColor
│   │   ├── renderingMode, fallbackUsed, charCodes: int[]
│   ├── List<PdfJsonImageElement>
│   │   ├── x, y, width, height, zIndex
│   │   ├── base64Data (embedded) or lazyReference (lazy loading)
│   │   └── mimeType, colorSpace, bitsPerComponent
│   ├── List<PdfJsonAnnotation>
│   ├── resources: PdfJsonCosValue (serialized page resource dictionary)
│   └── List<PdfJsonStream> (raw content streams for lossless round-tripping)
└── List<PdfJsonFormField> (AcroForm fields)
```

### 5.2 PDF → JSON Pipeline (`convertPdfToJson`)

1. **Upload & Optional Normalization** — File uploaded as `MultipartFile`, saved to temp file. If `fontNormalizationEnabled` and Ghostscript is available, runs `normalizePdfFonts()` via Ghostscript to expand font subsets
2. **Font Collection** (30–50% progress) — Iterates all pages, collects font resources into `LinkedHashMap<String, PdfJsonFont>`, handles font deduplication via `IdentityHashMap<COSBase, FontModelCacheEntry>`
3. **Text Extraction** (50–70%) — Uses `TextCollectingStripper` (extends `PDFTextStripper`) to extract positioned text elements with full matrix transforms, colors, rendering modes, and character codes
4. **Image Extraction** (70–90%) — Extracts all `PDImageXObject` instances, encodes as Base64, or in lazy mode stores references for on-demand loading via `PdfLazyLoadingService`
5. **Annotation & Form Extraction** — Extracts `PDAnnotation` objects and `PDAcroForm` fields
6. **Content Stream Serialization** — Raw PDF content streams preserved as `PdfJsonStream` for lossless round-tripping
7. **Cache Management** — PDDocuments cached in LRU cache with configurable budget (bytes or % of max heap)
8. **JSON Serialization** — Uses Jackson `ObjectMapper` with `@JsonInclude(NON_NULL/NON_DEFAULT)` to write compact JSON

### 5.3 JSON → PDF Pipeline (`convertJsonToPdf`)

1. **Document Creation** — New `PDDocument` created
2. **Metadata Application** — Restores document metadata and XMP packet
3. **Font Map Building** — Reconstructs `Map<String, PDFont>` from `PdfJsonFont` models, handles Type3 font conversion via `Type3FontConversionService`
4. **Page Reconstruction** — For each page:
   - Creates `PDPage` with original dimensions and rotation
   - Applies preserved content streams (lossless path)
   - Reconstructs image XObjects if content streams preserved
   - Runs text element **preflight** — validates character coverage, applies fallback fonts if needed
   - **Token rewrite** or **full regeneration**: Attempts to rewrite text operators in preserved content streams; falls back to full stream regeneration
   - Handles vector graphics extraction/preservation
   - Restores annotations and form fields
5. **AcroForm Reconstruction** — Restores document-level form fields
6. **Save** — Writes final PDF to output stream

### 5.4 Domain Model Interfaces

The `model/domain/` package defines clean interfaces for the document model:
- `IDocumentModel`, `IPageModel`, `ITextObject`, `IImageObject`, `IAnnotation`, `IMetadataModel`
- `DomainObject`, `DomainObjectType`
- `IDocumentChangeListener` — observer pattern for document changes

---

## 6. Security Architecture

### 6.1 Profile-Based Security

Security is **conditional** on the active Spring profile:
- **`default` profile** (no proprietary/SaaS JARs) → No security, all endpoints `permitAll()`
- **`security` profile** (proprietary JAR present) → Full security stack
- **`saas` profile** (SaaS JAR present) → Supabase-based auth

### 6.2 Security Stack (proprietary module)

**`SecurityConfiguration.java`** (`@Profile("!saas")`):
- **Spring Security** with `@EnableWebSecurity` + `@EnableMethodSecurity`
- **Authentication methods**:
  - **Username/password** — Form login via `/perform_login`, custom success/failure handlers
  - **OAuth2** — OpenID Connect with custom `UserService`, authority mapping, Tauri-aware authorization resolver
  - **SAML2** — OpenSAML5 provider, custom authentication converter, Pro+ only
  - **JWT** — `JwtAuthenticationFilter` for API token auth, `ApiKeyAuthenticationService`
  - **Remember-me** — Persistent token repository (JPA-backed), secure cookies
- **Session management** — `SessionPersistentRegistry` for concurrent session control
- **Rate limiting** — `IPRateLimitingFilter` + Bucket4j token bucket
- **CSRF** — Configured via Spring Security (disabled for API endpoints)
- **CORS** — Configured in `WebMvcConfig` with Tauri origin support (`tauri://localhost`, `http://tauri.localhost`)
- **IP-based login attempt tracking** — `LoginAttemptService`
- **Strict HTTP Firewall** — `StrictHttpFirewall` bean
- **Public endpoints** — Static resources and public auth endpoints are `permitAll()`; everything else requires authentication

### 6.3 Security Sub-packages (proprietary/security/)

| Package | Contents |
|---------|----------|
| `configuration/` | SecurityConfiguration, DatabaseConfig, CacheConfig, MailConfig, PasswordEncoderConfig |
| `filter/` | IPRateLimitingFilter, JwtAuthenticationFilter, UserAuthenticationFilter |
| `oauth2/` | Custom OAuth2 success/failure handlers, user service, Tauri auth resolver |
| `saml2/` | Custom SAML2 handlers, response authentication converter |
| `service/` | CustomUserDetailsService, JwtServiceInterface, UserService, LoginAttemptService, ApiKeyAuthenticationService |
| `session/` | Session persistent registry |
| `database/` | JPA repositories (PersistentLogin, etc.) |
| `model/` | User, Role, PersistentLogin entities |
| `repository/` | JPA repositories |

### 6.4 Input Sanitization
- **OWASP Java HTML Sanitizer** (`owasp-java-html-sanitizer:20260313.1`)
- **Custom HTML sanitizer** and **SVG sanitizer**
- **Office document sanitizer** before conversion
- **ProcessExecutor command validation** — null bytes, newlines, path traversal detection
- **SSRF protection** via `SsrfProtectionService`
- **Pixee `BoundedLineReader`** for safe process output reading (5 MB line limit)

---

## 7. Configuration System

### 7.1 Configuration Hierarchy

1. **`settings.yml`** — Primary config file, loaded from `InstallationPathConfig.getSettingsPath()`
2. **Custom settings** — Secondary config from `InstallationPathConfig.getCustomSettingsPath()`
3. **Environment variables** — `SYSTEMFILEUPLOADLIMIT`, `SYSTEM_MAXFILESIZE`, `SPRING_SERVLET_MULTIPART_MAX_FILE_SIZE`, `DOCKER_ENABLE_SECURITY`, `DISABLE_ADDITIONAL_FEATURES`, `STIRLING_PDF_TAURI_MODE`, `TAURI_PARENT_PID`
4. **Spring properties** — `spring.config.additional-location` for external YAML files
5. **Application properties** — `ApplicationProperties` class with nested config for all subsystems

### 7.2 Key Configuration Classes

| Class | Purpose |
|-------|---------|
| **ApplicationProperties** | Root properties: security, system, datasource, processExecutor, pdfEditor, endpoints, legal, metrics, etc. |
| **AppConfig** | Backend URL, server port, context path |
| **RuntimePathConfig** | Paths to external tools (WeasyPrint, UnoConvert, Calibre, OCRmyPDF, soffice) |
| **InstallationPathConfig** | Config file paths, static path, templates path |
| **ConfigInitializer** | Ensures config directory and files exist at startup |
| **InitialSetup** | Generates UUID key, secret key, legal URLs, app version (highest precedence `@PostConstruct`) |

### 7.3 Process Executor Configuration

Per-tool concurrency limits and timeouts (all configurable via settings):
- LibreOffice, PDFtoHTML, Python/OpenCV, WeasyPrint, Tesseract, qpdf, Calibre, ImageMagick, Ghostscript, OCRmyPDF, FFmpeg
- Each tool has: `sessionLimit` (concurrency) and `timeoutMinutes`

### 7.4 Multipart Configuration

- Default max file size: **2000 MB** (2 GB)
- Configurable via: `SPRING_SERVLET_MULTIPART_MAX_FILE_SIZE` env var, `SYSTEMFILEUPLOADLIMIT`, `SYSTEM_MAXFILESIZE`, or `settings.yml`
- `@DependsOn("applicationProperties")` ensures settings are loaded first

---

## 8. Database Usage

### 8.1 Database Stack (proprietary module only)

- **Default**: **H2 embedded database** (file-based, PostgreSQL compatibility mode)
  - URL: `jdbc:h2:file:{configPath}/stirling-pdf-DB-2.3.232;MODE=PostgreSQL`
- **Custom**: PostgreSQL (Pro+ feature, conditional on `premium.enabled`)
- **JPA/Hibernate** with `@EnableJpaRepositories` scanning multiple packages
- **Entity packages**: security models, proprietary models, storage, workflow, policy, access, integration

### 8.2 Tables (implied from repository/entity packages)

- User accounts, roles, authorities
- Persistent login tokens (remember-me)
- Login attempts / audit logs
- API keys
- Document storage metadata
- Workflow definitions
- Policy store / policy migrations
- Account linking
- Access control rules
- Integration configurations

### 8.3 No Database in Core

The `:core` and `:common` modules are **database-free**. All persistence is in the `:proprietary` module. The core PDF processing pipeline is entirely stateless (request → process → response).

---

## 9. File Handling & Temp File Management

### 9.1 Temp File Architecture

```
MultipartFile → TempFileManager.createManagedTempFile() → TempFile (registered in TempFileRegistry)
                                                                  ↓
                                                       Processing (PDFBox/jpdfium/external tools)
                                                                  ↓
                                                       TempFile.close() → automatic delete + unregister
```

### 9.2 TempFileManager Features

- **Configurable base directory** via `system.tempFileManagement.baseTmpDir` (default: system temp)
- **Configurable prefix** via `system.tempFileManagement.prefix`
- **Automatic registration** in `TempFileRegistry` (tracks all temp files for cleanup)
- **Dedicated LibreOffice temp directory** — `registerLibreOfficeTempDir()` creates `stirling-pdf-libreoffice/` subdirectory
- **Scheduled cleanup** — `TempFileCleanupService` deletes files older than `maxAgeHours`
- **Shutdown hook** — `TempFileShutdownHook` ensures cleanup on JVM exit
- **`TempFile` class** — AutoCloseable wrapper; closing triggers deletion

### 9.3 CustomPDFDocumentFactory Memory Strategy

| File Size | Strategy |
|-----------|----------|
| ≤ 10 MB | Fully in-memory (`byte[]` → `Loader.loadPDF()` with `MemoryUsageSetting.setupMixed(10MB)`)
| 10–50 MB | Mixed mode if ≥30% heap free and ≥256 MB free; otherwise file-backed |
| > 50 MB | Always file-backed via `RandomAccessReadBufferedFile` (non-destructive, original file preserved) |

**Concurrency**: Bounded by `Semaphore(MAX_CONCURRENT_OPS)` where `MAX_CONCURRENT_OPS = CPU cores`.

---

## 10. OCR Integration

OCR is provided through **two paths**:

1. **OCRmyPDF** (Python wrapper) — Configurable binary path, uses `ProcessExecutor.Processes.OCR_MY_PDF`. Combines Tesseract + Ghostscript for optimal results.
2. **Tesseract** (direct) — Configurable via `ProcessExecutor.Processes.TESSERACT`. Direct OCR output.

Both are **optional** — detected at startup by `ExternalAppDepConfig`. If not found, OCR features are disabled via `EndpointConfiguration.disableGroup()`.

---

## 11. Error Handling Patterns

### 11.1 Global Error Responses

`GlobalErrorResponseCustomizer` adds standard error responses (400, 413, 422, 500) to all `/api/v1/**` OpenAPI operations.

Error response format:
```json
{
  "status": 400,
  "error": "Bad Request",
  "message": "Invalid input parameters or corrupted file",
  "timestamp": "2024-01-15T10:30:00Z",
  "path": "/api/v1/example/endpoint"
}
```

### 11.2 PDF-Specific Error Handling

- **`PdfErrorUtils.isCorruptedPdfError()`** — Detects corrupted PDF errors
- **`ExceptionUtils.createNullArgumentException()`** — Standard null argument exceptions
- **`ExceptionUtils.createMultiplePdfCorruptedException()`** — Multi-file corruption errors
- **`ExceptionUtils.logException()`** — Structured exception logging

### 11.3 Controller-Level Patterns

- Controllers throw `IOException` which Spring converts to 500 responses
- `@AutoJobPostMapping` handles job-level error tracking
- Multipart parsing errors → 400/413
- `MergeController` specifically validates each input PDF with jpdfium before merging, collecting invalid indexes

### 11.4 Process-Level Errors

- `ProcessExecutor` throws `IOException` with exit code and error output on non-zero exit
- Special handling for qpdf exit code 3 (success with warnings)
- Process tree cleanup (descendants destroyed) on timeout
- Virtual thread interruption handling for stream readers

---

## 12. Performance Patterns

### 12.1 JVM Configuration (bootRun)

```
-XX:+UseG1GC
-XX:MaxGCPauseMillis=200
-XX:G1HeapRegionSize=4m
-XX:+ExplicitGCInvokesConcurrent
-XX:+UseStringDeduplication
--enable-native-access=ALL-UNNAMED
+ Compact Object Headers (JDK 25+ product flag, JEP 519)
```

### 12.2 Concurrency

- **Virtual threads** — Used extensively: `Executors.newVirtualThreadPerTaskExecutor()`, `Thread.ofVirtual()`, virtual threads for process output readers
- **Semaphore-bounded concurrency** — Per-tool limits in `ProcessExecutor` and `CustomPDFDocumentFactory`
- **Parallel dependency checking** — `ExternalAppDepConfig` probes all tools in parallel at startup
- **ConcurrentHashMap** — Used for document caches, font caches, process executor instances

### 12.3 Memory Management

- **3-tier PDF loading** in `CustomPDFDocumentFactory` (in-memory, mixed, file-backed)
- **LRU document cache** in `PdfJsonConversionService` with configurable budget
- **Heap-aware decisions** — Memory snapshots captured atomically; falls back to file-backed when heap is low
- **String deduplication** — JVM flag enabled
- **IdentityHashMap** for font/image deduplication during extraction (avoids equals() overhead)

### 12.4 Caching

- **Font cache** per document extraction (`IdentityHashMap<COSBase, FontModelCacheEntry>`)
- **Type3 font cache** (`ConcurrentHashMap`) for normalized Type3 fonts and glyph coverage
- **Document cache** with LRU eviction and configurable byte budget
- **Static asset caching** — Vite fingerprinted assets: 1-year immutable; branding: 1-day + 7-day SWR; SW: no-store
- **Gzip/Brotli pre-compression** for frontend assets (`index.html.gz`, `index.html.br`, etc.)
- **SPA prerendered pages** with OG tags per tool

### 12.5 Jetty HTTP/2

- `spring-boot-starter-jetty` with `jetty-http2-server` and `jetty-alpn-java-server`
- Tomcat explicitly excluded: `configurations.configureEach { exclude group: "org.springframework.boot", module: "spring-boot-starter-tomcat" }`

---

## 13. Custom PDFBox Patches / Extensions

No direct PDFBox source patches were observed. Instead, the project extends PDFBox through:

1. **`PDFTextStripper` subclass** (`TextCollectingStripper`) — Custom text extraction capturing positioned text with full matrix transforms, font references, rendering modes, and character codes for the JSON round-trip pipeline
2. **`PDFGraphicsStreamEngine` import** — Referenced in `PdfJsonConversionService` (likely used for content stream analysis/parsing)
3. **`CustomPDFDocumentFactory`** — Smart loading wrapper around `Loader.loadPDF()` with memory-aware stream cache functions
4. **`PdfJsonCosMapper`** — Custom COS object serialization/deserialization for preserving low-level PDF structures
5. **JPDFium bridge** — Uses PDFium (via jpdfium) for high-performance merge and render operations, complementing PDFBox

---

## 14. Startup & Initialization Sequence

1. **`main()`** — `ConfigInitializer.ensureConfigExists()`, load external settings files, set Spring profiles based on classpath detection, `app.run(args)`, create template/static directories, print startup logs
2. **`@PostConstruct InitialSetup`** (highest precedence) — Generate UUID + secret key (persisted to settings.yml), set legal URLs, record app version, detect new server
3. **`ContextRefreshedEvent`** — `StartupApplicationListener` records start time
4. **`@PostConstruct ExternalAppDepConfig`** — Parallel probe all external tools (virtual threads), disable unavailable feature groups, check Python/OpenCV, log summary
5. **`@PostConstruct TempFileConfiguration`** — Create temp directory, initialize registry
6. **`@PostConstruct PdfJsonConversionService`** — Load font normalization config, check Ghostscript availability, initialize cache budget
7. **`@PostConstruct SPDFApplication.init()`** — Normalize backend URL, detect Tauri mode, optionally open browser
8. **`ApplicationReadyEvent`** — Log actual runtime port, `WorkspaceManager` ready

---

## 15. Desktop Mode (Tauri) vs Web Mode

### 15.1 Tauri Desktop Mode

Activated via `STIRLING_PDF_TAURI_MODE=true` system property.

Key differences:
- **`TauriProcessMonitor`** (`@ConditionalOnProperty("STIRLING_PDF_TAURI_MODE")`) — Monitors parent Tauri process via `TAURI_PARENT_PID` env var. Checks every 5 seconds using `ProcessHandle.of(pid)`. If parent dies, initiates graceful shutdown of Spring context to prevent orphaned Java processes.
- **CORS** — WebMvcConfig automatically adds Tauri protocol origins: `tauri://*`, `tauri://localhost`, `http://tauri.localhost`, `https://tauri.localhost`
- **OAuth2** — `TauriAuthorizationRequestResolver` adjusts OAuth2 authorization requests for Tauri context
- **No browser opening** — Skips `BROWSER_OPEN` logic
- **Process monitoring** — Virtual thread-based scheduler, 5-second interval
- **Graceful shutdown** — 1-second delay before closing Spring context, falls back to `System.exit()`

### 15.2 Web Mode

- Standard browser-based access
- `BROWSER_OPEN=true` env var opens browser on startup (OS-specific: `rundll32`/`open`/`xdg-open`) using `io.github.pixee.security.SystemCommand`
- CORS defaults to allowing all origins (`allowedOriginPatterns("*")`) unless custom origins configured
- Full security stack available (OAuth2, SAML2, JWT, etc.)

### 15.3 Frontend Modes

The Gradle build supports multiple frontend modes via `-PfrontendMode` or auto-detection:
- `prototypes` → `-PprototypesMode=true`
- `saas` → SaaS frontend
- `proprietary` → Full proprietary frontend (default when proprietary module present)
- `core` → Minimal core frontend (when `disableAdditional=true`)

Backend-only mode (no frontend) serves `api-landing.html` as `index.html`.

---

## 16. Key Dependencies Summary

### 16.1 Core Dependencies

| Dependency | Version | Purpose |
|-----------|---------|--------|
| Spring Boot | 4.0.6 | Application framework |
| Apache PDFBox | 3.0.7 | PDF processing engine |
| JPDFium | 1.0.2 | Native PDFium (merge, render) |
| BouncyCastle | 1.78 | Cryptography (signing, encryption) |
| Jackson 2 | 2.22.1 | JSON serialization |
| Jetty | (managed) | Servlet container with HTTP/2 |
| TwelveMonkeys ImageIO | 3.13.1 | Image format support (BMP, JPEG, TIFF, WebP, SVG, PSD, etc.) |
| Batik | 1.19 | SVG processing (bridge module) |
| graphics2d (rototor) | 3.0.5 | Batik SVG → PDF conversion |
| Apache POI | 5.5.1 | Office document (OOXML) handling |
| Tabula | 1.0.5 | Table extraction from PDFs |
| Guava | 33.6.0-jre | Collections, caching, utilities |
| Commons IO | 2.15.1 | File utilities |
| Commons Lang3 | 3.20.0 | String/object utilities |
| OWASP HTML Sanitizer | 20260313.1 | Input sanitization |
| ZXing | 3.5.4 | Barcode generation |
| Commonmark | 0.28.0 | Markdown processing |
| OpenCSV | 5.12.0 | CSV parsing/writing |
| Flexmark | 0.64.8 | HTML-to-Markdown conversion |
| metadata-extractor | 2.20.0 | Image metadata (EXIF, etc.) |
| Simple Java Mail | 8.12.6 | EML/MSG email parsing |
| junrar | 7.6.0 | RAR archive support |
| veraPDF | 1.30.2 | PDF/A validation |
| Rhino | 1.9.1 | JavaScript engine (veraPDF dependency, CVE fix override) |
| Bucket4j | 8.19.0 | Rate limiting |
| Micrometer | (managed) | Metrics/Prometheus |
| Telegram Bots | 6.9.7.1 | Telegram notifications (long-polling, Grizzly/Jersey excluded) |
| PostHog | 1.2.0 | Product analytics |
| JAXB (Jakarta) | 4.0.2 | XML binding (veraPDF) |

### 16.2 CVE Mitigations

- **CVE-2025-48924**: `commons-lang3` forced to 3.20.0 (DoS prevention)
- **CVE-2024-47554**: `commons-io` forced to 2.15.1 (DoS prevention)
- **CVE-2025-66453**: `org.mozilla:rhino` forced to 1.9.1 (overrides veraPDF's 1.7.13)
- **CVE-2022-25647**: `gson` forced to 2.14.0 (unsafe deserialization)
- **FFmpeg disabled** entirely due to raised CVEs
- **Pixee `SystemCommand`** used for safe browser opening (replaces raw `Runtime.exec()`)
- **`BoundedLineReader`** used for safe process output reading

---

## 17. Monitoring & Observability

- **Micrometer** metrics with custom `PdfMetricsInterceptor` (per-request PDF metrics)
- **`MetricsAggregatorService`** — Aggregates metrics across operations
- **`PdfMetricsService`** — PDF-specific metrics (page count, file size, processing time)
- **`MetricsConfig`** — Metrics configuration
- **`MetricsFilter`** — Servlet filter for request metrics
- **`WAUTrackingFilter`** — Weekly Active Users tracking
- **`ResourceMonitor`** — System resource monitoring
- **OpenAPI/Swagger** — `springdoc-openapi-starter-webmvc-ui:3.0.3` with custom global error response documentation, SwaggerHub integration
- **`OpenApiConfig`** / **`SpringDocConfig`** — OpenAPI customization
- **`EndpointInspector`** — Endpoint discovery and documentation
- **SonarQube** integration via `org.sonarqube` Gradle plugin (7.2.3.7755)
- **JaCoCo** — Code coverage reporting

---

## 18. API Documentation

- **`ApiDocService`** — Generates API documentation
- **`ToolIORegistry`** — Registry mapping tools to their input/output formats
- **`ApiEndpoint`** model — Endpoint metadata
- **Custom annotations**: `@AutoJobPostMapping`, `@StandardPdfResponse`, `@ToolIO` (with `ToolFormat`, `ToolArity`)
- **Global error response schema** auto-injected for all `/api/v1/**` endpoints

---

## 19. Summary

PDF Elite (Stirling PDF fork) is a sophisticated, enterprise-grade PDF processing platform built on Spring Boot 4.0.6 with Java 25. The architecture follows a clean multi-module design:

- **Core module**: Stateless PDF processing with 25+ API endpoints, the massive PDF↔JSON bidirectional conversion pipeline for the web editor, and zero database dependencies
- **Common module**: Shared infrastructure including smart PDF loading with heap-aware memory management, tracked temp file lifecycle, external process execution with per-tool concurrency/timeout, and comprehensive security utilities
- **Proprietary module**: Authentication (OAuth2, SAML2, JWT, API keys), JPA persistence (H2/PostgreSQL), rate limiting, AI agents, and admin features
- **External tools**: Optional integration with Ghostscript, Tesseract/OCRmyPDF, LibreOffice, WeasyPrint, qpdf, ImageMagick, Calibre, Python/OpenCV — all auto-detected and gracefully disabled when missing
- **Desktop support**: Tauri integration with parent process monitoring, CORS handling, and OAuth2 adaptation

The system is designed for **stateless horizontal scaling** (core processing), with state (users, sessions, jobs) confined to the proprietary module's database layer. The PDF-JSON pipeline is the most complex component (~2000 lines) and represents a full bidirectional PDF editor backend with font normalization, lazy image loading, Type3 font support, and lossless content stream preservation.
