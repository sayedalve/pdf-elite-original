# Security Architecture — PDF Elite C++/Win32/PDFium Rebuild

> **Document ID:** FILE-17 | **Version:** 1.0 | **Status:** Draft
> **Source Analysis:** PDF Elite v2.14.2 (Tauri v2 + React 19 + Java 25/Spring Boot + PDFium WASM)

---

## 1. Current Security Analysis

### 1.1 Security Profile in Existing Application

```
FACT (AGENTS.md): "Default profile has NO security"
FACT (AGENTS.md): "Security profile (proprietary JAR) enables Spring Security
  with OAuth2, SAML2, JWT, API keys, IP rate limiting (Bucket4j), SSRF protection"
```

| Security Feature | Default Profile | Security Profile |
|-----------------|-----------------|------------------|
| Authentication | ❌ None | ✅ OAuth2, SAML2, JWT, API keys |
| Authorization | ❌ None | ✅ Spring Security roles |
| Rate limiting | ❌ None | ✅ Bucket4j (IP-based) |
| SSRF protection | ❌ None | ✅ Proprietary filter |
| HTTPS enforcement | ❌ No | ✅ Configurable |
| Input validation | ❌ Minimal | ✅ Enhanced |
| Code signing | ❌ N/A (electron) | ❌ N/A |

### 1.2 Identified Security Issues in Current Codebase

```
FACT: danger_accept_invalid_certs(true) in Rust auth code (login HTTP clients)
```

**Risk:** This disables TLS certificate verification for authentication flows, making the application vulnerable to man-in-the-middle attacks during login.

```
FACT: Backend runs on localhost with random port (Tauri IPC model)
FACT: -dangerous-settings feature flag on HTTP plugin
```

**Risk:** Dangerous settings feature flag suggests relaxed HTTP security in the Tauri HTTP plugin.

```
FACT: PDF.js and PDFium WASM both process untrusted PDF content in the browser
FACT: No sandboxing of PDF processing beyond browser sandbox
```

**Risk:** Malformed PDFs could exploit vulnerabilities in PDF.js or PDFium WASM within the WebView process. The browser sandbox provides some protection but is not designed for PDF parsing security.

### 1.3 Attack Surface Summary

```
┌─────────────────────────────────────────────────────────────┐
│                   Current Attack Surface                     │
├─────────────────────┬───────────────────────────────────────┤
│ PDF.js parser       │ Untrusted PDF → DOM → XSS potential  │
│ PDFium WASM         │ Untrusted PDF → WASM memory          │
│ Spring Boot APIs    │ HTTP on localhost (Tauri proxy)       │
│ File system access  │ Temp files, user documents            │
│ Web Workers         │ PDF processing in workers             │
│ IndexedDB           │ Stored files, thumbnails              │
│ Clipboard           │ Copy/paste from PDF content           │
│ Drag-and-drop       │ File import from Explorer             │
│ Print dialog        │ OS print spooler interaction           │
└─────────────────────┴───────────────────────────────────────┘
```

---

## 2. Threat Model for Native Application

### 2.1 Threat Categories

| Threat ID | Category | Severity | Description |
|-----------|----------|----------|-------------|
| **T-001** | Malformed PDF | 🔴 Critical | Crafted PDF exploits buffer overflow in PDFium C API |
| **T-002** | Malicious PDF | 🟡 Medium | PDF with embedded JavaScript (AcroForms JS) |
| **T-003** | Malicious PDF | 🟡 Medium | PDF with external URL references (GoToURI actions) |
| **T-004** | Malicious PDF | 🟡 Medium | File path traversal in embedded file names |
| **T-005** | DLL Side-Loading | 🔴 Critical | Attacker places malicious DLL next to executable |
| **T-006** | Temp File Disclosure | 🟡 Medium | Temp files readable by other users/processes |
| **T-007** | Insecure Deletion | 🟡 Medium | Temp files not securely wiped after use |
| **T-008** | Clipboard Exposure | 🟢 Low | Sensitive PDF content copied to clipboard |
| **T-009** | Embedded Files | 🟡 Medium | ZIP/PDF archives with malicious payloads |
| **T-010** | PDF/A Validation | 🟢 Low | Bypass of document compliance checks |
| **T-011** | Supply Chain | 🔴 Critical | Compromised PDFium binary or dependency |
| **T-012** | Privilege Escalation | 🟡 Medium | Exploit in file format handlers |
| **T-013** | DoS | 🟡 Medium | PDF causing excessive memory/CPU allocation |

### 2.2 Detailed Threat Analysis

#### T-001: Malformed PDF → PDFium Memory Corruption

```
RISK: PDFium is a C library that processes complex binary formats.
  A malformed PDF can trigger:
  - Buffer overflows in stream decoders
  - Integer overflows in array size calculations
  - Null pointer dereferences
  - Use-after-free in annotation handling
  - Stack overflow in recursive structure parsing

MITIGATION: This is partially mitigated by PDFium's internal hardening,
  but the C++ wrapper must add defense-in-depth.
```

#### T-002/T-003: Malicious PDF JavaScript and URLs

```
RISK: PDFs can contain embedded JavaScript (AcroForm actions) and
  URI actions that launch external links.

FACT: Current application processes PDFs in the browser where JS execution
  is inherently risky. The native app must explicitly disable this.

MITIGATION: Disable ALL JavaScript execution in PDFs. Never open external
  URLs automatically. Warn user before following any URI action.
```

#### T-005: DLL Side-Loading

```
RISK: Windows searches for DLLs in the application directory before
  system directories. An attacker could place:
  - pdfium.dll (trojaned)
  - Version.dll or other common dependency DLLs
  next to the PDF Elite executable.

MITIGATION: Use absolute DLL paths with LoadLibraryEx, enable
  SetDefaultDllDirectories, embed manifests.
```

#### T-006/T-007: Temporary File Handling

```
FACT: TempFileManager in Java backend handles temp files.
FACT: Backend designed stateless — files processed in memory/temp only.

RISK: Temp files may contain decrypted PDF content, extracted images,
  or intermediate processing results.

MITIGATION: Use secure temp directory with restrictive ACLs.
  Overwrite file contents before deletion (DoD 5220.22-M wipe).
```

---

## 3. Proposed Security Architecture

### 3.1 Security Layers

```
RECOMMENDATION: Defense-in-depth with 5 layers:

┌──────────────────────────────────────────────────────────────┐
│ Layer 5: Application Logic Security                          │
│   - Input validation before PDFium                            │
│   - Safe string handling (no sprintf, use safe alternatives) │
│   - Resource limits (max pages, max file size)               │
├──────────────────────────────────────────────────────────────┤
│ Layer 4: Process Hardening                                    │
│   - ASLR (Address Space Layout Randomization)               │
│   - DEP (Data Execution Prevention)                          │
│   - CFG (Control Flow Guard)                                 │
│   - Stack cookies (/GS)                                       │
│   - SafeSEH                                                   │
├──────────────────────────────────────────────────────────────┤
│ Layer 3: PDFium Sandbox                                       │
│   - Process PDFium operations in a sandboxed worker process  │
│   - Restrict file system access via job objects              │
│   - Memory limits on worker process                          │
├──────────────────────────────────────────────────────────────┤
│ Layer 2: File System Security                                 │
│   - Secure temp directory creation                            │
│   - ACLs on temp files (owner-only read/write)               │
│   - Secure deletion (overwrite + delete)                     │
│   - No symbolic link following                                │
├──────────────────────────────────────────────────────────────┤
│ Layer 1: Binary Integrity                                     │
│   - Authenticode code signing                                 │
│   - Embedded PDFium (statically linked or verified DLL)      │
│   - Manifest with requestedExecutionLevel                     │
│   - DLL search order protection                               │
└──────────────────────────────────────────────────────────────┘
```

### 3.2 PDFium Sandboxing Considerations

```
RECOMMENDATION: Run PDFium operations in a restricted worker process.

Architecture:
  ┌──────────────────┐         IPC          ┌──────────────────┐
  │  Main Process     │  ←────────────────→  │  PDFium Worker    │
  │  (UI, Controls)   │  Shared memory       │  (PDF Processing) │
  │                   │  + named pipes       │                   │
  │  - Win32 Message  │                      │  - FPDF_LoadDoc   │
  │    loop           │                      │  - FPDF_RenderPage│
  │  - Tile display   │  Bitmap handles      │  - FPDF_GetText   │
  │  - User input     │  passed as SHM       │  - FPDF_SaveAs    │
  └──────────────────┘                      └──────────────────┘

Worker restrictions (via Job Objects):
  - Memory limit: configurable (e.g., 2GB)
  - No network access (block all sockets)
  - No clipboard access
  - File system: only temp directory + explicitly opened files
  - No process creation
  - No registry access
  - CPU quota: 80% of single core
```

### 3.3 Input Validation Rules

```
RECOMMENDATION: Validate ALL inputs before passing to PDFium:

┌────────────────────────┬──────────────────────────────────────┐
│ Input                  │ Validation                            │
├────────────────────────┼──────────────────────────────────────┤
│ File path              │ Canonicalize, check it's within       │
│                        │ allowed directories, reject symlinks │
├────────────────────────┼──────────────────────────────────────┤
│ File size              │ Max 100GB (product requirement)      │
│                        │ Reject files claiming >100GB header  │
├────────────────────────┼──────────────────────────────────────┤
│ PDF header             │ Verify %PDF- magic bytes              │
│                        │ Reject files without valid header    │
├────────────────────────┼──────────────────────────────────────┤
│ Page count             │ Cap at reasonable maximum (100,000)   │
│                        │ Reject if PDF claims more            │
├────────────────────────┼──────────────────────────────────────┤
│ Embedded file names    │ Sanitize: reject path separators,    │
│                        │ reject "..", reject absolute paths   │
├────────────────────────┼──────────────────────────────────────┤
│ URI actions            │ Block all automatic navigation        │
│                        │ Prompt user before opening            │
├────────────────────────┼──────────────────────────────────────┤
│ JavaScript             │ Block ALL execution                    │
│                        │ Strip JS streams from forms           │
├────────────────────────┼──────────────────────────────────────┤
│ Search input           │ Sanitize: UTF-8 valid, max length     │
│                        │ No regex DoS (ReDoS) patterns        │
├────────────────────────┼──────────────────────────────────────┤
│ Metadata strings       │ Sanitize: max length 64KB per field   │
│                        │ No null bytes, valid UTF-8           │
└────────────────────────┴──────────────────────────────────────┘
```

### 3.4 Safe File Handling

```cpp
// RECOMMENDATION: Secure temp file management

class SecureTempFileManager {
public:
    // Creates a temp file with restrictive ACLs
    // Owner-only read/write, no inheritance
    Result<TempFileHandle> CreateSecureTemp(
        const std::wstring& prefix,
        size_t max_size_bytes = 100ULL * 1024 * 1024 * 1024  // 100GB
    );

    // Secure deletion: DoD 5220.22-M short (3-pass)
    Result<void> SecureDelete(TempFileHandle handle);

    // Get the secure temp directory path
    std::filesystem::path GetSecureTempDir() const;

private:
    // Creates directory with restrictive ACL
    Result<void> InitializeSecureTempDir();

    // Sets ACL: owner full control, no other access
    Result<void> SetRestrictiveAcl(const std::filesystem::path& path);

    // 3-pass overwrite: 0x00, 0xFF, random
    Result<void> OverwriteFile(const std::filesystem::path& path);
};

// Secure temp directory location:
//   %LOCALAPPDATA%\PDF Elite\Temp\
//   ACL: S-1-5-21-... (current user) only
```

### 3.5 PDF Processing Security Rules

```
RECOMMENDATION: Hard rules for PDF processing in C++:

Rule 1: NO JavaScript execution
  - Never call FPDFDoc_SetJavaScriptAction
  - Never evaluate any JavaScript embedded in PDFs
  - Strip JavaScript from form fields on import

Rule 2: NO automatic network access
  - Never resolve URI actions automatically
  - Show "This PDF contains links" notification
  - User must explicitly click to open external links
  - All links open in user's default browser, not in-app

Rule 3: NO embedded file auto-extraction
  - List embedded files but never extract automatically
  - User must explicitly choose to extract
  - Scan extracted files with Windows Defender before opening
  - Warn about executable file types

Rule 4: Memory limits
  - Per-document: max 2GB working set in PDFium worker
  - Per-render: max 256MB per page render operation
  - Per-text-extraction: max 64MB output buffer
  - Abort operation if limits exceeded

Rule 5: Timeout limits
  - Page render: 10 seconds, then abort and show placeholder
  - Text extraction: 30 seconds per document
  - File save: 60 seconds per GB of output
  - Search: 5 seconds per page batch

Rule 6: No privileged operations
  - Never run as administrator
  - Manifest: requestedExecutionLevel = asInvoker
  - No COM elevation requests
```

### 3.6 Binary Hardening

```
RECOMMENDATION: Compiler and linker flags for MSVC:

# Compiler flags:
/GS           # Buffer security check (stack cookies)
/sdl          # Enable SDL security checks
/DYNAMICBASE   # ASLR
/HIGHENTROPYVA # 64-bit ASLR entropy
/CETCOMPAT    # Control-flow Enforcement Technology (x64)
/Zc:strictStrings # Strict string literal conversion
/Zc:wchar_t    # wchar_t as built-in type
/W4           # High warning level
/WX           # Warnings as errors

# Linker flags:
/DYNAMICBASE   # ASLR (linker)
/NXCOMPAT     # DEP compatible
/HIGHENTROPYVA # 64-bit ASLR entropy (linker)
/GUARD:CF     # Control Flow Guard
/INTEGRITYCHECK # Code integrity check
/ALLOWISOLATION # Manifest isolation awareness

# Manifest:
<requestedExecutionLevel level="asInvoker" uiAccess="false"/>
<loadFromResourceSources>true</loadFromResourceSources>
```

### 3.7 DLL Security

```
RECOMMENDATION: Prevent DLL side-loading:

1. Static linking:
   - Prefer static linking for PDFium and all dependencies
   - Eliminates DLL side-loading entirely
   - Trade-off: larger binary, but much more secure

2. If dynamic linking required:
   - Use LoadLibraryEx with LOAD_LIBRARY_SEARCH_APPLICATION_DIR
   - Call SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)
   - Embed manifest with <probing> element pointing to app directory
   - Verify PDFium DLL hash before loading

3. Manifest-based protection:
   - Include dependentAssembly entries for all DLLs
   - Specify exact version numbers
   - Windows will validate DLL identity before loading
```

### 3.8 Code Signing

```
RECOMMENDATION: Authenticode code signing:

  1. Sign all executables and DLLs with EV code signing certificate
  2. Enable SmartScreen reputation (requires sufficient signing history)
  3. Timestamp signatures for long-term validity
  4. Sign PDFium DLL separately if dynamically linked
  5. Verify signatures at startup (optional, for paranoid mode)

  Build pipeline:
    build.exe → sign.exe → timestamp → release
    build.dll  → sign.dll  → timestamp → release
```

---

## 4. Security Testing Requirements

### 4.1 Fuzz Testing

```
RECOMMENDATION: Continuous fuzzing of PDF parsing:

  Tool: libFuzzer with PDFium harness
  Corpus:
    - Public PDF fuzzing corpora (Google Project Zero, etc.)
    - Generated PDFs with edge-case structures
    - Malformed PDFs from CVE databases
  Coverage:
    - All PDFium API entry points used by the application
    - Custom file access handler
    - Metadata parsing
    - Annotation handling
  Integration:
    - Run in CI/CD on every commit
    - Address new crashes within 48 hours
```

### 4.2 Static Analysis

```
RECOMMENDATION: Static analysis in CI pipeline:

  Tool: Microsoft PREfast (/analyze) + Clang-Tidy + Coverity
  Checks:
    - Buffer overflow risks
    - Integer overflow risks
    - Use-after-free
    - Double-free
    - Null pointer dereference
    - Information disclosure
    - Privilege escalation paths
```

### 4.3 Security Audit Checklist

| Area | Check | Status |
|------|-------|--------|
| Input validation | All file paths sanitized | ⬜ |
| Input validation | PDF headers verified before PDFium | ⬜ |
| Input validation | Size limits enforced | ⬜ |
| Memory safety | RAII wrappers for all PDFium handles | ⬜ |
| Memory safety | No raw pointers to PDFium objects | ⬜ |
| Binary hardening | ASLR, DEP, CFG enabled | ⬜ |
| Binary hardening | Stack cookies (/GS) | ⬜ |
| Binary hardening | SDL checks enabled | ⬜ |
| DLL security | Static linking or secure loading | ⬜ |
| DLL security | DLL search order protection | ⬜ |
| Temp files | Secure directory with ACLs | ⬜ |
| Temp files | Secure deletion (3-pass overwrite) | ⬜ |
| Code signing | Authenticode signature | ⬜ |
| Network | No outbound connections from PDF processing | ⬜ |
| JavaScript | All PDF JavaScript disabled | ⬜ |
| Privilege | Runs as standard user only | ⬜ |
| Fuzzing | Continuous fuzzing pipeline | ⬜ |
| Static analysis | PREfast + Clang-Tidy in CI | ⬜ |

---

## 5. Comparison: Current vs. Proposed

| Security Aspect | Current (Tauri/React/Java) | Proposed (C++/Win32/PDFium) |
|----------------|---------------------------|------------------------------|
| Code execution in PDFs | Browser context (inherently risky) | Explicitly disabled |
| PDF parsing sandbox | Browser sandbox only | Dedicated worker process + Job Object |
| TLS cert verification | ⚠️ Disabled in auth flow | Not applicable (no auth) |
| DLL side-loading | N/A (Electron packages differently) | Static linking / manifest protection |
| Temp file security | OS default temp | Secure directory + ACLs + wipe |
| Binary hardening | Chromium defaults | MSVC /GS, ASLR, DEP, CFG, CET |
| Code signing | Tauri updater signature | Authenticode EV signing |
| Input validation | Minimal (Spring defaults) | Explicit pre-PDFium validation |
| Attack surface | 4 memory domains | 2 processes (UI + worker) |

---

*Next: See [ERROR_HANDLING.md](ERROR_HANDLING.md) for error handling architecture.*
