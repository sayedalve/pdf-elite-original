# PDF Elite — Tauri Rust Layer Comprehensive Analysis

> Repository: https://github.com/sayedalve/PDF-Elite/
> Path analyzed: `frontend/editor/src-tauri/src/`
> Files: 17 Rust source files + Cargo.toml + build.rs + tauri.conf.json

---

## 1. Module Structure Overview

```
src-tauri/src/
├── main.rs                  # Entry point (7 lines) — delegates to app_lib::run()
├── lib.rs                   # App builder, plugin registration, event loop, command handler registry (269 lines)
├── commands/
│   ├── mod.rs               # Re-exports all command functions (51 lines)
│   ├── backend.rs           # Java/Spring Boot process lifecycle management (488 lines)
│   ├── auth.rs              # Authentication, OAuth PKCE, token storage (937 lines)
│   ├── connection.rs        # Connection mode, provisioning, update policy (578 lines)
│   ├── default_app.rs       # Default PDF handler registration (299 lines)
│   ├── files.rs             # File queue management for IPC (45 lines)
│   ├── local_proxy.rs       # High-performance loopback HTTP proxy (148 lines)
│   ├── platform.rs          # OS detection utility (21 lines)
│   ├── print.rs             # Native PDF printing (macOS only) (69 lines)
│   ├── updater.rs           # Auto-update with progress (272 lines)
│   └── window.rs            # Multi-window management (217 lines)
├── state/
│   ├── mod.rs               # Re-exports (1 line)
│   └── connection_state.rs  # AppConnectionState, ConnectionMode, ServerConfig (41 lines)
└── utils/
    ├── mod.rs               # Re-exports (5 lines)
    ├── logging.rs           # In-memory + file logging, get_tauri_logs command (90 lines)
    └── paths.rs             # Platform-specific app data and provisioning paths (32 lines)
```

**Total: ~3,600 lines of Rust**

---

## 2. All Tauri Commands (IPC Handlers)

These are the 28 functions registered in `lib.rs` via `tauri::generate_handler![]` and callable from the frontend via `@tauri-apps/api/core invoke()`.

### Backend Process
| Command | Signature | Description |
|---------|-----------|-------------|
| `start_backend` | `async (AppHandle, State<AppConnectionState>) -> Result<String, String>` | Launches the bundled Stirling-PDF JAR with Java |
| `get_backend_port` | `() -> Option<u16>` | Returns the dynamically-assigned port after startup |
| `get_tauri_logs` | `async () -> Result<Vec<String>, String>` | Returns the in-memory log buffer (last 100 entries) |

### Connection / Configuration
| Command | Signature | Description |
|---------|-----------|-------------|
| `get_connection_config` | `async (AppHandle, State<AppConnectionState>) -> Result<ConnectionConfig, String>` | Reads connection mode, server config, lock flag from Tauri Store |
| `set_connection_mode` | `async (AppHandle, State, ConnectionMode, Option<ServerConfig>, Option<bool>) -> Result<(), String>` | Writes connection mode; respects lock flag |
| `is_first_launch` | `async (AppHandle) -> Result<bool, String>` | Checks if setup wizard has been completed |
| `reset_setup_completion` | `async (AppHandle) -> Result<(), String>` | Forces setup wizard on next launch |
| `get_update_mode` | `async (AppHandle) -> Result<UpdateModeInfo, String>` | Returns update policy (Prompt/Auto/Disabled) + locked status |
| `set_update_mode` | `async (AppHandle, UpdateMode) -> Result<(), String>` | Changes update policy; blocked when locked by provisioning |

### Authentication
| Command | Signature | Description |
|---------|-----------|-------------|
| `login` | `async (server_url, username, password, mfa_code, supabase_key, saas_server_url) -> Result<LoginResponse, String>` | Dual-mode login: Supabase (SaaS) or Spring Boot (self-hosted) |
| `start_oauth_login` | `async (AppHandle, provider, auth_server_url, supabase_key, success_html, error_html) -> Result<OAuthCallbackResult, String>` | Full PKCE OAuth flow with localhost callback server |
| `save_auth_token` | `async (AppHandle, token) -> Result<(), String>` | Stores auth token (keyring primary, Tauri Store fallback) |
| `get_auth_token` | `async (AppHandle) -> Result<Option<String>, String>` | Retrieves auth token |
| `clear_auth_token` | `async (AppHandle) -> Result<(), String>` | Deletes auth token from both stores |
| `save_refresh_token` | `async (AppHandle, token) -> Result<(), String>` | Stores refresh token (keyring primary, Tauri Store fallback) |
| `get_refresh_token` | `async (AppHandle) -> Result<Option<String>, String>` | Retrieves refresh token |
| `clear_refresh_token` | `async (AppHandle) -> Result<(), String>` | Deletes refresh token from both stores |
| `save_user_info` | `async (AppHandle, username, email) -> Result<(), String>` | Stores user info in Tauri Store |
| `get_user_info` | `async (AppHandle) -> Result<Option<UserInfo>, String>` | Retrieves user info |
| `clear_user_info` | `async (AppHandle) -> Result<(), String>` | Deletes user info |

### File Handling
| Command | Signature | Description |
|---------|-----------|-------------|
| `get_opened_files` | `async () -> Result<Vec<String>, String>` | Returns all queued file paths |
| `pop_opened_files` | `async () -> Result<Vec<String>, String>` | Atomically returns and clears queued file paths |
| `clear_opened_files` | `async () -> Result<(), String>` | Clears the file queue |

### Window Management
| Command | Signature | Description |
|---------|-----------|-------------|
| `open_in_new_window` | `async (AppHandle, paths) -> Result<String, String>` | Spawns a new webview window with files pre-queued |
| `open_files_in_new_window` | `async (AppHandle, file_ids) -> Result<String, String>` | Opens stored files (by ID) in a new window |
| `pop_window_file_ids` | `async (WebviewWindow) -> Result<Vec<String>, String>` | Returns file IDs queued for the calling window |

### Default App / Platform
| Command | Signature | Description |
|---------|-----------|-------------|
| `is_default_pdf_handler` | `() -> Result<bool, String>` | Checks if app is the system's default PDF handler |
| `set_as_default_pdf_handler` | `() -> Result<String, String>` | Registers/unregisters as default PDF handler |
| `get_desktop_os` | `() -> DesktopOS` | Returns the current OS (macos/windows/linux/unknown) |

### PDF Proxy
| Command | Signature | Description |
|---------|-----------|-------------|
| `proxy_local_pdf_request` | `async (Request) -> Result<Response, String>` | Binary-optimized loopback proxy for backend HTTP calls |

### Printing
| Command | Signature | Description |
|---------|-----------|-------------|
| `print_pdf_file_native` | `(AppHandle, file_path, title) -> Result<(), String>` | Native PDF printing via PDFKit (macOS only) |

### Auto-Update
| Command | Signature | Description |
|---------|-----------|-------------|
| `check_for_update` | `async (AppHandle) -> Result<Option<UpdateInfo>, String>` | Checks for available updates |
| `download_and_install_update` | `async (AppHandle) -> Result<(), String>` | Downloads and installs update with progress events |
| `restart_app` | `(AppHandle)` | Restarts the process to apply update (never returns) |
| `can_install_updates` | `() -> CanInstallResult` | Checks write permissions on install directory (Windows) |
| `get_app_version` | `(AppHandle) -> String` | Returns the current app version string |

---

## 3. State Management

### 3.1 Tauri-Managed State
- **`AppConnectionState`** (`state/connection_state.rs`): A `Mutex<ConnectionState>` wrapping `ConnectionMode` (SaaS/SelfHosted/Local), an optional `ServerConfig` (URL string), and a `lock_connection_mode` bool. Registered via `.manage(AppConnectionState::default())` in `lib.rs`.

### 3.2 Global Static Mutexes
All use `std::sync::Mutex` — no async-aware locking (Tokio `Mutex` is only used for the updater's progress counter).

| Static | Type | Module | Purpose |
|--------|------|--------|--------|
| `BACKEND_PROCESS` | `Mutex<Option<CommandChild>>` | `commands/backend.rs` | Holds the Java process handle |
| `BACKEND_STARTING` | `Mutex<bool>` | `commands/backend.rs` | Guards against concurrent startup |
| `BACKEND_PORT` | `Mutex<Option<u16>>` | `commands/backend.rs` | Stores the detected port from stdout |
| `OPENED_FILES` | `Mutex<Vec<String>>` | `commands/files.rs` | File paths queued for the frontend |
| `NEXT_WINDOW_ID` | `AtomicU32` | `commands/window.rs` | Monotonic counter for window labels |
| `PENDING_FILE_IDS` | `Mutex<Option<HashMap<String, Vec<String>>>>` | `commands/window.rs` | Per-window stored-file ID queues |
| `BACKEND_LOGS` | `Mutex<VecDeque<String>>` | `utils/logging.rs` | In-memory circular log buffer (100 entries) |
| `CLIENT` | `OnceLock<Client>` | `commands/local_proxy.rs` | Process-wide reqwest HTTP client (connection pooling) |

### 3.3 Persistent Storage (Tauri Store / Keyring)

**Tauri Store** (`tauri-plugin-store`): JSON files in the app data directory.
- `connection.json`: `setup_completed`, `connection_mode`, `server_config`, `lock_connection_mode`, `login_agreement_enabled`, `update_mode`, `update_mode_locked`, `user_info`
- `tokens.json`: `auth_token`, `refresh_token` (fallback when OS keyring is unavailable)

**OS Keyring** (`keyring` crate): Primary token storage.
- Service: `"stirling-pdf"`
- Keys: `"auth-token"`, `"refresh-token"`
- Falls back to Tauri Store when keyring write doesn't round-trip correctly (e.g., managed Windows environments with GPO restrictions)

---

## 4. Backend Process Management (Java/Spring Boot)

The application bundles a **Stirling-PDF** Java Spring Boot application. The lifecycle is managed in `commands/backend.rs`.

### 4.1 Startup Flow
1. During `lib.rs::setup()`, `start_backend()` is spawned as a non-blocking async task.
2. `start_backend` checks connection mode (SaaS/SelfHosted/Local all start the local backend).
3. Guards against duplicate starts via `BACKEND_STARTING` and `BACKEND_PROCESS` mutexes.
4. Locates the bundled **JRE** at `resource_dir/runtime/jre/bin/java{.exe}`.
5. Locates the **Stirling-PDF JAR** in `resource_dir/libs/` (case-insensitive `*stirling-pdf*.jar`, sorted reverse-alphabetically for latest version).
6. Normalizes Windows UNC paths (`\\?\` prefix).
7. Creates platform-specific directories under `app_data_dir/`: `configs/`, `logs/`, and uses the data dir as working directory.
8. Migrates legacy `workspace/` subdirectory contents into the app data root (one-time migration, then deletes the old dir).
9. Launches Java with these flags:
   - `-Xmx2g` (2GB heap)
   - `-Dserver.port=0` (OS assigns a free port)
   - `-DBROWSER_OPEN=false`
   - `-DSTIRLING_PDF_TAURI_MODE=true`
   - `-Dsecurity.enableLogin=false`
   - `-Dsecurity.csrfDisabled=true`
   - `-Dserver.forward-headers-strategy=none` (anti-SSRF: prevents X-Forwarded-For spoofing)
   - `-Dlogging.file.path=<log_dir>`
   - `-Dlogging.file.name=stirling-pdf.log`
   - `-Dlegal.loginAgreement.enabled=true` (conditionally, if provisioned)
10. Sets env vars: `TAURI_PARENT_PID`, `STIRLING_PDF_CONFIG_DIR`, `STIRLING_PDF_LOG_DIR`, `STIRLING_PDF_WORK_DIR`.
11. Uses `tauri-plugin-shell` to spawn the process and captures stdout/stderr.
12. Monitors output on a separate Tokio task, parsing `"running on port: PORT"` to discover the assigned port.

### 4.2 Shutdown
- `cleanup_backend()` is called on `RunEvent::ExitRequested`.
- Calls `child.kill()` on the stored `CommandChild`.
- No graceful shutdown signal is sent (SIGTERM equivalent) — it's a hard kill.

### 4.3 Port Discovery
- The backend starts with `-Dserver.port=0` (ephemeral port).
- The `monitor_backend_output` task parses stdout for `"running on port: "` and extracts the port number.
- The frontend polls `get_backend_port` until it returns `Some(port)`, then configures its API base URL.

---

## 5. File Handling

### 5.1 File Opening Pathways
Files reach the frontend through **four** pathways, all converging into the same `OPENED_FILES` queue:

1. **CLI arguments at first launch**: Parsed in `setup()` via `parse_launch_files()`, which filters for paths that exist on disk.
2. **Second instance (single-instance plugin)**: When a second launch is attempted, the `tauri_plugin_single_instance` callback receives the CLI args, parses files, and calls `forward_files_to_window()`.
3. **Drag-and-drop**: `RunEvent::WindowEvent::DragDrop` captures dropped paths and calls `forward_files_to_window()`.
4. **macOS file-open event**: `RunEvent::Opened` decodes `file://` URLs (with URL percent-encoding handling) and routes to the focused window.
5. **Deep links**: `stirlingpdf://` scheme URLs are dispatched via `deep-link` plugin, emitting `deep-link` events to the frontend.

### 5.2 File Queue Architecture
- **Disk paths**: Stored in the global `OPENED_FILES: Mutex<Vec<String>>`. The frontend calls `pop_opened_files()` (atomic get-and-clear) or `get_opened_files()` + `clear_opened_files()`.
- **Stored file IDs** (IndexedDB references): Stored per-window in `PENDING_FILE_IDS: Mutex<Option<HashMap<String, Vec<String>>>>`. Each new window pops its own IDs via `pop_window_file_ids()`.
- **Event notification**: `files-changed` and `window-files-ready` events are emitted to specific windows to trigger the frontend to consume queued files.

### 5.3 Window Routing Strategy
`target_window_label()` selects where to send files: focused window → main window → any open window. This prevents spawning duplicate windows when the user double-clicks a PDF while the app is running.

---

## 6. Window Management

### 6.1 Multi-Window Architecture
- The app uses Tauri's multi-window model: a single Rust backend process, multiple WebView frontend instances.
- All windows share the same backend and the same IndexedDB (via WebView2 user-data folder on Windows).

### 6.2 Window Creation
- `build_window()` is the centralized window builder ensuring all windows have identical configuration:
  - Title: "Stirling-PDF"
  - Size: 1280×800, min 1030×600
  - On Windows: `--enable-features=CertVerifierBuiltinFeature` (required for all WebView2 instances sharing a user-data dir).
- Window creation **must** happen on the main thread (WebView2 requirement). `run_on_main_thread_result()` helper hops from Tauri's worker thread to the main thread using `tokio::sync::oneshot`.
- Window labels: `"main"` for the first, `"main-2"`, `"main-3"`, etc. for subsequent (via `AtomicU32` counter).

### 6.3 File Association
- `tauri.conf.json` registers `.pdf` as a file association (role: Editor, mimeType: application/pdf).
- The main window starts invisible (`visible: false`) — the frontend shows it after setup.
- `dragDropEnabled: false` in tauri.conf.json; instead, drag-drop is handled at the Rust level via `RunEvent::DragDrop` for more control.

---

## 7. Authentication / Connection Handling

### 7.1 Connection Modes
Three modes stored in `connection_state.rs`:
- **SaaS**: Cloud-hosted Stirling-PDF via Supabase authentication.
- **SelfHosted**: Connect to a user-specified Spring Boot server URL.
- **Local**: Use only the bundled local backend (no remote server).

### 7.2 Login Flow (auth.rs — `login` command)
A single `login()` command handles both authentication backends:
- **Detection**: Compares the `server_url` against `saas_server_url` to auto-detect the backend type.
- **Supabase (SaaS)**: POST to `{server}/auth/v1/token?grant_type=password` with email/password. Returns `access_token` + `refresh_token`.
- **Spring Boot (Self-Hosted)**: POST to `{server}/api/v1/auth/login` with username/password + optional MFA code. Returns `access_token` only.
- Both use `reqwest` with `danger_accept_invalid_certs(true)` (accepts self-signed certificates) and a 30-second timeout.
- Rich error messages: TLS version mismatch, connection refused, DNS failure, timeout — all mapped to user-friendly strings.

### 7.3 OAuth PKCE Flow (auth.rs — `start_oauth_login`)
1. Generates a 128-character random `code_verifier` and computes SHA-256 `code_challenge` (base64url encoded, no padding).
2. Starts a `tiny_http` server on `127.0.0.1:0` (OS-assigned port).
3. Opens the system browser with the Supabase authorize URL containing `code_challenge` and the callback URL.
4. Waits up to 120 seconds for the callback on the local HTTP server.
5. Parses the `code` from the callback URL query parameters.
6. Exchanges the authorization code for tokens via POST to `{server}/auth/v1/token?grant_type=pkce`.
7. Returns `OAuthCallbackResult` with `access_token`, `refresh_token`, and `expires_in`.

### 7.4 Token Storage Strategy
- **Primary**: OS-native keyring (macOS Keychain, Windows Credential Manager) via the `keyring` crate with platform-specific features (`apple-native`, `windows-native`).
- **Fallback**: Tauri Store JSON file (for dev mode, managed environments, or when keyring is unavailable).
- **Keyring verification**: On save, a round-trip read is performed. If it doesn't match, the keyring write is deemed unreliable and the Tauri Store fallback is used instead. Stale fallback data is cleaned up when the keyring is authoritative.
- **Test hooks**: `STIRLING_PDF_TEST_FORCE_AUTH_KEYRING_FAIL` and `STIRLING_PDF_TEST_FORCE_REFRESH_KEYRING_FAIL` env vars force keyring failures for testing the fallback path.

---

## 8. PDF Proxy / Request Forwarding

### 8.1 Motivation
The `@tauri-apps/plugin-http` plugin serializes request/response bodies as JSON arrays of byte values (~3.5× size increase plus heavy GC pressure). For large PDF uploads/downloads, this is a significant performance problem.

### 8.2 Solution (local_proxy.rs)
A dedicated `proxy_local_pdf_request` command uses `InvokeBody::Raw` to transfer binary data efficiently across the IPC bridge.

**Frame format:** `[u32 BE meta_len][meta JSON (UTF-8)][raw body bytes]`

The frontend sends a single `ArrayBuffer` containing the serialized metadata JSON followed by the raw body. The Rust side parses the frame, makes the HTTP request, and returns the response in the same frame format.

### 8.3 Security Measures
- **Loopback-only enforcement**: The URL's host is parsed to an `IpAddr` and checked with `is_loopback()`. This covers the entire 127.0.0.0/8 range and `::1`. Domain `localhost` is also accepted (case-insensitive).
- **No redirect following**: `Policy::none()` disables HTTP redirects, preventing SSRF via 3xx responses pointing to non-loopback hosts.
- **Connection pooling**: A `OnceLock<Client>` creates a single process-wide reqwest client, keeping connections alive across calls.
- **Header validation**: Individual header parsing is done per-pair; a single malformed header doesn't fail the entire request.

---

## 9. Error Handling Patterns

### 9.1 Tauri Command Errors
All commands return `Result<T, String>` — errors are stringly-typed. This is a common Tauri pattern where the error string is serialized and surfaced to the frontend.

### 9.2 Error Propagation
- `map_err(|e| format!("...: {}", e))` — the dominant pattern. Every fallible operation is mapped to a descriptive string.
- Mutex operations use `.lock().unwrap()` throughout (no error handling for poison — if a mutex is poisoned, the app panics).

### 9.3 Specific Error Handling
- **Backend startup**: Uses a `BACKEND_STARTING` flag that must be manually reset via `reset_starting_flag()` on error paths — if forgotten, the backend can never be restarted.
- **Login**: Rich TLS/network error classification (version mismatch, cert issues, DNS, timeout, connection refused).
- **Keyring**: Best-effort with fallback. Keyring failures never block disk-based operations. On clear, failed deletion is followed by overwriting with empty string.
- **Update check**: Errors are swallowed as `Ok(None)` — the frontend never sees error banners for transient network failures.

### 9.4 Logging on Errors
Every error path includes `add_log()` or `log::error!()` calls, making failures diagnosable from the in-memory log buffer and the persistent log file.

---

## 10. Dependencies (Cargo.toml)

### Core Framework
| Dependency | Version | Purpose |
|-----------|---------|---------|
| `tauri` | 2.10.3 | Application framework (with `devtools` feature) |
| `tauri-build` | 2.5.3 | Build-time Tauri configuration |

### Tauri Plugins
| Plugin | Version | Purpose |
|--------|---------|---------|
| `tauri-plugin-log` | 2.8.0 | Structured logging |
| `tauri-plugin-shell` | 2.3.4 | Process spawning (Java backend) |
| `tauri-plugin-fs` | 2.5.0 | File system access |
| `tauri-plugin-dialog` | 2.7.0 | Native file dialogs |
| `tauri-plugin-http` | 2.5.8 | HTTP client from frontend (with `dangerous-settings` for cert bypass) |
| `tauri-plugin-single-instance` | 2.4.1 | Prevent multiple app instances (with `deep-link`) |
| `tauri-plugin-store` | 2.4.2 | Persistent JSON key-value storage |
| `tauri-plugin-opener` | 2.5.3 | Open URLs/files in system apps |
| `tauri-plugin-deep-link` | 2.4.6 | Custom URL scheme handling (`stirlingpdf://`) |
| `tauri-plugin-notification` | 2.3.3 | System notifications |
| `tauri-plugin-updater` | 2.5.0 | Auto-update functionality |
| `tauri-plugin-window-state` | 2.2.1 | Remember window size/position |

### Libraries
| Dependency | Version | Purpose |
|-----------|---------|---------|
| `serde` / `serde_json` | 1.0 | Serialization/deserialization |
| `reqwest` | 0.12 | HTTP client (with `rustls-tls`, `rustls-tls-native-roots`) |
| `tokio` | 1.50 | Async runtime (features: `time`, `sync`) |
| `tiny_http` | 0.12 | Lightweight HTTP server for OAuth callback |
| `keyring` | 3.6.1 | OS-native credential storage (apple-native, windows-native) |
| `sha2` | 0.11 | PKCE code_challenge generation |
| `base64` | 0.23 | Base64url encoding (no padding variant) |
| `rand` | 0.10 | PKCE code_verifier generation |
| `url` | 2.5 | URL parsing |
| `urlencoding` | 2.1 | URL encoding for OAuth parameters |
| `log` | 0.4 | Logging facade |

### Platform-Specific
| Dependency | Platform | Purpose |
|-----------|----------|---------|
| `core-foundation` / `core-services` | macOS | LaunchServices API for default PDF handler |
| `objc2` / `objc2-app-kit` | macOS | NSPrintInfo, NSPrintOperation for printing |
| `objc2-foundation` | macOS | NSString, NSURL for macOS APIs |
| `objc2-pdf-kit` | macOS | PDFDocument, PDFPrintScalingMode for native PDF printing |
| `windows` | Windows | COM, Shell APIs for default PDF handler check |

### Notable Absences
- No database crate (all persistence via Tauri Store JSON files and OS keyring)
- No image processing (handled by the Java backend)
- No WebSocket library (communication is purely HTTP-based)

---

## 11. Build Configuration

### 11.1 build.rs
Minimal — just `tauri_build::build()`. No custom build scripts, conditional compilation, or code generation.

### 11.2 Cargo.toml Configuration
- **Edition**: 2021
- **Rust version**: 1.77.2 (minimum)
- **Lint level**: `warnings = "deny"` — all Rust warnings are treated as errors
- **Crate type**: `staticlib`, `cdylib`, `rlib` (standard Tauri library configuration for cross-platform compatibility)
- **TLS**: Uses `rustls-tls` with `rustls-tls-native-roots` (no OpenSSL dependency — pure Rust TLS)

### 11.3 tauri.conf.json Highlights
- **Product name**: "PDF Elite"
- **Version**: 2.14.2
- **Identifier**: `com.pdfelite.desktop`
- **Main window**: 1280×800, resizable, starts invisible, drag-drop disabled at config level (handled in Rust)
- **Bundled resources**: `libs/*.jar` and `runtime/jre/**/*` — the Java backend and JRE are bundled inside the app package
- **File associations**: `.pdf` → role "Editor"
- **Deep link scheme**: `stirlingpdf://`
- **Updater endpoint**: GitHub releases (`latest.json`)
- **Updater install mode**: `passive` (Windows)
- **Linux packaging**: deb, rpm (zstd compression level 3), AppImage
- **macOS**: Minimum 10.15, custom Info.plist
- **Windows**: SHA-256 digest, DigiCert timestamp, WiX installer with provisioning fragment
- **Shell plugin**: `open: true` (allows opening URLs in browser)
- **FS plugin**: `requireLiteralLeadingDot: false` (allows dotfile access)

---

## 12. Security-Related Code

### 12.1 Positive Security Measures
| Area | Implementation | File |
|------|---------------|------|
| **Token storage** | OS-native keyring with Tauri Store fallback; keyring verification round-trip | `auth.rs` |
| **PKCE OAuth** | Full Proof Key for Code Exchange implementation (SHA-256 challenge, 128-char verifier) | `auth.rs` |
| **Local proxy SSRF prevention** | Loopback-only URL validation via `IpAddr::is_loopback()` + redirect disable | `local_proxy.rs` |
| **Forward-header spoofing** | `-Dserver.forward-headers-strategy=none` prevents X-Forwarded-For abuse | `backend.rs` |
| **Local mode security** | Login disabled (`-Dsecurity.enableLogin=false`), CSRF disabled for desktop mode | `backend.rs` |
| **Provisioning lock** | Admin-only system paths (ProgramData, /etc, /Library) can lock settings; user paths cannot | `connection.rs` |
| **Connection mode lock** | When locked, `set_connection_mode` preserves existing settings | `connection.rs` |
| **Certificate validation** | TLS 1.2+ only (rustls); self-signed cert acceptance for self-hosted mode | `auth.rs`, `reqwest` config |

### 12.2 Security Concerns
| Concern | Details | Severity |
|---------|---------|----------|
| **`danger_accept_invalid_certs(true)`** | The login and OAuth HTTP clients accept ANY certificate, including expired and mismatched ones. This is intentional for self-hosted deployments but could enable MITM attacks on the SaaS path. | Medium |
| **`dangerous-settings` feature** | `tauri-plugin-http` is loaded with the `dangerous-settings` feature, which allows the frontend to make arbitrary HTTP requests with custom configurations. | Medium |
| **Mutex `.unwrap()` everywhere** | Any poisoned mutex causes a panic. In production, this means a thread panic during lock-hold crashes the app rather than degrading gracefully. | Low |
| **Token in log output** | The `Equivalent command` log line in `backend.rs` could theoretically leak env vars, though it only logs `TAURI_PARENT_PID`. However, `log::debug!("PKCE code_verifier generated: {} chars")` is safe (length only). | Low |
| **OAuth callback server** | Binds to `127.0.0.1:0` (good), but the 120-second timeout could allow a local attacker to race for the callback. The PKCE code_verifier mitigates this. | Low |
| **Hardcoded bundle ID** | `"stirling.pdf.dev"` is hardcoded for macOS default handler checks — if the bundle ID changes, the check silently fails. | Low |
| **Updater public key in config** | The Tauri updater public key is base64-encoded in `tauri.conf.json`. If the config is tampered with pre-build, updates could be forged. Build-time integrity is the mitigation. | Low |

---

## 13. Performance-Related Code

### 13.1 Local Proxy Binary Transport
The most significant performance optimization. By using `InvokeBody::Raw` and a custom binary frame format, large PDF uploads/downloads avoid the ~3.5× overhead of JSON-array byte serialization in the standard Tauri HTTP plugin. The frame format is compact: 4-byte length prefix + JSON metadata + raw bytes.

### 13.2 Connection Pooling
- **Local proxy**: `OnceLock<Client>` creates a single process-wide reqwest client with keep-alive, reused across all proxied requests.
- **OAuth/login**: Creates a new `reqwest::Client` per call (not pooled, but these are infrequent operations).

### 13.3 Java Backend Configuration
- `-Xmx2g` allocates 2GB heap for the Java process.
- `-Dserver.port=0` lets the OS pick a free port (avoids port conflicts).
- Legacy workspace migration happens once at startup (one-time cost).

### 13.4 Logging
- In-memory log buffer is capped at 100 entries (ring buffer via `VecDeque`).
- Log file writes are append-only (no rotation, but the log level is `Info` which is moderate).

### 13.5 Window Management
- WebView2 instances share a user-data folder (requires identical `additional_browser_args`), avoiding duplicate caches.
- `AtomicU32` for window ID generation is lock-free.

### 13.6 Async Patterns
- Backend monitoring runs on a dedicated Tokio task (non-blocking).
- OAuth callback server runs on a dedicated `std::thread` (blocking, with 1-second poll timeout).
- Window creation dispatches to the main thread via `run_on_main_thread_result()` with oneshot channels.

### 13.7 Areas for Potential Improvement
- The Java backend startup is fire-and-forget with no retry mechanism.
- File path validation (`Path::new(arg).exists()`) is synchronous and could block if called with many paths.
- No HTTP response caching for the proxy layer.
- `Mutex::lock().unwrap()` could be replaced with `parking_lot::Mutex` for better performance under contention, though contention is likely minimal in this app.

---

## 14. Provisioning System (MDM/Enterprise)

A JSON provisioning file (`stirling-provisioning.json`) can be placed in:
- **User path**: `{app_data_dir}/stirling-provisioning.json` (deleted after apply)
- **System path**: Platform-specific admin-owned locations:
  - Windows: `%PROGRAMDATA%\Stirling-PDF\`
  - macOS: `/Library/Application Support/Stirling-PDF/`
  - Linux: `/etc/stirling-pdf/`

Provisioning can configure: `serverUrl`, `lockConnectionMode`, `loginAgreementEnabled`, `updateMode`.

System-path provisioning files **lock** the update mode UI (preventing user override). User-path files do **not** lock the UI (preventing accidental self-lockout). The file is deleted after apply if it's in the user path, but left in place if system-managed (for re-application on updates).

---

## 15. Platform-Specific Code

### macOS
- Native PDF printing via PDFKit (`objc2-pdf-kit`)
- Default PDF handler via LaunchServices (`LSCopyDefaultRoleHandlerForContentType`)
- Set default handler via `LSSetDefaultRoleHandlerForContentType` (deprecated but functional)
- `RunEvent::Opened` handling for file:// URLs with percent-decoding
- Deep link registration at runtime (not needed on macOS — handled by bundle metadata)

### Windows
- Default PDF handler via COM `IApplicationAssociationRegistration`
- Set default handler opens `ms-settings:defaultapps` (user manually picks)
- WiX installer with provisioning fragment
- `additionalBrowserArgs` for WebView2 shared user-data folder
- UNC path normalization for Java and JAR paths
- Update install directory write-check via probe file
- Certificate timestamping via DigiCert

### Linux
- Default PDF handler via `xdg-mime`
- Desktop file resolution from XDG data directories
- Multiple desktop file name candidates (`Stirling-PDF.desktop`, `stirling-pdf.desktop`)
- Deep link registration at runtime (Linux + Windows)

---

## 16. Key Architectural Insights

1. **Hybrid architecture**: The app is a Tauri (Rust + WebView) shell wrapping a full Java Spring Boot backend (Stirling-PDF). Rust handles desktop integration, native APIs, and IPC optimization; Java handles the actual PDF processing.

2. **Connection mode abstraction**: The same frontend can operate in SaaS, Self-Hosted, or Local mode, with the Rust layer abstracting the differences. The local backend is always started regardless of mode (for hybrid execution).

3. **Enterprise-ready provisioning**: The provisioning system supports MDM deployment with locked settings, update policy control, and admin-managed configuration files.

4. **Defense in depth for the proxy**: The local proxy is designed as a performance optimization, not a general proxy. Loopback-only enforcement + no redirects makes it safe against SSRF.

5. **Token storage resilience**: The keyring-with-fallback pattern handles diverse enterprise environments where Credential Manager may be restricted by GPO or AV policies.

6. **Test coverage**: Unit tests exist for provisioning path ownership logic (`connection.rs` has 7 tests). Auth module has test hooks via environment variables. No integration tests for the backend lifecycle.

7. **Error surface area**: All errors flow to the frontend as strings. The logging system captures everything for diagnostics. The updater intentionally swallows network errors to avoid noisy startup banners.
