# PDF Elite Regression Testing Architecture

This document outlines the architecture and guidelines for the automated PDF regression suite integrated into the native application build.

## Test Architecture
The testing suite relies on a lightweight, custom C++ `TestFramework.h` header embedded directly into `RegressionSuite.cpp`. This avoids bloating the native footprint with gigantic frameworks like GTest while retaining essential `TEST`, `EXPECT_TRUE`, and `EXPECT_EQ` semantics. 
All tests are grouped categorically (e.g., Security, Core, Page, Perf) and are built automatically via the standard CMake pipeline into `RegressionSuite.exe`.

## Fixtures & Test Data
To ensure maintainability, the framework dynamically instantiates valid PDF byte structures at runtime into structured fixture directories rather than polluting the repository with huge blobs.
- **Base Paths**: `%workspace%/native/tests/fixtures/...`
- **Generative Tests**: `minimal.pdf` and malformed PDF stubs (`corrupt.pdf`, `truncated.pdf`) are rewritten safely on every suite run. 

## Golden File Verification
Rendering and extraction tests (scheduled for future integration) rely on Golden Output mapping.
- Output deterministic artifacts (e.g., rendered D2D BMP buffers or extracted text strings) into `tests/golden/`.
- Validate against canonical standards with a defined tolerance.
- For pixels, absolute binary equivalence is required; rendering engines are pinned to specific Chromium branch versions to prevent arbitrary kerning/anti-aliasing drifts across CI environments.

## CI Behavior
- `RegressionSuite.exe` serves as the primary health gate in GitHub Actions (`native-build.yml`).
- It outputs detailed latency and PASS/FAIL states to STDOUT.
- Exits with `1` (fail) upon any failed assertion.
- Automatically preserves generated files via the `actions/upload-artifact` workflow hook exclusively on failures to enable post-mortem analysis.

## Tolerances and Rules
- Test the API behavior, not internal PDFium pointers.
- Do not add huge external PDF files strictly to pad out tests. Use generative structures where possible. 
- "Save" implementations must load the file, execute an operation, serialize the file to disk, and successfully `LoadFromFile` that identical file matching the updated internal state. Blind "Save returned true" logic is strictly invalid.
