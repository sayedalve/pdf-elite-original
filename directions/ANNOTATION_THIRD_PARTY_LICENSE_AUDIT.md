# Annotation Third Party License Audit

This document tracks all third-party open-source implementations evaluated or integrated into PDF Elite's annotation system.

## 1. Okular (KDE)
- **Project URL:** https://github.com/KDE/okular
- **License:** GPL v2.0+
- **Compatibility with PDF Elite:** PDF Elite is a proprietary product (implied by "proprietary" layers in frontend, though core is OSS). GPL is generally viral and incompatible with proprietary or differently-licensed code without an explicit exception.
- **Direct copy allowed?** NO.
- **Action:** Study algorithm design (e.g., PageViewAnnotator state machine, Hit Testing logic, Tool priorities) and reimplement independently without copying source code.

## 2. Xournal++
- **Project URL:** https://github.com/xournalpp/xournalpp
- **License:** GPL v2.0+
- **Compatibility with PDF Elite:** Incompatible for direct copy.
- **Direct copy allowed?** NO.
- **Action:** Study Freehand/Ink smoothing, Bezier curve fitting, and eraser algorithms. Reimplement logic from scratch.

## 3. PDF4QT
- **Project URL:** https://github.com/JakubMelka/PDF4QT
- **License:** LGPL v3.0 (with parts potentially GPL)
- **Compatibility with PDF Elite:** LGPL requires dynamic linking or providing object files for relinking, which is complex for direct source inclusion in a statically linked or proprietary C++ binary.
- **Direct copy allowed?** NO. (Better safe than sorry; we will not copy code directly).
- **Action:** Study annotation data models, WYSIWYG properties, and geometry handling. Reimplement independently.

## Summary
To maintain a clean IP boundary and ensure strict legal compliance, **NO SOURCE CODE WILL BE DIRECTLY COPIED** from Okular, Xournal++, or PDF4QT. All architectural patterns, data models, and interaction state machines will be studied purely for inspiration and algorithmic understanding, followed by an independent, clean-room implementation tailored specifically to PDF Elite's C++ / PDFium architecture.
