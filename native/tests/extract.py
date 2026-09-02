import pymupdf
doc = pymupdf.open('native/tests/unicode_prototype.pdf')
with open('native/tests/extracted.txt', 'w', encoding='utf-8') as f:
    f.write(doc[0].get_text())
