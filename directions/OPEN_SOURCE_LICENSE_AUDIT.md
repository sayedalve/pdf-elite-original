# Open Source License Audit

This document formally audits the licenses of the three reference projects (Phase 12). It establishes strict boundaries on how we can legally integrate their concepts into PDF Elite without violating open-source terms.

---

## 1. PDF4QT
**Project:** PDF4QT (https://github.com/JakubMelka/PDF4QT)
**Copyright:** Copyright (c) 2018-2025 Jakub Melka and Contributors
**License:** MIT License
* **Allowed use:** Commercial use, modification, distribution, private use.
* **Attribution requirements:** Must include the original copyright notice and permission notice in all copies or substantial portions of the software.
* **Compatibility with PDF Elite:** **HIGH** (Fully Compatible)
* **Can code be copied directly?:** **YES**. Snippets or mathematical transforms (e.g., text layout matrices, object property mapping) can be directly ported into our codebase, provided we include the MIT header/attribution.
* **Target Subsystems:** `PDFObjectEditorAbstractModel`, `PdfTextEditPseudoWidget`

---

## 2. Xournal++
**Project:** Xournal++ (https://github.com/xournalpp/xournalpp)
**Copyright:** Copyright (C) Xournal++ Team
**License:** GNU General Public License v2.0 or later (GPL-2.0+)
* **Allowed use:** Commercial use is allowed, *but* only if the combined/derivative work is also released under the GPL.
* **Attribution requirements:** State changes, include original copyright, include full license text, disclose source.
* **Compatibility with PDF Elite:** **INCOMPATIBLE** (Assuming PDF Elite is closed-source/proprietary or differently licensed).
* **Can code be copied directly?:** **NO**. We absolutely cannot copy/paste any C++ code from this repository into PDF Elite.
* **Can the algorithm be reimplemented?:** **YES**. We can read the code to understand the *mathematical principles* of stroke stabilization (e.g., Velocity Gaussian algorithms), shape recognition, and the granular command pattern, and then write our own independent implementations from scratch in PDF Elite.
* **Target Subsystems:** `StrokeStabilizer`, `ShapeRecognizer`, `UndoRedoHandler`.

---

## 3. Okular
**Project:** Okular (https://github.com/KDE/okular)
**Copyright:** Copyright (C) KDE Community / KDE e.V.
**License:** GNU General Public License v2.0 or later (GPL-2.0+)
* **Allowed use:** Commercial use allowed, provided derivative works are also GPL.
* **Attribution requirements:** State changes, include original copyright, include full license text, disclose source.
* **Compatibility with PDF Elite:** **INCOMPATIBLE** (Assuming PDF Elite is closed-source/proprietary or differently licensed).
* **Can code be copied directly?:** **NO**. We absolutely cannot copy/paste any C++ code from this repository into PDF Elite.
* **Can the algorithm be reimplemented?:** **YES**. We can study how they calculate zoom coordinates (`zoomWithFixedCenter`), manage memory limits (`TILES_MAXSIZE`), and extract text quad-points (`TextEntity`). We must independently rewrite these mathematical/architectural concepts for PDF Elite.
* **Target Subsystems:** `TilesManager`, `PageView` (Zoom/Scroll), `TextPage` (Selection).

---

## Final Directive (Phase 11 & 12 Enforced)
**Rule:** When building Phase B through I, developers are strictly barred from copy/pasting files from `reference/XournalPP` or `reference/Okular`. 
**Workflow:**
1. Open reference file (e.g., `StrokeStabilizer.cpp`).
2. Read and comprehend the math.
3. Close the reference file.
4. Write the equivalent native Direct2D/PDFium code in PDF Elite from scratch.
5. Document the algorithmic inspiration.
