# PDF Regression Tests — PDF Elite C++/Win32/PDFium Rebuild

> **Document ID:** FILE-20 | **Version:** 1.0 | **Status:** Draft
> **Source Analysis:** PDF Elite v2.14.2 (Tauri v2 + React 19 + Java 25/Spring Boot)

---

## 1. Overview

```
FACT (AGENTS.md): Current application relies on UI and API testing with no unit tests.
  Regression testing is the critical safety net for the PDF processing pipeline.

ASSUMPTION: The C++ native application must have comprehensive PDF regression tests
  covering all document types encountered in production, including edge cases that
  caused issues in the Java/PDF.js/PDFium WASM stack.
```

This document defines golden PDF test cases, test categories, validation methods,
and a regression test runner design for the PDF Elite native C++ rebuild.

---

## 2. Golden PDF Test Cases

### 2.1 Test Document Library

| ID | Name | Description | Pages | Size | Source | Priority |
|----|------|-------------|-------|------|--------|----------|
| **G001** | `simple_text.pdf` | Single page, plain text, Times New Roman | 1 | 12 KB | Generated | P0 |
| **G002** | `multi_page.pdf` | 20 pages, mixed text content, page numbers | 20 | 180 KB | Generated | P0 |
| **G003** | `annotated.pdf` | 5 pages with highlight, strikeout, text, ink annotations | 5 | 95 KB | Generated | P0 |
| **G004** | `form_acro.pdf` | AcroForm with text fields, checkboxes, radio buttons, dropdown | 3 | 65 KB | Generated | P0 |
| **G005** | `form_xfa.pdf` | XFA dynamic form (if supported) | 2 | 150 KB | Generated | P1 |
| **G006** | `image_heavy.pdf` | 10 pages, 5+ high-res images per page (JPEG, PNG, TIFF) | 10 | 15 MB | Generated | P0 |
| **G007** | `encrypted_128.pdf` | RC4 128-bit encryption, owner/user password | 3 | 45 KB | Generated | P0 |
| **G008** | `encrypted_256.pdf` | AES 256-bit encryption | 3 | 48 KB | Generated | P0 |
| **G009** | `rotated_pages.pdf` | Pages rotated 0°, 90°, 180°, 270° | 4 | 40 KB | Generated | P0 |
| **G010** | `bookmarks_outline.pdf` | 3-level deep bookmark/outline tree | 15 | 200 KB | Generated | P0 |
| **G011** | `mixed_fonts.pdf` | 30+ fonts: Type1, TrueType, CFF, OpenType, CID | 5 | 2 MB | Generated | P0 |
| **G012** | `unicode_text.pdf` | CJK (Chinese, Japanese, Korean), Arabic, Hebrew, Devanagari | 5 | 300 KB | Generated | P0 |
| **G013** | `large_table.pdf` | Multi-page table with merged cells, headers, borders | 8 | 500 KB | Generated | P0 |
| **G014** | `landscape_portrait_mix.pdf` | Alternating landscape (11×8.5") and portrait (8.5×11") | 10 | 400 KB | Generated | P1 |
| **G015** | `pdfa1b.pdf` | PDF/A-1b compliant document | 5 | 1 MB | Generated | P0 |
| **G016** | `pdfa2a.pdf` | PDF/A-2a compliant document | 5 | 1.2 MB | Generated | P1 |
| **G017** | `layered_content.pdf` | Optional content groups (layers), on/off states | 5 | 800 KB | Generated | P1 |
| **G018** | `javascript_embedded.pdf` | PDF with embedded JavaScript (must be BLOCKED) | 1 | 20 KB | Generated | P0 |
| **G019** | `external_links.pdf` | GoToURI actions, Launch actions (must be BLOCKED) | 3 | 35 KB | Generated | P0 |
| **G020** | `embedded_files.pdf` | PDF with attached files (ZIP, EXE, DOCX) | 2 | 5 MB | Generated | P1 |
| **G021** | `linearized.pdf` | Fast web view (linearized) PDF | 100 | 3 MB | Generated | P1 |
| **G022** | `large_1000pages.pdf` | 1000-page text document for performance testing | 1000 | 8 MB | Generated | P0 |
| **G023** | `large_100mb.pdf` | ~100MB scanned image PDF | 500 | 100 MB | Generated | P0 |
| **G024** | `compressed_streams.pdf` | Flate, LZW, CCITT, JBIG2, JPEG2000 compressed streams | 5 | 3 MB | Generated | P1 |
| **G025** | `colorspaces.pdf` | CMYK, RGB, Gray, Lab, ICC profile-based | 5 | 2 MB | Generated | P1 |
| **G026** | `truetype_subset.pdf` | TrueType font subsetting | 3 | 150 KB | Generated | P2 |
| **G027** | `cid_fonts.pdf` | CID-keyed fonts for CJK text | 2 | 1.5 MB | Generated | P1 |
| **G028** | `forms_signed.pdf` | Digitally signed form (signature field) | 2 | 100 KB | Generated | P2 |
| **G029** | `page_labels.pdf` | Custom page labels (i, ii, iii, A-1, B-2...) | 10 | 200 KB | Generated | P1 |
| **G030** | `article_threads.pdf` | Article threads with bead chains | 5 | 150 KB | Generated | P2 |

### 2.2 Real-World Test Documents

```
RECOMMENDATION: Include real-world PDFs from public domain sources:

  Source                         | Documents       | License
  ───────────────────────────────┼─────────────────┼───────────
  IRS tax forms (irs.gov)        | f1040.pdf, ...  | Public domain
  NIH research papers (nih.gov)  | Various         | Open access
  W3C specifications             | html5.pdf, ...  | Permissive
  PDF 2.0 specification          | pdf20.pdf       | ISO (fair use)
  Adobe sample files             | Various         | Adobe license
  PDF Association samples        | Various         | Permissive

  NOTE: All test files must have verified licensing for inclusion in
  the test suite. No proprietary or copyrighted documents.
```

### 2.3 Malformed PDF Corpus

```
RECOMMENDATION: Corpus of intentionally malformed PDFs for robustness testing:

| ID | Description | Expected Behavior |
|----|-------------|-------------------|
| M001 | Truncated file (cut at 50%) | Error: corrupt PDF |
| M002 | Invalid header (%!PDF-1.0) | Error: unsupported version |
| M003 | Corrupted xref table | Error: recoverable or fail gracefully |
| M004 | Circular page tree references | Error: detect and reject (no infinite loop) |
| M005 | Object stream with invalid length | Error: partial read |
| M006 | Missing EOF marker | Error: recoverable (read to end) |
| M007 | Negative page count | Error: reject invalid value |
| M008 | Page size 0×0 | Error: reject or render empty page |
| M009 | Page size 1,000,000×1,000,000 | Error: reject unreasonable size |
| M010 | Embedded JPEG with claimed size > actual | Error: render partial/fallback |
| M011 | String with null bytes in metadata | Handle gracefully (sanitize) |
| M012 | Unicode BOM in wrong position | Handle gracefully |
| M013 | Duplicate object IDs | Handle (last one wins, per PDF spec) |
| M014 | Cross-reference stream (PDF 1.5+) | Parse correctly |
| M015 | Object types mismatched (stream claimed on non-stream) | Error: handle gracefully |

ASSUMPTION: Malformed PDFs must NEVER crash the application.
  Every malformed input must produce an Error result, not a crash.
```

---

## 3. Test Categories and Validation

### 3.1 Render Tests

```
RECOMMENDATION: Render → Compare golden bitmap pipeline

  Flow:
    1. Open golden PDF (G001..G030)
    2. Render each page at 72, 150, and 300 DPI
    3. Compare rendered bitmap to stored golden bitmap
    4. Report pixel differences exceeding tolerance

  Tolerance:
    - Max per-channel difference: 2 (out of 255)
    - Max total pixel difference: 0.5% of total pixels
    - Anti-aliasing edges: excluded from comparison (mask applied)

  Golden image format: 24-bit BMP (lossless, no compression artifacts)
  Storage: tests/rendering/golden_images/{test_id}/page{N}_{dpi}.bmp

  Test per document:
    TEST(G003_Annotated, RenderAllPages) {
        auto doc = PdfDocument::Open("annotated.pdf");
        for (int p = 0; p < doc->PageCount(); ++p) {
            for (int dpi : {72, 150, 300}) {
                auto bitmap = RenderPage(*doc->GetPage(p), dpi);
                auto golden = LoadGolden("G003", p, dpi);
                EXPECT_BITMAPS_MATCH(bitmap, golden);
            }
        }
    }
```

### 3.2 Text Extraction Tests

```
RECOMMENDATION: Extract → Compare known text pipeline

  Flow:
    1. Open golden PDF
    2. Extract text from each page
    3. Compare to known expected text file
    4. Verify whitespace normalization, Unicode correctness

  Validation rules:
    - Exact match for ASCII-only documents (G001, G002)
    - Normalized match for Unicode documents (G012): ignore ZWSP, BOM
    - Order-independent match for reflowed text (some PDFs reorder)
    - Whitespace-tolerant: collapse multiple spaces/newlines

  Expected text format: UTF-8 text files
  Storage: tests/data/expected_text/{test_id}_text.txt

  Example:
    // G001: simple_text.pdf expected text
    "The quick brown fox jumps over the lazy dog.\n"
    "Page 1 of 1\n"

    // G012: unicode_text.pdf expected text (partial)
    "こんにちは世界\n"          // Japanese
    "مرحبا بالعالم\n"          // Arabic (RTL)
    "नमस्ते दुनिया\n"          // Devanagari
    "你好世界\n"               // Chinese
```

### 3.3 Search Tests

```
RECOMMENDATION: Search → Verify results pipeline

  Test cases per document:
    1. Search for known present term → verify found on correct page(s)
    2. Search for known absent term → verify 0 results
    3. Case-sensitive search → verify correct behavior
    4. Whole-word search → verify word boundaries respected
    5. Multi-term search → verify AND/OR logic
    6. Regex search (if supported) → verify pattern matching

  | Document | Search Term | Expected |
  |----------|------------|----------|
  | G001     | "quick"    | Page 1, offset ~4 |
  | G001     | "QUICK"    | Page 1 (case-insensitive) or 0 (case-sensitive) |
  | G001     | "notfound"| 0 results |
  | G012     | "世界"     | Page 1 (Japanese), Page 4 (Chinese) |
  | G010     | "Chapter 3"| Page in bookmark tree at level 2 |
  | G022     | "Page 999" | Page 999 of 1000 |
```

### 3.4 Edit Tests

```
RECOMMENDATION: Modify → Save → Reopen → Verify pipeline

  Operations tested:
    1. Rotate page → save → verify rotation persisted
    2. Delete page → save → verify page removed, others intact
    3. Insert blank page → save → verify page added
    4. Reorder pages → save → verify new order
    5. Add text annotation → save → verify annotation present
    6. Add highlight annotation → save → verify highlight present
    7. Add ink/freehand annotation → save → verify strokes present
    8. Fill form field → save → verify field value persisted
    9. Modify metadata (title, author, subject) → save → verify
    10. Flatten form → save → verify form no longer editable

  Verification after save:
    - Page count matches expected
    - Modified pages render correctly
    - Unmodified pages render identically to original
    - Metadata fields match expected values
    - File size is reasonable (not zero, not >10x original for text edits)
```

### 3.5 Annotation Tests

```
RECOMMENDATION: Specific annotation type testing:

  | Annotation Type | Test | Verification |
  |-----------------|------|--------------|
  | Text (sticky note) | Add, save, reopen | Content text preserved |
  | Highlight | Add, save, reopen | Color, position, quadpoints preserved |
  | Underline | Add, save, reopen | Position, quadpoints preserved |
  | Strikeout | Add, save, reopen | Position, quadpoints preserved |
  | FreeText | Add, save, reopen | Text, font, position preserved |
  | Ink (freehand) | Draw stroke, save, reopen | Stroke points, width, color preserved |
  | Square | Draw rectangle, save, reopen | Position, size, color preserved |
  | Circle | Draw ellipse, save, reopen | Position, size, color preserved |
  | Line | Draw line, save, reopen | Start/end points preserved |
  | Stamp | Add stamp, save, reopen | Image content preserved |
  | File attachment | Attach file, save, reopen | File content matches original |

  Source document: G003 (annotated.pdf) has pre-existing annotations for round-trip testing.
```

### 3.6 Page Operations Tests

```
RECOMMENDATION: Page manipulation operation testing:

  | Operation    | Input               | Expected Output              |
  |-------------|---------------------|------------------------------|
  | Rotate      | G009 page 2 (90°)   | Now 180° after rotate        |
  | Split       | G002 pages 5-10     | New PDF with 6 pages         |
  | Split       | G002 pages 1,3,5    | New PDF with 3 pages (5,15,25)|
  | Merge       | G001 + G003         | New PDF with 6 pages         |
  | Insert      | G001 before page 1  | Blank page + original        |
  | Delete      | G002 page 10        | 19 pages, content preserved  |
  | Reorder     | G002 [2,1,3..20]    | Pages 2 and 1 swapped        |
  | Extract     | G002 page 15        | New PDF with 1 page          |
  | Replace     | G002 page 5 ← G001  | G001 content in G002 slot 5  |
  | Crop        | G006 page 1, box    | Cropped region only          |
```

### 3.7 Save/Reopen Round-Trip Tests

```
RECOMMENDATION: Full round-trip verification:

  1. Open document
  2. Record: page count, text per page, metadata, annotations, page sizes
  3. Save to temp file
  4. Close document
  5. Reopen from temp file
  6. Verify all recorded values match
  7. Secure-delete temp file

  Test matrix:

  | Document | Operations Before Save    | Verify After Reopen |
  |----------|--------------------------|---------------------|
  | G001     | None (identity round-trip) | Page count, text identical |
  | G002     | Rotate page 5, 10, 15    | Rotations persisted |
  | G003     | Add new annotation page 3 | Original + new annotation |
  | G004     | Fill all form fields     | Field values persisted |
  | G007     | Save with new password   | Reopen with new password |
  | G009     | Change rotation to 0°    | All pages 0° |
  | G010     | Add new bookmark          | Original + new bookmark |
  | G011     | None (identity, fonts)    | Font rendering identical |
```

### 3.8 Metadata Tests

```
RECOMMENDATION: PDF metadata read/write verification:

  Read Tests:
    | Field       | G001 Expected    | G002 Expected       |
    |-------------|------------------|---------------------|
    | Title       | "Simple Text PDF" | "Multi-Page Test"   |
    | Author      | "PDF Elite Test"  | "PDF Elite Test"    |
    | Subject     | "Unit test"       | "Regression test"  |
    | Keywords    | "test, simple"   | "test, multi-page"  |
    | Creator     | "Test Generator"  | "Test Generator"    |
    | Producer    | "PDFium Test"     | "PDFium Test"       |
    | CreationDate| Fixed test date   | Fixed test date      |
    | ModDate     | Fixed test date   | Fixed test date      |
    | Page count  | 1                 | 20                   |

  Write Tests:
    1. Set Title → save → reopen → verify Title matches
    2. Set Author → save → reopen → verify Author matches
    3. Set all fields → save → reopen → verify all match
    4. Clear all fields → save → reopen → verify empty
    5. Set very long values (10KB strings) → save → reopen → verify
    6. Set Unicode values (CJK, Arabic) → save → reopen → verify
```

### 3.9 Form Tests

```
RECOMMENDATION: Form filling and validation:

  Source: G004 (form_acro.pdf)

  Test Cases:
    1. Enumerate all form fields → verify count and types
    2. Get field value (empty) → verify ""
    3. Set text field → save → reopen → verify value
    4. Check checkbox → save → reopen → verify checked
    5. Select radio button → save → reopen → verify selection
    6. Select dropdown option → save → reopen → verify selection
    7. Fill all fields → save → reopen → verify all values
    8. Clear all fields → save → reopen → verify empty
    9. Flatten form → save → reopen → verify fields no longer interactive
    10. Calculate form (if JS calc) → verify computed values

  Field Validation:
    | Field Name     | Type        | Test Value           |
    |---------------|-------------|----------------------|
    | name          | Text        | "Jane Doe"           |
    | email         | Text        | "jane@example.com"  |
    | phone         | Text        | "+1-555-0123"        |
    | subscribe     | Checkbox    | true                 |
    | newsletter    | Radio       | "monthly"            |
    | country       | Dropdown    | "United States"      |
    | comments      | Text (multi)|"This is a\nmultiline" |
```

---

## 4. Output Validation

### 4.1 PDF Structure Validation

```cpp
// RECOMMENDATION: Post-operation PDF structure verification

Result<void> ValidatePdfStructure(const std::filesystem::path& path,
                                  const PdfStructureExpectation& expected) {
    auto doc = PdfDocument::Open(path);
    RETURN_IF_ERROR(doc);

    // Page count
    auto count = doc->PageCount();
    RETURN_IF_ERROR(count);
    EXPECT_EQ(count.value(), expected.page_count);

    // Page sizes
    for (int i = 0; i < count.value(); ++i) {
        auto page = doc->GetPage(i);
        RETURN_IF_ERROR(page);
        auto size = page->Size();
        RETURN_IF_ERROR(size);
        if (expected.page_sizes.contains(i)) {
            EXPECT_EQ(size->width, expected.page_sizes.at(i).width);
            EXPECT_EQ(size->height, expected.page_sizes.at(i).height);
        }
    }

    // Metadata
    auto meta = doc->Metadata();
    RETURN_IF_ERROR(meta);
    EXPECT_EQ(meta->title, expected.title);
    EXPECT_EQ(meta->author, expected.author);

    // Text content (sampling)
    if (!expected.sample_texts.empty()) {
        for (auto& [page_idx, text] : expected.sample_texts) {
            auto page = doc->GetPage(page_idx);
            RETURN_IF_ERROR(page);
            auto extracted = ExtractText(*page);
            RETURN_IF_ERROR(extracted);
            EXPECT_NE(extracted->find(text), std::string::npos)
                << "Page " << page_idx << " should contain: " << text;
        }
    }

    // Annotation count
    if (expected.annotation_counts.contains("all")) {
        int total = 0;
        for (int i = 0; i < count.value(); ++i) {
            auto page = doc->GetPage(i);
            RETURN_IF_ERROR(page);
            auto ann_count = page->AnnotationCount();
            RETURN_IF_ERROR(ann_count);
            total += ann_count.value();
        }
        EXPECT_EQ(total, expected.annotation_counts.at("all"));
    }

    return {};
}
```

### 4.2 Content Integrity Verification

```
RECOMMENDATION: Multi-level verification after any modification:

  Level 1 — File Level:
    - File exists and is non-zero size
    - File starts with %PDF- header
    - File ends with %%EOF marker
    - File can be opened without errors

  Level 2 — Structure Level:
    - Page count matches expected
    - Cross-reference table valid
    - All referenced objects exist
    - No circular references

  Level 3 — Content Level:
    - Text content matches expected
    - Images render correctly (bitmap comparison)
    - Annotations present and correctly positioned
    - Forms functional with correct values
    - Bookmarks/outline tree intact

  Level 4 — Semantic Level:
    - PDF/A compliance preserved (if applicable)
    - Encryption maintained (if applicable)
    - Digital signatures still valid (if applicable)
    - Page labels correct
    - File attachments intact
```

---

## 5. Regression Test Runner

### 5.1 Architecture

```
RECOMMENDATION: Dedicated regression test runner binary:

┌──────────────────────────────────────────────────────────────┐
│                   Regression Test Runner                      │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  pdf-regression-runner [OPTIONS]                             │
│                                                              │
│  Modes:                                                      │
│    --all              Run all regression tests              │
│    --render           Run render tests only                  │
│    --text-extract     Run text extraction tests              │
│    --search           Run search tests                       │
│    --edit             Run edit/save/reopen tests             │
│    --annotate         Run annotation tests                   │
│    --page-ops         Run page operation tests               │
│    --metadata         Run metadata tests                     │
│    --forms            Run form tests                          │
│    --malformed        Run malformed PDF robustness tests     │
│    --large            Run large document performance tests  │
│    --golden-update    Re-generate all golden images         │
│                                                              │
│  Options:                                                    │
│    --filter G001..G003    Run specific test documents        │
│    --dpi 72,150,300       DPI levels to test                 │
│    --tolerance 2          Max pixel channel difference       │
│    --max-diff 0.5          Max pixel diff percentage         │
│    --output-dir ./results  Output directory for reports       │
│    --baseline-dir ./goldens  Golden image directory          │
│    --verbose               Verbose logging                   │
│    --json                  JSON output for CI parsing        │
│    --timeout 60            Per-test timeout (seconds)        │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│  Exit Codes:                                                 │
│    0  All tests passed                                       │
│    1  Tests failed                                           │
│    2  Tests skipped (missing dependencies)                   │
│    3  Tests timed out                                        │
│    4  Crash detected (non-zero exit from test)              │
└──────────────────────────────────────────────────────────────┘
```

### 5.2 Runner Implementation Sketch

```cpp
// RECOMMENDATION: Regression test runner core

class RegressionTestRunner {
public:
    struct TestResult {
        std::string test_id;
        std::string category;
        bool passed;
        double duration_ms;
        std::string details;
        std::optional<std::filesystem::path> diff_image;  // For render failures
    };

    struct RunReport {
        std::vector<TestResult> results;
        int passed;
        int failed;
        int skipped;
        double total_duration_ms;
    };

    RunReport RunAll(const RunnerOptions& opts) {
        RunReport report;
        auto start = std::chrono::steady_clock::now();

        for (auto& test : registry_.tests()) {
            if (!MatchesFilter(test, opts.filter)) continue;

            auto result = RunSingleTest(test, opts);
            report.results.push_back(result);

            if (result.passed) report.passed++;
            else if (result.details == "SKIPPED") report.skipped++;
            else report.failed++;

            if (opts.json) {
                std::cout << result.ToJson() << std::endl;
            } else {
                std::cout << (result.passed ? "  PASS" : "  FAIL")
                          << " [" << result.test_id << "] "
                          << result.duration_ms << "ms" << std::endl;
            }
        }

        auto end = std::chrono::steady_clock::now();
        report.total_duration_ms =
            std::chrono::duration<double, std::milli>(end - start).count();

        return report;
    }

private:
    TestRegistry registry_;
};
```

### 5.3 CI Integration

```yaml
# RECOMMENDATION: GitHub Actions workflow step

- name: PDF Regression Tests
  run: |
    ./pdf-regression-runner --all --json --output-dir ./regression-results
  timeout-minutes: 30

- name: Upload Regression Results
  if: always()
  uses: actions/upload-artifact@v4
  with:
    name: regression-results
    path: ./regression-results/

- name: Check Regression Results
  run: |
    PASSED=$(./pdf-regression-runner --all --json --output-dir /dev/null 2>&1 | \
      jq -r '.summary.passed')
    FAILED=$(./pdf-regression-runner --all --json --output-dir /dev/null 2>&1 | \
      jq -r '.summary.failed')
    echo "Passed: $PASSED, Failed: $FAILED"
    if [ "$FAILED" -gt 0 ]; then exit 1; fi
```

---

## 6. Golden Image Generation

### 6.1 Baseline Generation Process

```
RECOMMENDATION: One-time baseline generation with review:

  1. Generate test PDFs programmatically (Pdfium + test harness)
  2. Render each page at 72, 150, 300 DPI using PDFium directly
  3. Save as 24-bit BMP in tests/rendering/golden_images/
  4. Manual visual review of ALL golden images
  5. Commit golden images to version control
  6. Any golden image change requires manual review + PR description

  Golden image naming convention:
    golden_images/
    └── {TEST_ID}/
        ├── page{N}_72dpi.bmp
        ├── page{N}_150dpi.bmp
        └── page{N}_300dpi.bmp
```

### 6.2 Golden Image Update Policy

```
RECOMMENDATION: Golden image updates require:

  1. File a PR with --golden-update output
  2. PR description must explain WHY golden images changed
  3. Manual visual review by at least one team member
  4. Accepted changes update the golden baseline
  5. Rejected changes indicate a regression to fix

  NEVER auto-update golden images in CI.
  ALWAYS require human review for golden changes.
```

### 6.3 Anti-Aliasing Handling

```
RECOMMENDATION: Exclude anti-aliasing edges from pixel comparison:

  Technique: Apply a dilation mask to both bitmaps before comparison.
  Any pixel within 2px of a color transition is excluded.

  This prevents false failures due to:
    - Sub-pixel rendering differences between runs
    - Slight font hinting variations
    - PDFium version differences (minor rendering tweaks)

  The mask is generated per-golden-image at creation time and stored
  alongside the golden bitmap.
```

---

## 7. Test Sources and Licensing

### 7.1 Generated Test PDFs

```cpp
// RECOMMENDATION: In-code PDF generation for reproducibility

// Use PDFium's FPDF_CreateNewDocument() to generate test PDFs
// This ensures tests are fully reproducible and don't depend on
// external binary files that could be corrupted or lost.

class TestPdfGenerator {
public:
    static Result<std::filesystem::path> CreateSimpleTextPdf(
        const std::filesystem::path& output_dir,
        const std::string& text,
        int page_count = 1
    );

    static Result<std::filesystem::path> CreateAnnotatedPdf(
        const std::filesystem::path& output_dir,
        const std::vector<AnnotationSpec>& annotations
    );

    static Result<std::filesystem::path> CreateFormPdf(
        const std::filesystem::path& output_dir,
        const std::vector<FormFieldSpec>& fields
    );

    static Result<std::filesystem::path> CreateMultiPagePdf(
        const std::filesystem::path& output_dir,
        int page_count,
        const std::string& content_template
    );

    static Result<std::filesystem::path> CreateLargePdf(
        const std::filesystem::path& output_dir,
        int page_count,
        PageSizePolicy policy  // Fixed, Random, Increasing
    );
};
```

### 7.2 External Test PDFs

```
RECOMMENDATION: For real-world test PDFs, maintain a manifest:

  tests/data/manifest.json
  [
    {
      "id": "irsf1040",
      "filename": "f1040.pdf",
      "source": "https://www.irs.gov/pub/irs-pdf/f1040.pdf",
      "license": "Public Domain (US Government)",
      "last_verified": "2024-01-15",
      "sha256": "abc123..."
    },
    ...
  ]

  A verification script checks:
    - File exists
    - SHA-256 matches manifest
    - License is compatible
    - Source URL still valid (optional)
```

---

## 8. Summary: Test Coverage Matrix

```
┌───────────────┬──────┬──────┬──────┬──────┬──────┬──────┬──────┐
│ Document      │Render│ Text │Search│ Edit │Annot │ Page │Meta │Forms│
│               │      │Extract│     │      │      │  Ops │ data │     │
├───────────────┼──────┼──────┼──────┼──────┼──────┼──────┼──────┼─────┤
│ G001 simple   │  ✅  │  ✅  │  ✅  │  ✅  │  —   │  ✅  │  ✅  │  —  │
│ G002 multi    │  ✅  │  ✅  │  ✅  │  ✅  │  —   │  ✅  │  ✅  │  —  │
│ G003 annot    │  ✅  │  ✅  │  ✅  │  ✅  │  ✅  │  ✅  │  ✅  │  —  │
│ G004 form     │  ✅  │  ✅  │  ✅  │  ✅  │  —   │  —   │  ✅  │  ✅ │
│ G006 images   │  ✅  │  ✅  │  ✅  │  ✅  │  —   │  ✅  │  ✅  │  —  │
│ G007 encrypt  │  ✅  │  ✅  │  ✅  │  ✅  │  —   │  ✅  │  ✅  │  —  │
│ G009 rotated  │  ✅  │  ✅  │  ✅  │  ✅  │  —   │  ✅  │  ✅  │  —  │
│ G010 bookmarks│  ✅  │  ✅  │  ✅  │  ✅  │  —   │  ✅  │  ✅  │  —  │
│ G011 fonts    │  ✅  │  ✅  │  ✅  │  —   │  —   │  —   │  ✅  │  —  │
│ G012 unicode  │  ✅  │  ✅  │  ✅  │  —   │  —   │  —   │  ✅  │  —  │
│ G015 PDF/A    │  ✅  │  ✅  │  ✅  │  ✅  │  —   │  ✅  │  ✅  │  —  │
│ G022 large    │  ✅  │  ✅  │  ✅  │  —   │  —   │  —   │  ✅  │  —  │
│ G023 100MB    │  ✅* │  ✅* │  ✅* │  —   │  —   │  —   │  ✅* │  —  │
│ M001-M015     │  ⚠️  │  ⚠️  │  —   │  —   │  —   │  —   │  —   │  —  │
└───────────────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┴─────┘
  ✅  = Full test coverage
  ⚠️  = Must not crash (error handling verification)
  ✅* = Subset (performance-focused, not all pages)
  —   = Not applicable
```

---

*Previous: See [TESTING.md](TESTING.md) for the overall testing architecture.*
