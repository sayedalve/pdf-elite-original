# PDF Elite - Option Functionality Status

This document catalogs the current implementation status of all native PDF Elite tools and options as of the initial Batch 1-5 development phase.

## Fully Implemented (Native C++ / Direct2D / PDFium)
- **Text Editing (Line & Paragraph)**: Editing, grouping, caret, keyboard input, Bold/Italic via Ctrl+B/I.
- **Image Editing**: Drag to move, resize, alignment guides (geometric snapping).
- **Page Management**: Reorder (drag & drop), Delete, Extract, Rotate, Insert Blank Page.
- **Context Menus**:
  - **Images**: Replace Image, Extract Image, Delete.
  - **Text**: Copy, Edit, Delete.
- **Search**: Fully integrated via `SearchEngine`.

## Visually Present but Disabled (By Design)
The following features are intentionally exposed in the UI (e.g., Toolbar, Context Menus) to match the target UX design, but their backend actions are disabled so the application does not pretend they work.

### Annotations & Comments
- Highlight, Underline, Strikeout
- Squiggly, Caret, Line, Arrow, Rectangle, Oval, Polygon, Cloud
- Note, Attachment, Measure, Stamps, Signature

### Advanced Editing
- Add Link (Text Context Menu & Toolbar)
- Watermark, Background
- Crop (Image Context Menu)

### Advanced Page Management
- Replace Page
- Split PDF

### View & Window Management
- Dark Mode
- Split View
- New Window
- Hand Tool, Select Tool (fallback modes)

### System & Conversion
- Convert: To Word, To Excel, To PPT, To Image
- Utility: Print, Cloud, Share

## Missing / Pending Future Implementation
- Reading Modes (Fit Width, Fit Page implemented, but custom mode UX pending)
- Bookmark Context Menu
- Thumbnail Context Menu (beyond Drag-to-move)
- Main Application Menu (Ribbon / Top Menu)
- Save / Save-As workflows
- PDF/A, DRM, and XFA Form validations
