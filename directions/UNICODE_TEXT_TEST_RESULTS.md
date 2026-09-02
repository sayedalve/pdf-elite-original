# Unicode and Font Handling Test Results (Phase 15D)

## Prototype Execution
A standalone executable (`UnicodeFontPrototype`) was written to directly test PDFium's `FPDFText_LoadFont` and `FPDFPageObj_CreateTextObj` APIs using native Windows fonts before integrating the logic into the PDF Elite UI.

### Test Matrix
We embedded two fonts via `FPDFText_LoadFont` (`cid=true` for Unicode support):
1. **Arial** (`C:\Windows\Fonts\arial.ttf`)
2. **Nirmala UI** (`C:\Windows\Fonts\Nirmala.ttc`) - A standard Windows UI font with robust Indic script support.

Four text objects were created on a fresh page with the following combinations:
1. `Hello World (Arial)` using Arial.
2. `বাংলাদেশ (Nirmala)` using Nirmala UI.
3. `Hello বাংলাদেশ PDF (Nirmala)` using Nirmala UI.
4. `বাংলাদেশ (Arial)` using Arial.

### Results
The resulting `unicode_prototype.pdf` was successfully saved and then parsed using PyMuPDF to verify extraction and character mapping (ToUnicode).

**Extraction Output:**
1. `Hello World (Arial)` -> Success.
2. `বাংলাদেশ (Nirmala)` -> Success. Characters extracted perfectly.
3. `Hello বাংলাদেশ PDF (Nirmala)` -> Success. Mixed scripts correctly map to their glyphs.
4. `ÿÿÿÿÿÿÿÿ (Arial)` -> Failure (Expected). Arial does not contain Bengali glyphs, so the text is corrupted.

## Findings
1. **Fallback Viability:** Loading `Nirmala.ttc` dynamically from the Windows Font directory and passing it to PDFium works perfectly for Bengali and mixed scripts.
2. **CID vs Non-CID:** Using `cid=true` in `FPDFText_LoadFont` correctly generates the `ToUnicode` map for the embedded font, ensuring the text remains fully searchable and extractable.
3. **Existing Text Limitations:** We confirmed that applying unsupported Unicode characters to a standard English font (like Arial) results in corrupted `ÿÿÿÿ` extraction and rendering. 

## Conclusion
The prototype proves that `Nirmala UI` is a robust and reliable fallback font for Indic scripts. We will proceed to integrate a `FontManager` into the `pdf_engine` to automatically instantiate this fallback when unsupported characters are detected during text insertion.
