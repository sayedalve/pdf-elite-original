# Crash Handling Test Results

## Implementation Details
- **Architecture**: Integrated an Unhandled Exception Filter (`SetUnhandledExceptionFilter`) that executes safely in-process to catch native C++ crashes.
- **Dump Type**: A standard `MiniDumpNormal` dump via `MiniDumpWriteDump` from `dbghelp.dll`. This minimizes footprint while preserving the thread stacks, registers, and module lists.
- **Safe Execution**: Constructing paths and formatting unique dump names is handled statically avoiding potentially broken heap allocations in a crashed state.

## Dump Specifications
- **Dump Format**: Windows Minidump (.dmp)
- **Dump Location**: `%LOCALAPPDATA%\PDFElite\crashes\`
- **Unique Filename Scheme**: `PDFElite_Crash_<arch>_<config>_<YYYYMMDD>_<HHMMSS>.dmp` (e.g., `PDFElite_Crash_x64_Release_20260817_161958.dmp`)

## Test Procedure & Results
1. **Crash Invocation**: Run `build\src\app\Release\PDFElite.exe --dev --test-crash` to intentionally dereference a null pointer during startup initialization.
2. **Result**: The application abruptly exited (as expected) without throwing arbitrary dialogs or recursion.
3. **Dump Verification**: The `%LOCALAPPDATA%\PDFElite\crashes\` directory was successfully populated with a non-zero byte `.dmp` file accurately reflecting the crash architecture and timestamp.

## Tests Performed
- [x] Crash dump creation successfully triggered.
- [x] Correct dump location verified (`%LOCALAPPDATA%`).
- [x] Unique filenames format verified.
- [x] Normal application startup verified unaffected without `--dev --test-crash`.
- [x] No crash handler recursion.

## Known Limitations
- The crash handler currently executes in-process inside the same crashed process. For catastrophic memory corruption (e.g., severe heap corruption or stack overflow), it is possible the exception filter itself could fail to execute. In modern robust architectures (like Chromium's Crashpad), crash reporting is offloaded to a separate secondary process.
- No user-facing UI currently informs the user that a crash occurred (it exits silently and writes the dump).
