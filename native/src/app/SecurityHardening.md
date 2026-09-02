# PDF Elite Native Security Hardening Guidelines (Phase 24)

1. **JavaScript Execution**: Disabled by default in PDFium (`FPDF_DisableJS(true)`). Only enabled if user explicitly checks the setting and signs a warning dialog.
2. **External Link Handling**: Intercept all URI actions from PDFium. Prompt user before launching the default browser (`ShellExecuteW`).
3. **Embedded Files/Attachments**: Files extracted from PDFs must be saved to a temporary directory with a randomized suffix and stripped of execution flags.
4. **Memory Limits**: Restrict PDFium allocations. If a PDF asks for an unreasonable bitmap size (e.g., 50000x50000), fail gracefully instead of attempting an out-of-memory allocation.
5. **Path Validation**: When processing PDFs locally, ensure no path traversal `../` is allowed when exporting or saving.
