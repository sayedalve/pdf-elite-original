# Risk Register

> **FACT:** This risk register covers the entire migration from Java/Tauri/React to C++/Win32/PDFium.
> **RECOMMENDATION:** Review and update this register at the start of each sprint.

---

## Risk Matrix

|  | **Negligible (1)** | **Minor (2)** | **Moderate (3)** | **Major (4)** | **Critical (5)** |
|--|-------------------|---------------|-----------------|---------------|-----------------|
| **Almost Certain (5)** |  |  | R3, R4 | R1, R2 |  |
| **Likely (4)** |  |  | R8, R10 | R5, R6 | R7 |
| **Possible (3)** |  |  | R12, R13 | R9 |  |
| **Unlikely (2)** |  | R14 | R11 |  |  |
| **Rare (1)** |  |  |  |  |  |

*Score = Probability × Impact. Risks scored ≥ 12 are critical.*

---

## Technical Risks

### R1: PDFium API Limitations for Text Editing

| Field | Detail |
|-------|--------|
| **ID** | R1 |
| **Category** | Technical |
| **Probability** | 5 (Almost Certain) |
| **Impact** | 4 (Major) |
| **Score** | **20** |
| **Description** | PDFium has no high-level text editing API. Text editing requires direct manipulation of PDF text objects, which is complex and error-prone. Font matching, text flow, and character positioning must be handled manually. |
| **Trigger** | Attempting to implement in-place text editing (Phase 3) |
| **Mitigation** | 1. Prototype text editing in Phase 1 spike to validate feasibility. 2. Consider MuPDF for text editing if PDFium proves insufficient. 3. Limit initial scope to find/replace rather than full WYSIWYG. 4. Study how SumatraPDF and other open-source editors handle this. |
| **Owner** | Lead Developer |
| **Status** | 🔴 Open |

### R2: Large Document Performance

| Field | Detail |
|-------|--------|
| **ID** | R2 |
| **Category** | Technical |
| **Probability** | 5 (Almost Certain) |
| **Impact** | 4 (Major) |
| **Score** | **20** |
| **Description** | Documents with 1000+ pages, large images, or complex vector graphics may overwhelm the tile cache or cause excessive memory usage. |
| **Trigger** | Opening a 5000-page technical manual or a 500MB image-heavy PDF |
| **Mitigation** | 1. Implement aggressive tile eviction (LRU with priority). 2. Render tiles at reduced DPI when zoomed out. 3. Profile with real-world large documents early. 4. Set memory limits and degrade gracefully. 5. Consider disk-backed tile cache. |
| **Owner** | Performance Lead |
| **Status** | 🟡 Mitigated |

### R3: Malformed PDF Crashes

| Field | Detail |
|-------|--------|
| **ID** | R3 |
| **Category** | Technical |
| **Probability** | 5 (Almost Certain) |
| **Impact** | 3 (Moderate) |
| **Score** | **15** |
| **Description** | Malformed or maliciously crafted PDFs can crash PDFium, cause buffer overflows, or trigger undefined behavior. The current app already suffers from this (browser crashes on malformed PDFs). |
| **Trigger** | Opening any untrusted PDF file |
| **Mitigation** | 1. Wrap all PDFium calls in try/except or structured exception handling (SEH). 2. Validate PDF structure before full load. 3. Run fuzzing (libFuzzer) with PDF corpus. 4. Sand PDFium calls in a separate process (extreme mitigation). 5. Maintain a test suite of known-bad PDFs. |
| **Owner** | Security Lead |
| **Status** | 🟡 Mitigated |

### R4: Font Handling Complexity

| Field | Detail |
|-------|--------|
| **ID** | R4 |
| **Category** | Technical |
| **Probability** | 5 (Almost Certain) |
| **Impact** | 3 (Moderate) |
| **Score** | **15** |
| **Description** | PDFs can embed fonts, reference system fonts, or use CID-keyed fonts for CJK. Text editing requires matching fonts, handling missing fonts, and correctly positioning glyphs. |
| **Trigger** | Editing text in a document with embedded CID-keyed fonts |
| **Mitigation** | 1. Use PDFium's font enumeration to identify available fonts. 2. For text editing, match against system-installed fonts. 3. Fall back to a standard font if the original is not available. 4. Test extensively with CJK, Arabic, and complex script PDFs. |
| **Owner** | Lead Developer |
| **Status** | 🟡 Mitigated |

### R5: Rendering Quality Differences

| Field | Detail |
|-------|--------|
| **ID** | R5 |
| **Category** | Technical |
| **Probability** | 4 (Likely) |
| **Impact** | 4 (Major) |
| **Score** | **16** |
| **Description** | Native PDFium rendering may differ from the current JPDFium/WebView rendering in subtle ways (anti-aliasing, color profiles, subpixel rendering). Users may perceive these as regressions. |
| **Trigger** | Side-by-side comparison with current app |
| **Mitigation** | 1. Match PDFium rendering flags to current output. 2. Support both GDI and Direct2D rendering backends. 3. Implement color profile handling. 4. User-testing to identify unacceptable differences. |
| **Owner** | Rendering Lead |
| **Status** | 🔴 Open |

### R6: Win32 UI Development Speed

| Field | Detail |
|-------|--------|
| **ID** | R6 |
| **Category** | Technical |
| **Probability** | 4 (Likely) |
| **Impact** | 4 (Major) |
| **Score** | **16** |
| **Description** | Raw Win32 UI development is significantly slower than React/Mantine. Custom controls (property grids, annotation toolbars, page thumbnails) require substantial effort. |
| **Trigger** | Implementing annotation toolbar and property panels |
| **Mitigation** | 1. Build a thin UI toolkit layer to abstract common patterns. 2. Start with the simplest viable UI and iterate. 3. Consider using Windows Common Controls (ComboBoxEx, Rebar, etc.) where possible. 4. Reuse open-source Win32 control implementations. |
| **Owner** | UI Lead |
| **Status** | 🟡 Mitigated |

### R7: PDFium Version Lock-In

| Field | Detail |
|-------|--------|
| **ID** | R7 |
| **Category** | Technical |
| **Probability** | 4 (Likely) |
| **Impact** | 5 (Critical) |
| **Score** | **20** |
| **Description** | PDFium's C API is not guaranteed stable between versions. Google may change or remove functions. Pinning to a specific version means missing security fixes and PDF spec updates. |
| **Trigger** | PDFium releases a breaking API change |
| **Mitigation** | 1. Pin to a specific PDFium version initially. 2. Create a thin C++ wrapper around PDFium API (PIMPL pattern) to isolate changes. 3. Track Chromium/PDFium release notes. 4. Budget 1 week per quarter for PDFium version updates. 5. Run full regression suite on every PDFium update. |
| **Owner** | Lead Developer |
| **Status** | 🟡 Mitigated |

### R8: Annotation Implementation Complexity

| Field | Detail |
|-------|--------|
| **ID** | R8 |
| **Category** | Technical |
| **Probability** | 4 (Likely) |
| **Impact** | 3 (Moderate) |
| **Score** | **12** |
| **Description** | 12 annotation types require distinct UIs, interaction models, and PDFium API calls. Some (ink, stamps) are particularly complex. |
| **Trigger** | Implementing ink (freehand) and stamp annotations |
| **Mitigation** | 1. Prioritize text markup (highlight, underline, strikethrough) — these are most used. 2. Implement annotations in priority order. 3. Reuse PDFium's built-in annotation rendering. 4. Create an annotation factory pattern to share code. |
| **Owner** | Annotation Lead |
| **Status** | 🟡 Mitigated |

### R9: Threading and Concurrency Bugs

| Field | Detail |
|-------|--------|
| **ID** | R9 |
| **Category** | Technical |
| **Probability** | 3 (Possible) |
| **Impact** | 5 (Critical) |
| **Score** | **15** |
| **Description** | PDFium is not thread-safe per-document. Concurrent access from render threads, UI thread, and search thread can cause data races, deadlocks, or corruption. |
| **Trigger** | Scrolling while searching while a background render is in progress |
| **Mitigation** | 1. Strict threading model: one owner thread per document. 2. All PDFium access goes through a document mutex. 3. Use TSan (Thread Sanitizer) in CI. 4. Stress test with concurrent operations. 5. Design message-passing architecture (see THREADING.md). |
| **Owner** | Concurrency Lead |
| **Status** | 🟡 Mitigated |

---

## Product Risks

### R10: Feature Gaps vs Current App

| Field | Detail |
|-------|--------|
| **ID** | R10 |
| **Category** | Product |
| **Probability** | 5 (Almost Certain) |
| **Impact** | 3 (Moderate) |
| **Score** | **15** |
| **Description** | The v1.0 native app will lack some features of the current app (format conversion, OCR, compression). Users may see this as a regression. |
| **Trigger** | User attempts to convert PDF to Word and finds no option |
| **Mitigation** | 1. Clearly communicate that this is a focused PDF editor, not a PDF Swiss Army knife. 2. Prioritize the features 90% of users actually use. 3. Document feature parity status publicly. 4. Offer format conversion as a separate tool or web service link. |
| **Owner** | Product Manager |
| **Status** | 🟡 Accepted |

### R11: UI Quality Perception

| Field | Detail |
|-------|--------|
| **ID** | R11 |
| **Category** | Product |
| **Probability** | 2 (Unlikely) |
| **Impact** | 4 (Major) |
| **Score** | **8** |
| **Description** | Raw Win32 UI may look dated compared to the modern React/Mantine UI. Users accustomed to modern web UIs may perceive the app as low-quality. |
| **Trigger** | First user testing session |
| **Mitigation** | 1. Use Windows 10/11 visual styles (visual styles manifest). 2. Implement smooth animations for toolbar and sidebar transitions. 3. Use custom-drawn controls where common controls look outdated. 4. Invest in icon design and visual polish. |
| **Owner** | UI/UX Designer |
| **Status** | 🟡 Mitigated |

---

## Migration Risks

### R12: Scope Creep

| Field | Detail |
|-------|--------|
| **ID** | R12 |
| **Category** | Migration |
| **Probability** | 3 (Possible) |
| **Impact** | 4 (Major) |
| **Score** | **12** |
| **Description** | The migration may expand in scope as stakeholders request additional features, cross-platform support, or integration capabilities. |
| **Trigger** | "Can we also add..." requests during development |
| **Mitigation** | 1. Strict milestone definitions (see DEFINITION_OF_DONE.md). 2. Feature freeze at each milestone. 3. New features go into the backlog, not the current sprint. 4. Product owner has final say on scope. |
| **Owner** | Product Manager |
| **Status** | 🟡 Mitigated |

### R13: Underestimated Complexity

| Field | Detail |
|-------|--------|
| **ID** | R13 |
| **Category** | Migration |
| **Probability** | 3 (Possible) |
| **Impact** | 3 (Moderate) |
| **Score** | **9** |
| **Description** | PDF internals are notoriously complex. Text editing, form handling, and annotation editing may take 2-3x longer than estimated. |
| **Trigger** | First implementation attempt for text editing or annotations |
| **Mitigation** | 1. Time-box spikes for unknown areas. 2. Build proof-of-concept prototypes before committing to implementation. 3. Maintain 20% schedule buffer. 4. Have a fallback plan (reduce feature set). |
| **Owner** | Tech Lead |
| **Status** | 🟡 Mitigated |

### R14: PDFium Build/Integration Issues

| Field | Detail |
|-------|--------|
| **ID** | R14 |
| **Category** | Migration |
| **Probability** | 2 (Unlikely) |
| **Impact** | 3 (Moderate) |
| **Score** | **6** |
| **Description** | Building PDFium from source or integrating it via vcpkg may fail on certain configurations, Windows versions, or with certain compile flags. |
| **Trigger** | CI build failure after PDFium update |
| **Mitigation** | 1. Use pre-built PDFium binaries as primary, build-from-source as fallback. 2. Pin PDFium version strictly. 3. Maintain a working build in a separate branch. 4. Automate PDFium integration tests. |
| **Owner** | Build Engineer |
| **Status** | 🟢 Controlled |

---

## Risk Summary

| Risk | Score | Status | Priority |
|------|-------|--------|----------|
| R1: PDFium text editing API limitations | 20 | 🔴 Open | P0 — Spike in Phase 1 |
| R7: PDFium version lock-in | 20 | 🟡 Mitigated | P0 — Wrapper layer |
| R2: Large document performance | 20 | 🟡 Mitigated | P0 — Profile early |
| R5: Rendering quality differences | 16 | 🔴 Open | P1 — Side-by-side testing |
| R6: Win32 UI development speed | 16 | 🟡 Mitigated | P1 — UI toolkit layer |
| R3: Malformed PDF crashes | 15 | 🟡 Mitigated | P0 — SEH + fuzzing |
| R4: Font handling complexity | 15 | 🟡 Mitigated | P1 — CJK test suite |
| R9: Threading/concurrency bugs | 15 | 🟡 Mitigated | P0 — TSan in CI |
| R10: Feature gaps | 15 | 🟡 Accepted | P2 — Communication |
| R8: Annotation complexity | 12 | 🟡 Mitigated | P1 — Prioritized rollout |
| R12: Scope creep | 12 | 🟡 Mitigated | P1 — Milestone discipline |
| R13: Underestimated complexity | 9 | 🟡 Mitigated | P2 — Spikes + buffer |
| R11: UI quality perception | 8 | 🟡 Mitigated | P2 — Visual polish |
| R14: PDFium build issues | 6 | 🟢 Controlled | P3 — Pre-built binaries |