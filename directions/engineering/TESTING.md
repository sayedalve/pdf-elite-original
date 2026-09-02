# Testing Architecture — PDF Elite C++/Win32/PDFium Rebuild

> **Document ID:** FILE-19 | **Version:** 1.0 | **Status:** Draft
> **Source Analysis:** PDF Elite v2.14.2 (Tauri v2 + React 19 + Java 25/Spring Boot)

---

## 1. Current Testing Landscape

### 1.1 Existing Test Frameworks

```
FACT (AGENTS.md): "No unit tests currently (relies on UI and API testing per AGENTS.md)"
```

Despite no unit tests, the project has test infrastructure defined for multiple layers:

| Layer | Framework | Status | Notes |
|-------|-----------|--------|-------|
| **Backend** | JUnit 5 | Defined | Spring Boot controller tests |
| **Frontend** | Vitest | Defined | React component/hook tests |
| **E2E Browser** | Playwright | Stubbed + Live | UI interaction tests |
| **Python Engine** | pytest | Defined | PDF processing pipeline |
| **Integration** | Cucumber | Defined | Feature-based integration tests |
| **Docker** | Docker integration tests | Defined | Full-stack smoke tests |

```
FACT: 25+ API controllers use @AutoJobPostMapping for job tracking.
  These endpoints are the primary integration test surface.
```

### 1.2 Current Testing Gaps

```
┌────────────────────────────────────────────────────────────────┐
│                Current Testing Gaps                            │
├────────────────────────────────────────────────────────────────┤
│ ❌ No unit tests for core business logic                        │
│ ❌ No rendering correctness tests                               │
│ ❌ No memory leak detection tests                               │
│ ❌ No thread safety tests                                      │
│ ❌ No performance regression tests                              │
│ ❌ No malformed/corrupted PDF handling tests                    │
│ ❌ No large document performance tests                          │
│ ❌ Error handling not tested systematically                      │
│ ❌ Security edge cases not tested                               │
│ ❌ No crash recovery tests                                      │
└────────────────────────────────────────────────────────────────┘
```

---

## 2. Proposed Testing Architecture

### 2.1 Testing Pyramid

```
RECOMMENDATION: Inverted pyramid with heavy emphasis on unit + integration:

                    ┌─────────────┐
                    │   E2E UI    │   ~5%  — Windows UI Automation
                   ┌┴─────────────┴┐
                   │  Integration   │  ~25% — PDFium operations, rendering
                  ┌┴───────────────┴┐
                  │    Unit Tests    │ ~70% — All C++ modules, algorithms
                  └─────────────────┘

Total target: >80% code coverage
```

### 2.2 Test Framework Stack

| Layer | Framework | Purpose |
|-------|-----------|---------|
| **Unit Tests** | Google Test (gtest) | All C++ modules, algorithms, data structures |
| **Mocking** | Google Mock (gmock) | Interface mocking for cross-module tests |
| **Integration** | Google Test (custom fixtures) | PDFium operations end-to-end |
| **Rendering** | Google Test + bitmap comparison | Pixel-level rendering correctness |
| **Memory** | AddressSanitizer (ASan) | Memory leak and corruption detection |
| **Threading** | ThreadSanitizer (TSan) | Race condition detection |
| **Undefined Behavior** | UBSan | UB detection (integer overflow, etc.) |
| **Fuzzing** | libFuzzer | Continuous PDF parsing fuzzing |
| **UI Automation** | Windows UI Automation API | E2E UI testing |
| **Performance** | Custom benchmark framework | Regression detection |
| **Coverage** | OpenCppCoverage (MSVC) | Code coverage reports |

---

## 3. Unit Tests

### 3.1 Module Test Coverage

```
RECOMMENDATION: Every C++ module has corresponding unit tests:

┌─────────────────────────────┬───────────────────────────┬────────┐
│ Module                      │ Test File                 │ Target │
├─────────────────────────────┼───────────────────────────┼────────┤
│ pdf_document.cpp           │ pdf_document_test.cpp     │ 100%   │
│ pdf_page.cpp               │ pdf_page_test.cpp         │ 100%   │
│ pdf_text_page.cpp          │ pdf_text_page_test.cpp    │ 100%   │
│ pdf_bitmap.cpp             │ pdf_bitmap_test.cpp       │ 100%   │
│ pdf_annotation.cpp         │ pdf_annotation_test.cpp   │ 100%   │
│ pdf_form.cpp               │ pdf_form_test.cpp         │ 100%   │
│ tile_cache.cpp             │ tile_cache_test.cpp       │ 100%   │
│ thumbnail_cache.cpp        │ thumbnail_cache_test.cpp  │ 100%   │
│ text_cache.cpp             │ text_cache_test.cpp       │ 100%   │
│ text_search.cpp            │ text_search_test.cpp      │ 100%   │
│ pdf_metadata.cpp           │ pdf_metadata_test.cpp     │ 100%   │
│ error.cpp                  │ error_test.cpp            │ 100%   │
│ memory_budget.cpp          │ memory_budget_test.cpp    │ 100%   │
│ secure_temp_file.cpp       │ secure_temp_file_test.cpp │ 100%   │
│ file_access_handler.cpp    │ file_access_handler_test.cpp │ 95% │
│ ipc_protocol.cpp           │ ipc_protocol_test.cpp     │ 100%   │
│ render_engine.cpp          │ render_engine_test.cpp     │ 90%   │
│ page_tree.cpp              │ page_tree_test.cpp         │ 100%  │
│ bookmark_model.cpp         │ bookmark_model_test.cpp     │ 100%  │
│ document_model.cpp         │ document_model_test.cpp     │ 100%  │
└─────────────────────────────┴───────────────────────────┴────────┘
```

### 3.2 Test Patterns

```cpp
// RECOMMENDATION: Test fixture pattern for PDFium-dependent tests

class PdfDocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a known test PDF
        test_pdf_ = CreateTestPdf(
            /*pages=*/5,
            /*text=*/"Hello World",
            /*page_size=*/{612, 792}  // US Letter
        );
        ASSERT_TRUE(test_pdf_.has_value());
    }

    void TearDown() override {
        // RAII cleanup happens automatically
        test_pdf_.reset();
    }

    Result<PdfDocument> test_pdf_;
};

TEST_F(PdfDocumentTest, PageCountReturnsCorrectCount) {
    auto count = test_pdf_->PageCount();
    ASSERT_TRUE(count.has_value());
    EXPECT_EQ(count.value(), 5);
}

TEST_F(PdfDocumentTest, GetPageReturnsValidPageForValidIndex) {
    auto page = test_pdf_->GetPage(0);
    ASSERT_TRUE(page.has_value());
    EXPECT_EQ(page->Index(), 0);
}

TEST_F(PdfDocumentTest, GetPageReturnsErrorForNegativeIndex) {
    auto page = test_pdf_->GetPage(-1);
    ASSERT_FALSE(page.has_value());
    EXPECT_EQ(page.error().category, "pdf");
    EXPECT_EQ(page.error().code, 102);
}

TEST_F(PdfDocumentTest, GetPageReturnsErrorForOutOfRangeIndex) {
    auto page = test_pdf_->GetPage(100);
    ASSERT_FALSE(page.has_value());
    EXPECT_EQ(page.error().category, "pdf");
    EXPECT_EQ(page.error().code, 102);
}
```

```cpp
// RECOMMENDATION: Tile cache test with memory budget simulation

class TileCacheTest : public ::testing::Test {
protected:
    static constexpr size_t kBudgetBytes = 1024 * 1024;  // 1MB test budget
    TileCache cache_{kBudgetBytes};
};

TEST_F(TileCacheTest, InsertAndRetrieveTile) {
    auto bitmap = CreateTestBitmap(256, 256);
    TileCache::Key key{"doc1", 0, 100, 0, 0};

    cache_.Insert(key, bitmap);

    auto result = cache_.Get(key);
    ASSERT_NE(result, nullptr);
}

TEST_F(TileCacheTest, LRU evictionWhenOverBudget) {
    // Insert tiles until budget exceeded
    for (int i = 0; i < 10; ++i) {
        TileCache::Key key{"doc1", i, 100, 0, 0};
        auto bitmap = CreateTestBitmap(256, 256);  // 256KB each
        cache_.Insert(key, bitmap);
    }

    // Budget is 1MB = ~4 tiles. First tiles should be evicted.
    TileCache::Key first_key{"doc1", 0, 100, 0, 0};
    EXPECT_EQ(cache_.Get(first_key), nullptr);

    // Recent tiles should still be present
    TileCache::Key last_key{"doc1", 9, 100, 0, 0};
    EXPECT_NE(cache_.Get(last_key), nullptr);
}
```

---

## 4. Integration Tests

### 4.1 PDFium End-to-End Tests

```
RECOMMENDATION: Integration tests verify complete PDF workflows:

┌──────────────────────────────────┬──────────────────────────────────┐
│ Test Scenario                     │ Verification                       │
├──────────────────────────────────┼──────────────────────────────────┤
│ Open → Read metadata → Close     │ Metadata fields match expected    │
│ Open → Render all pages → Close   │ Each page renders without error   │
│ Open → Extract all text → Close   │ Text matches known content        │
│ Open → Search → Navigate → Close  │ Search results match, nav works  │
│ Open → Rotate page → Save → Reopen│ Rotation persisted correctly     │
│ Open → Add annotation → Save     │ Annotation saved correctly        │
│ Open → Delete page → Save         │ Page removed, others intact      │
│ Open → Split → Save two PDFs     │ Each PDF has correct pages        │
│ Open → Merge with another → Save  │ Combined PDF has all pages        │
│ Open encrypted → Enter password   │ Content accessible after decrypt  │
│ Create new → Add pages → Save     │ New document saved correctly      │
│ Open → Print → Verify spooler     │ Print job submitted               │
└──────────────────────────────────┴──────────────────────────────────┘
```

### 4.2 IPC Integration Tests

```cpp
// RECOMMENDATION: Test IPC between main process and PDFium worker

class IpcIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Launch worker process
        worker_ = LaunchWorkerProcess();
        ASSERT_TRUE(worker_.has_value());
        main_ = MainProcessStub(worker_.value());
    }

    void TearDown() override {
        worker_.reset();  // Terminates worker
    }

    Result<WorkerProcess> worker_;
    MainProcessStub main_;
};

TEST_F(IpcIntegrationTest, OpenDocument) {
    auto result = main_.OpenDocument(test_pdf_path);
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->page_count, 0);
}

TEST_F(IpcIntegrationTest, RenderPageReturnsBitmap) {
    auto doc = main_.OpenDocument(test_pdf_path);
    ASSERT_TRUE(doc.has_value());

    auto bitmap = main_.RenderPage(doc->id, 0, 150);
    ASSERT_TRUE(bitmap.has_value());
    EXPECT_GT(bitmap->width, 0);
    EXPECT_GT(bitmap->height, 0);
}

TEST_F(IpcIntegrationTest, WorkerCrashRecovery) {
    auto doc = main_.OpenDocument(test_pdf_path);
    ASSERT_TRUE(doc.has_value());

    // Simulate worker crash
    worker_->SimulateCrash();

    // Main process should detect and handle
    EXPECT_TRUE(main_.IsWorkerHealthy());

    // Should be able to reopen after recovery
    auto doc2 = main_.OpenDocument(test_pdf_path);
    ASSERT_TRUE(doc2.has_value());
}
```

---

## 5. Specialized Testing

### 5.1 Rendering Tests (Golden Image Comparison)

```cpp
// RECOMMENDATION: Pixel comparison with configurable tolerance

class RenderingTest : public ::testing::Test {
protected:
    // Maximum allowed pixel difference per channel (0-255)
    static constexpr int kMaxChannelDiff = 2;

    // Maximum percentage of pixels that can differ
    static constexpr float kMaxPixelDiffPercent = 0.5f;

    void CompareToGolden(int page_index, int dpi,
                         const std::string& golden_name) {
        // Render the page
        auto bitmap = RenderTestPage(page_index, dpi);
        ASSERT_TRUE(bitmap.has_value());

        // Load golden image
        auto golden = LoadGoldenImage(golden_name);
        ASSERT_TRUE(golden.has_value());

        // Compare
        auto result = CompareBitmaps(*bitmap, *golden,
                                      kMaxChannelDiff, kMaxPixelDiffPercent);
        EXPECT_TRUE(result.matches)
            << "Page " << page_index << " at " << dpi << " DPI: "
            << result.diff_pixel_count << " pixels differ (max: "
            << result.max_allowed_diff << ")";
    }
};

TEST_F(RenderingTest, SimpleTextPageAt150Dpi) {
    CompareToGolden(0, 150, "simple_text_page_150dpi.bmp");
}

TEST_F(RenderingTest, MultiPageDocument) {
    for (int i = 0; i < 5; ++i) {
        CompareToGolden(i, 150, "multipage_page" + std::to_string(i) + "_150dpi.bmp");
    }
}
```

### 5.2 Memory Leak Tests

```cpp
// RECOMMENDATION: ASan-based leak detection in test suite

// Test that opening and closing 100 documents doesn't leak
TEST(MemoryLeakTest, OpenCloseCycle) {
    for (int i = 0; i < 100; ++i) {
        auto doc = PdfDocument::Open(test_pdf_path);
        ASSERT_TRUE(doc.has_value());
        // Document closes automatically when going out of scope
    }
    // ASan will report any leaked memory at process exit
}

// Test that rendering doesn't leak bitmap memory
TEST(MemoryLeakTest, RenderCycle) {
    auto doc = PdfDocument::Open(multi_page_pdf);
    ASSERT_TRUE(doc.has_value());

    for (int i = 0; i < 100; ++i) {
        for (int p = 0; p < doc->PageCount(); ++p) {
            auto page = doc->GetPage(p);
            if (page.has_value()) {
                auto bitmap = RenderPage(*page, 150);
                // Bitmap freed automatically when out of scope
            }
        }
    }
}

// Test tile cache eviction doesn't leak
TEST(MemoryLeakTest, TileCacheEviction) {
    TileCache cache(1 * 1024 * 1024);  // 1MB budget
    for (int i = 0; i < 10000; ++i) {
        TileCache::Key key{"doc", i % 100, 150, 0, 0};
        auto bitmap = CreateTestBitmap(256, 256);
        cache.Insert(key, bitmap);
    }
    // Should not leak despite thousands of insertions
}
```

### 5.3 Threading Tests

```cpp
// RECOMMENDATION: ThreadSanitizer (TSan) verifies no data races

TEST(ThreadingTest, ConcurrentPageRendering) {
    auto doc = PdfDocument::Open(large_pdf);
    ASSERT_TRUE(doc.has_value());
    int page_count = doc->PageCount().value();

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t]() {
            for (int p = t; p < page_count; p += 8) {
                auto page = doc->GetPage(p);
                if (page.has_value()) {
                    auto bitmap = RenderPage(*page, 150);
                    if (bitmap.has_value()) {
                        success_count.fetch_add(1);
                    }
                }
            }
        });
    }

    for (auto& t : threads) t.join();
    EXPECT_EQ(success_count.load(), page_count);
}

TEST(ThreadingTest, ConcurrentTileCacheAccess) {
    TileCache cache(10 * 1024 * 1024);  // 10MB budget

    std::vector<std::thread> writers;
    std::vector<std::thread> readers;

    // 4 writer threads
    for (int w = 0; w < 4; ++w) {
        writers.emplace_back([&, w]() {
            for (int i = 0; i < 1000; ++i) {
                TileCache::Key key{"doc", w * 1000 + i, 150, 0, 0};
                cache.Insert(key, CreateTestBitmap(64, 64));
            }
        });
    }

    // 4 reader threads
    for (int r = 0; r < 4; ++r) {
        readers.emplace_back([&, r]() {
            for (int i = 0; i < 1000; ++i) {
                TileCache::Key key{"doc", r * 1000 + i, 150, 0, 0};
                auto result = cache.Get(key);
                (void)result;  // Just checking no data race
            }
        });
    }

    for (auto& t : writers) t.join();
    for (auto& t : readers) t.join();
    // TSan verifies no data races
}
```

### 5.4 Fuzzing Tests

```cpp
// RECOMMENDATION: libFuzzer harness for PDF parsing

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Write fuzzed data to temp file
    TempFile temp = WriteTempFile(data, size);

    // Try to open with PDFium
    auto doc = PdfDocument::Open(temp.path());
    if (doc.has_value()) {
        // If it opens, try common operations
        auto count = doc->PageCount();
        if (count.has_value()) {
            for (int i = 0; i < std::min(count.value(), 10); ++i) {
                auto page = doc->GetPage(i);
                if (page.has_value()) {
                    // Try rendering
                    auto bitmap = RenderPage(*page, 72);
                    (void)bitmap;

                    // Try text extraction
                    auto text = ExtractText(*page);
                    (void)text;
                }
            }
        }
    }

    // Cleanup happens via RAII
    return 0;
}
```

### 5.5 Large Document Tests

```
RECOMMENDATION: Dedicated tests for large document handling:

┌──────────────────────────────────┬──────────────────────────────────┐
│ Test                              │ Verification                       │
├──────────────────────────────────┼──────────────────────────────────┤
│ 1000-page PDF: open time          │ < 2 seconds to first page render  │
│ 1000-page PDF: memory usage        │ < 200MB for single page view      │
│ 1000-page PDF: sequential scroll   │ No memory growth after steady state│
│ 1000-page PDF: search              │ < 5 seconds to search all pages    │
│ 100MB PDF: open time               │ < 3 seconds to first page          │
│ 100MB PDF: memory usage            │ < 100MB for single page view      │
│ 100MB PDF: save time               │ < 30 seconds for full save        │
│ 1000-page PDF: thumbnail gen       │ All thumbnails generated < 30s    │
│ 10,000-page PDF: page navigation   │ < 500ms to render any page        │
│ Rapid open/close (100 docs)       │ No memory leak, no handle leak    │
└──────────────────────────────────┴──────────────────────────────────┘
```

---

## 6. UI Automation Tests

### 6.1 Windows UI Automation

```cpp
// RECOMMENDATION: Use Windows UI Automation API for E2E UI tests

class UiAutomationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Launch PDF Elite application
        process_ = LaunchApplication(L"pdf-elite.exe");
        automation_ = UIAutomation::Create();
        main_window_ = automation_->FindWindow(L"PDF Elite");
    }

    void TearDown() override {
        process_.Terminate();
    }

    Process process_;
    UIAutomation automation_;
    UIElement main_window_;
};

TEST_F(UiAutomationTest, CanOpenFileViaMenu) {
    auto file_menu = main_window_.FindByName(L"File");
    file_menu.Click();
    auto open_item = main_window_.FindByName(L"Open...");
    open_item.Click();

    // File dialog appears — enter path
    auto dialog = automation_->FindWindow(L"Open");
    auto path_edit = dialog_.FindByControlType(Edit);
    path_edit.SetText(test_pdf_path_);

    auto open_button = dialog_.FindByName(L"Open");
    open_button.Click();

    // Verify document loaded
    auto title = main_window_.GetTitle();
    EXPECT_TRUE(title.find(L"test.pdf") != std::wstring::npos);
}

TEST_F(UiAutomationTest, CanNavigatePages) {
    OpenTestDocument();

    auto page_label = main_window_.FindByName("PageLabel");
    EXPECT_EQ(page_label.GetText(), L"Page 1 of 5");

    auto next_button = main_window_.FindByName("NextPage");
    next_button.Click();

    EXPECT_EQ(page_label.GetText(), L"Page 2 of 5");
}
```

---

## 7. Test File Organization

```
RECOMMENDATION: Mirror source tree structure:

pdf-elite/
├── src/
│   ├── core/
│   │   ├── pdf_document.cpp
│   │   ├── pdf_page.cpp
│   │   └── ...
│   └── ...
├── tests/
│   ├── unit/
│   │   ├── core/
│   │   │   ├── pdf_document_test.cpp
│   │   │   ├── pdf_page_test.cpp
│   │   │   └── ...
│   │   ├── cache/
│   │   │   ├── tile_cache_test.cpp
│   │   │   └── ...
│   │   └── CMakeLists.txt
│   ├── integration/
│   │   ├── pdfium_integration_test.cpp
│   │   ├── ipc_integration_test.cpp
│   │   └── CMakeLists.txt
│   ├── rendering/
│   │   ├── golden_images/          # Reference bitmaps
│   │   ├── rendering_test.cpp
│   │   └── CMakeLists.txt
│   ├── fuzzing/
│   │   ├── pdf_fuzzer.cpp
│   │   ├── corpus/                 # Seed PDF files
│   │   └── CMakeLists.txt
│   ├── memory/
│   │   ├── leak_test.cpp
│   │   ├── budget_test.cpp
│   │   └── CMakeLists.txt
│   ├── threading/
│   │   ├── race_test.cpp
│   │   ├── deadlock_test.cpp
│   │   └── CMakeLists.txt
│   ├── performance/
│   │   ├── benchmarks.cpp
│   │   └── CMakeLists.txt
│   ├── ui/
│   │   ├── ui_automation_test.cpp
│   │   └── CMakeLists.txt
│   ├── data/                       # Test PDF files
│   │   ├── simple_text.pdf
│   │   ├── multi_page.pdf
│   │   ├── annotated.pdf
│   │   ├── encrypted.pdf
│   │   ├── large_1000pages.pdf
│   │   ├── malformed/
│   │   │   ├── truncated_header.bin
│   │   │   ├── corrupt_xref.bin
│   │   │   └── ...
│   │   └── regression/             # PDFs from bug reports
│   │       ├── issue_001.pdf
│   │       └── ...
│   ├── test_utils/
│   │   ├── pdf_generator.h        # Programmatic PDF creation for tests
│   │   ├── golden_image_loader.h
│   │   ├── bitmap_comparator.h
│   │   └── temp_file_helper.h
│   └── CMakeLists.txt
└── CMakeLists.txt                  # Top-level includes tests
```

---

## 8. CMake Test Configuration

```cmake
# RECOMMENDATION: CMake test configuration

# Top-level CMakeLists.txt
enable_testing()
include(CTest)

# Unit tests
add_subdirectory(tests/unit)

# Integration tests (require PDFium)
if(PDFIUM_FOUND)
    add_subdirectory(tests/integration)
endif()

# Rendering tests (require golden images)
if(BUILD_RENDERING_TESTS)
    add_subdirectory(tests/rendering)
endif()

# Fuzzing tests (only in special builds)
if(BUILD_FUZZING_TESTS)
    add_subdirectory(tests/fuzzing)
endif()

# Memory tests (ASan builds)
if(ENABLE_ASAN)
    add_subdirectory(tests/memory)
endif()

# Threading tests (TSan builds)
if(ENABLE_TSAN)
    add_subdirectory(tests/threading)
endif()
```

```cmake
# tests/unit/CMakeLists.txt
file(GLOB_RECURSE UNIT_TESTS "*.cpp")

add_executable(pdf_elite_unit_tests ${UNIT_TESTS})
target_link_libraries(pdf_elite_unit_tests
    PRIVATE
        gtest
        gtest_main
        gmock
        pdf_elite_core
        pdf_elite_cache
        pdf_elite_utils
)

# Add to CTest
include(GoogleTest)
gtest_discover_tests(pdf_elite_unit_tests
    PROPERTIES
        TIMEOUT 30
        LABELS "unit"
)
```

---

## 9. CI/CD Integration

### 9.1 Pipeline Stages

```
RECOMMENDATION: GitHub Actions CI pipeline:

┌────────────────────────────────────────────────────────────────────┐
│ Stage 1: Build Matrix                                              │
│   - Debug (ASan + UBSan) → run unit + memory tests                │
│   - Debug (TSan) → run threading tests                             │
│   - Release → run unit + integration + rendering tests            │
│   - Fuzzing → run 1-hour fuzzing session                          │
│   - Coverage → generate coverage report                            │
├────────────────────────────────────────────────────────────────────┤
│ Stage 2: Test Execution                                            │
│   - Unit tests: < 5 minutes                                        │
│   - Integration tests: < 10 minutes                                │
│   - Rendering tests: < 15 minutes                                  │
│   - Memory tests (ASan): < 20 minutes                              │
│   - Threading tests (TSan): < 10 minutes                           │
│   - Fuzzing: 1 hour (nightly)                                      │
│   - Performance benchmarks: 30 minutes (weekly)                    │
├────────────────────────────────────────────────────────────────────┤
│ Stage 3: Reporting                                                  │
│   - Test results → GitHub check                                   │
│   - Coverage → codecov.io or Codecov                              │
│   - Fuzzing → crash reports stored as artifacts                   │
│   - Performance → trend chart, alert on regression >10%          │
│   - Rendering failures → failed golden images as artifacts         │
└────────────────────────────────────────────────────────────────────┘
```

### 9.2 Coverage Targets

| Module | Target Coverage | Minimum Threshold |
|--------|-----------------|-------------------|
| Core (PDF wrapper) | 95% | 90% |
| Cache subsystem | 95% | 90% |
| Render engine | 90% | 85% |
| Search/index | 90% | 85% |
| Error handling | 100% | 95% |
| Security/validation | 100% | 95% |
| IPC protocol | 90% | 85% |
| UI (Win32) | 80% | 70% |
| **Overall** | **92%** | **85%** |

---

## 10. Comparison: Current vs. Proposed

| Aspect | Current (Tauri/React/Java) | Proposed (C++/Win32/PDFium) |
|--------|---------------------------|------------------------------|
| Unit test coverage | 0% (no unit tests) | >90% target |
| Test frameworks | JUnit 5, Vitest, Playwright | gtest, gmock, ASan, TSan, libFuzzer |
| Memory testing | None | ASan + Valgrind + custom leak tests |
| Threading testing | None | ThreadSanitizer + custom race tests |
| Rendering correctness | Manual visual inspection | Golden image comparison |
| Fuzzing | None | Continuous libFuzzer fuzzing |
| Performance regression | None | Benchmark suite with trend tracking |
| UI testing | Playwright (browser) | Windows UI Automation API |
| Crash recovery | None | Structured crash recovery tests |
| CI/CD | Partial (Docker integration) | Full pipeline with all stages |

---

*Next: See [PDF_REGRESSION_TESTS.md](PDF_REGRESSION_TESTS.md) for specific regression test cases.*

## 11. Annotation Regression (Task 11E Update)
As part of the native C++ annotation implementation, the regression suite (`RegressionSuite.cpp`) now explicitly verifies:
- Annotation Creation (`CreateAnnotation`)
- Annotation Persistence (Save/Reopen loop)
- Annotation Property Modifications (Bounds, Color/FillColor)
- Annotation Deletion (`RemoveAnnotation`)
- All tests operate over the native `pdf_engine::IAnnotation` and interact correctly with `pdf_engine::InteractionManager`.
