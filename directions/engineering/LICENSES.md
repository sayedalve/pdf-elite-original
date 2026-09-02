# License Inventory & Compliance

> **FACT:** The current PDF Elite application is a fork of Stirling PDF, licensed under Apache 2.0.
> **FACT:** The proposed native C++/PDFium rewrite will also be licensed Apache 2.0 to maintain license continuity.
> **RECOMMENDATION:** All third-party licenses must be cataloged in a `NOTICE` file shipped with the installer.

---

## Current Application Licenses

### Core Application

| Component | Version | License | Link | Used In |
|-----------|---------|---------|------|----------|
| Stirling PDF (base) | latest | Apache 2.0 | https://www.apache.org/licenses/LICENSE-2.0 | Full application base |
| PDF Elite (modifications) | current | Apache 2.0 | https://www.apache.org/licenses/LICENSE-2.0 | UI, branding, feature trim |

### Backend (Java/Spring)

| Component | Version | License | Link | Used In |
|-----------|---------|---------|------|----------|
| Spring Boot | 4.0.6 | Apache 2.0 | https://www.apache.org/licenses/LICENSE-2.0 | Application server |
| PDFBox | 3.0.7 | Apache 2.0 | https://www.apache.org/licenses/LICENSE-2.0 | PDF parsing, manipulation |
| JPDFium | 1.0.2 | **UNKNOWN** | N/A (private Maven artifact) | PDF rendering bridge |
| Tesseract | 5.x | Apache 2.0 | https://www.apache.org/licenses/LICENSE-2.0 | OCR engine |
| Ghostscript | latest | **AGPL 3.0** | https://www.gnu.org/licenses/agpl-3.0.html | PostScript, PDF conversion |
| LibreOffice | latest | MPL 2.0 | https://www.mozilla.org/en-US/MPL/2.0/ | Format conversion |
| Python/FastAPI | 3.x/0.x | MIT / ASL 2.0 | Varies | AI engine, automation |

### Frontend (JavaScript/React)

| Component | Version | License | Link | Used In |
|-----------|---------|---------|------|----------|
| React | 18.x | MIT | https://opensource.org/licenses/MIT | UI framework |
| Mantine | 7.x | MIT | https://opensource.org/licenses/MIT | Component library |
| pdfjs-dist | 4.x | Apache 2.0 | https://www.apache.org/licenses/LICENSE-2.0 | PDF rendering (fallback) |
| pdf-lib | 1.17 | MIT | https://opensource.org/licenses/MIT | Client-side PDF manipulation |
| @embedpdf/core | latest | **UNKNOWN** | N/A (commercial NPM) | PDF editing, annotations |
| @embedpdf/annotations | latest | **UNKNOWN** | N/A (commercial NPM) | 12 annotation types |
| @embedpdf/react | latest | **UNKNOWN** | N/A (commercial NPM) | React integration |

### Desktop Shell (Rust/Tauri)

| Component | Version | License | Link | Used In |
|-----------|---------|---------|------|----------|
| Tauri | 2.x | Apache 2.0 / MIT (dual) | https://tauri.app/license | Desktop shell, windowing |
| Microsoft WebView2 | latest | MIT | https://opensource.org/licenses/MIT | Browser engine |
| Rust standard lib | latest | MIT / ASL 2.0 | https://www.rust-lang.org/policies/licenses | Core runtime |

### Rust Crates (via Cargo)

| Crate | License | Notes |
|-------|---------|-------|
| serde / serde_json | MIT / ASL 2.0 | Serialization |
| tokio | MIT | Async runtime |
| reqwest | MIT / ASL 2.0 | HTTP client |
| Various others | MIT / ASL 2.0 | Utility crates |

---

## Proposed Native Application Licenses

### Core Engine

| Component | Version | License | Link | Used In |
|-----------|---------|---------|------|----------|
| PDFium | latest | BSD 3-clause | https://pdfium.googlesource.com/pdfium/+/refs/heads/main/LICENSE | Sole PDF engine |
| MSVC CRT | latest | Microsoft License | https://visualstudio.microsoft.com/license-terms/ | C++ runtime redistributable |

### vcpkg Packages

| Package | License | Notes |
|---------|---------|-------|
| pdfium | BSD 3-clause | Static library build |
| fmt | MIT | Text formatting |
| spdlog | MIT | Logging |
| nlohmann/json | MIT | JSON parsing (settings) |
| utf8proc | MIT | Unicode text processing |
| stb | MIT / Public Domain | Image loading (icons) |
| catch2 | BSL-1.0 | Testing framework (dev only) |
| google/benchmark | Apache 2.0 | Benchmarking (dev only) |

### Build Tools

| Tool | License | Notes |
|------|---------|-------|
| CMake | BSD 3-clause | Build system |
| MSVC (Visual Studio) | Microsoft EULA | Compiler |
| vcpkg | MIT | Package manager |
| WiX Toolset | MS-RL | Installer creation |

---

## License Compliance Requirements

### Distribution Requirements

| Requirement | Details | Status |
|-------------|---------|--------|
| NOTICE file | Must include all Apache 2.0 attribution notices | Required |
| LICENSE file | Ship LICENSE for the application itself | Required |
| Third-party licenses | Bundle all third-party license texts in installer | Required |
| PDFium license | Include BSD 3-clause notice | Required |
| MSVC redistributable | Include VC_REDIST license in installer | Required |
| Source availability | Apache 2.0 requires source offer on request | Required |

### RECOMMENDATION: License Display

The application should include an **About → Licenses** dialog that displays:

1. Application license (Apache 2.0) with copyright notice
2. PDFium license (BSD 3-clause)
3. MSVC runtime notice
4. List of all vcpkg packages with license links
5. Link to source code repository

### Static Linking Analysis

> **FACT:** PDFium is licensed under BSD 3-clause. Static linking is explicitly permitted.
> **FACT:** The BSD 3-clause license does not impose copyleft requirements for static or dynamic linking.
> **RECOMMENDATION:** Static link PDFium to produce a single executable with no external PDFium DLL dependency.

```
BSD 3-Clause License conditions for PDFium:
1. ✅ Redistributions of source code must retain copyright notice
2. ✅ Redistributions in binary form must reproduce copyright notice in documentation
3. ✅ Neither the name of Google nor the names of contributors may endorse/promote
4. ✅ No copyleft — static linking is fully permitted
```

### MSVC Redistributable

> **FACT:** The MSVC runtime (MSVCP140.dll, VCRUNTIME140.dll) is required at runtime.
> **RECOMMENDATION:** Bundle the VC++ Redistributable installer or statically link the CRT (`/MT` flag).

**RECOMMENDATION:** Use `/MT` (static CRT linking) to eliminate the VC++ redistributable dependency entirely. This produces a single self-contained executable.

---

## Application License

> **RECOMMENDATION:** License the new native PDF Elite under **Apache 2.0** to maintain continuity with the original Stirling PDF base and current PDF Elite codebase.

### Rationale

| Factor | Apache 2.0 | MIT | GPL 3.0 |
|--------|------------|-----|---------|
| Compatibility with Stirling PDF base | ✅ Same license | ✅ Compatible | ⚠️ Copyleft conflict |
| Patent grant | ✅ Explicit | ❌ None | ⚠️ Implicit |
| Commercial use | ✅ Yes | ✅ Yes | ⚠️ Requires disclosure |
| Contributions from others | ✅ CLA friendly | ✅ Yes | ✅ Yes |
| Corporate adoption | ✅ Preferred | ✅ Preferred | ❌ Often rejected |
| Attribution requirement | ✅ Reasonable | ✅ Minimal | ✅ Reasonable |

### Required Headers

Every source file must include:

```cpp
/*
 * Copyright [Year] PDF Elite Contributors
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
```

---

## License Risk Assessment

| Dependency | Risk Level | Issue | Mitigation |
|-----------|------------|-------|------------|
| JPDFium | 🔴 **HIGH** | UNKNOWN license, private artifact | Remove entirely; use PDFium directly |
| @embedpdf/* | 🔴 **HIGH** | UNKNOWN commercial license | Remove entirely; implement natively |
| Ghostscript | 🔴 **HIGH** | AGPL requires source distribution | Remove; feature not in scope |
| LibreOffice | 🟡 **MEDIUM** | MPL 2.0 file-level copyleft | Remove; feature not in scope |
| PDFium | 🟢 **LOW** | BSD 3-clause, well-understood | Static link, include notice |
| MSVC CRT | 🟢 **LOW** | Microsoft EULA, standard practice | Static link via /MT |
| vcpkg packages | 🟢 **LOW** | All MIT/BSD/Apache | Bundle license texts |

---

## NOTICE File Template

The following `NOTICE` file must be shipped with every distribution:

```
PDF Elite
Copyright [Year] PDF Elite Contributors

This product includes software developed at
The Apache Software Foundation (https://www.apache.org/).

Based on Stirling PDF (https://github.com/Stirling-Tools/Stirling-PDF)
Copyright 2023-2024 Stirling PDF Contributors
Licensed under the Apache License, Version 2.0.

This product includes PDFium, developed by Google.
Copyright 2014 The Chromium Authors. All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the conditions of the BSD
3-Clause License are met.

[Additional third-party notices as needed]
```
