# Font Handling Architecture

## Overview
PDF Elite Native utilizes a controlled fallback strategy for Unicode font handling, focusing on preserving system stability and PDF integrity without bloating the application bundle with massive font files.

## FontManager (`pdf_engine::FontManager`)
The `FontManager` is responsible for evaluating whether a text string can be correctly rendered using the current font context, and if not, providing a safe fallback font for PDFium to embed.

### DirectWrite Integration
The `FontManager` leverages Windows DirectWrite (`IDWriteFactory`) to inspect character support dynamically:
- It locates `Nirmala UI` (or other fallback fonts) from the system's `C:\Windows\Fonts` directory.
- It iterates through the target string, evaluating surrogate pairs and resolving them into Unicode code points.
- It queries the font face using `GetGlyphIndices` to verify whether the fallback font actually contains the necessary glyphs.

### PDFium Integration
PDFium does not easily allow swapping the font of an existing `FPDF_PAGEOBJECT` using public headers.
Therefore, our text insertion and editing logic adapts based on the context:

#### 1. New Text Insertion (`Add Text`)
When creating a new text object:
1. `PdfPage::InsertTextObject` performs a fast-path check for non-ASCII characters (`> 127`).
2. If non-ASCII characters are detected, it delegates to `FontManager::HasGlyphs` to ensure the fallback font supports the text.
3. If supported, `FontManager::LoadFallbackFont` loads `Nirmala.ttc` into the document via `FPDFText_LoadFont` with `cid=true` to enable Unicode encoding and ToUnicode mappings.
4. The new text object is created with this fallback font via `FPDFPageObj_CreateTextObj`.

#### 2. Existing Text Modification (`Edit Text`)
When editing an existing text object:
1. `PdfTextObject::SetText` intercepts the new text.
2. If the text contains non-ASCII characters (which would typically corrupt standard English font blocks like Arial into `ÿÿÿÿ`), the edit is gracefully rejected.
3. This prevents irreversible corruption of the document while ensuring the application remains robust.

## Fallback Fonts
Currently, the system uses **Nirmala UI** (`Nirmala.ttc`) as the standard fallback font. Nirmala UI is an official Windows font with comprehensive support for Indic scripts (including Bengali, Hindi, etc.) and is safe for embedding.

## Limitations and Future Work
- Graceful rejection of Unicode into existing standard text blocks is a safe compromise for the prototype. Future phases will implement full text block recreation in-place (extracting matrix, color, and size, deleting the old object, and inserting a new Unicode-compatible object).
- Paragraph reflow and multi-line wrapping are currently outside the scope of basic font handling and will be handled by a dedicated layout engine layer in the future.
