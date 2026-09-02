
Fixes from awful UI to exact target Wondershare PDFelement (your last 5 images):

BEFORE (awful - image_61712b, image_656520):
- Pure black #0f1117 background, blue squares 48px, no icons, text encoding broken â€¢, cramped, no Open PDF white button
- Toolbar garbled â†© characters

AFTER (exact target - image_b8a449, b7a863, aa37f7, etc):
- Sidebar #171729, main #1e1e2f, surface #28283e, toolbar #2b2b42, accent #6b8cff
- Home: Logo Wondershare PDFelement, Open PDF WHITE button 48px radius 10, Create PDF outlined, Quick Tools 4x2 grid with colored icons Orange/Green/Purple/Red/Blue exact from target, Recent Files table Name/Modified Time/Size with search pill
- Viewer: TopBar 48px #1c1c2e tabs rounded 8px bg #28283e with X and + new tab, right blue avatar circle, bell, gear, min/max/close
- Toolbar 48px exact:
  * View: undo/redo | zoom -/+ | Hand Rect | Edit All ▼ Add Text OCR Crop Combine Compress ... | Search Tools save print cloud upload
  * Comment: highlight, area highlight, pencil, eraser, underline, text, rect, stamp, image, signature
  * Edit: Edit All ▼ Add Text Add Link Image ▼ Watermark ▼ Background ▼
  * Organize: undo/redo | zoom -/+ | 1 dropdown | rotate left/right trash | Extract Split ▼ Insert ▼ | Crop Rotate Size
- LeftRail 64px #171729 icons: Home rocket, Comment bubble, Edit pencil, Convert arrows, View eye, Organize pages, Tools toolbox, Form list, ... bottom
- RightRail 48px: thumbnails, bookmarks, comments, page 14/564 or 4/51 blue selected bg #28283e, up/down, hand, fit, 200% zoom, +/-
- Center: Organize 4-col thumbnail grid exact from image_33a3e9 with blue border selected and rotate/trash top actions, View/Edit black PDF page with white text as in target
- All states: normal #9a9ab0, hover #32324e #e8e8f0, active #28283e border #6b8cff, disabled opacity 0.4
- Encoding fixed: • not â€¢, proper UTF-8

Build: cl /EHsc /DUNICODE /std:c++17 Theme.cpp AppShell.cpp MainWindow.cpp LayoutManager.cpp PdfCanvas.cpp PdfDocument.cpp DocumentView.cpp GraphicsDevice.cpp RenderWorker.cpp TileCache.cpp d2d1.lib dwrite.lib
