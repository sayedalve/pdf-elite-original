# PDF Elite Native C++ UI Specification
Derived from Redesign.html (Source of Truth)

## Better Colors (from liked HTML)
- app-bg: #0f1117
- sidebar-bg: #151821
- surface: #1c1f2b
- elevated: #232636
- card: #2a2e3d / hover #32364a
- border: rgba(255,255,255,0.06) / strong 0.10 / hover 0.12
- text-primary: #e4e6eb
- text-secondary: #8b90a8
- text-tertiary: #5a5f7a
- accent: #7c9cff glow 0.25 subtle 0.12
- success: #34d399

## Window Structure
MainWindow (Win32 overlapped, 1440x900 default)
  TopBar 44px: hamburger, home icon, document tabs, + new tab, right: avatar #7c9cff blue, bell, gear, min/max/close (native frame handled by Win32)
  TabBar 44px: Actually part of TopBar in design - single tab "Lecture 1 & 2.pdf" with X, active bg #1c1f2b border #FFFFFF0F radius 8
  MainToolbar 56px: contextual per ViewerMode, bg #1c1f2b border bottom rgba(255,255,255,0.06)
  Workspace:
    LeftRail 72px: bg #151821 border right 0.06, 8 items: Home, Comment, Edit, Convert, View, Organize, Tools, Form, each 56px height, icon 20px, label 11px uppercase, active bg #232636 border #7c9cff radius 12
    Center: bg #0f1117 grid pattern rgba(255,255,255,0.02) 32px
      View/Comment/Edit: Single page centered 794x1123 scaled 0.77, shadow 0 8px 40px rgba(0,0,0,0.5), white bg
      Organize: Thumbnail grid 3 cols gap 24px, thumb 10px radius bg #1c1f2b border 0.06, selected border 2px #7c9cff, top actions rotate/delete, number below 11px
    RightRail 48px: bg #151821 border left 0.06, icons 32px, page indicator 4 / 51, 4 selected blue bg #232636, zoom 77%
  StatusBar 24px: not prominent in design, but bottom info

## Top Level Navigation
Home: Left sidebar 280px with 5 items: Home, Recent (12), Starred (4), Shared (3), Trash, each 32px height active bg #232636, count pill bg #1c1f2b radius 6
Home Main: Padding 40px, gap 32px
  Workspace header: Title 18px bold #e4e6eb, subtitle 12.5px #8b90a8, count with nav actions
  Quick Tools: Title 11px uppercase #5a5f7a, grid 4 cols gap 20px, card 110px height 16px radius bg #2a2e3d border 0.06, icon 48px radius 14px gradient accent, name 13px medium, desc 12.5px
  Recent Files: Title 13px medium, table header 11px uppercase #5a5f7a border bottom, rows 56px radius 10 hover #232636, icon 36px radius 10 bg #232636, name 13px, meta 11px #5a5f7a, size/modified 12.5px #8b90a8

## Document Tab Area
Single tab in top bar, not separate bar. Height 28px inside 44px bar, rounded 8, active bg surface, inactive hover elevated. X close 16px.

## Main Toolbar - 4 Contextual States
View (default): undo ↩, redo ↪, |, zoom out -, zoom in +, |, hand select, rect select, |, Edit All dropdown bg elevated, Add Text, OCR, Crop, Combine, Compress, ... overflow, |, Search Tools #5a5f7a, save, print, cloud, upload
Comment: highlight (yellow), area highlight, pencil, eraser, |, underline, strikethrough, |, text, text box, rectangle, |, stamp, image, signature, attachment, highlight color dropdown, eye toggle
Edit: Edit All ▼, |, Add Text, Add Link, Image ▼, |, Watermark ▼, Background ▼, ...
Organize: undo/redo | zoom | page dropdown "1" | rotate left/right trash | Extract Split ▼ Insert ▼ | Crop Rotate Size ...

All toolbar items: height 28px, padding 12px, radius 6, normal #8b90a8, hover bg #232636 #e4e6eb, active bg #2a2e3d, disabled opacity 0.4

## Left Navigation / Sidebar
Viewer: 72px width, Home uses 280px width. Icons centered, label below. Hover bg #1c1f2b, active bg #232636 + accent border 1px #7c9cff, accent dot.

## PDF Canvas
Defined bounds inside Center after subtracting rails and toolbar. Clipping rectangle enforced via ID2D1RenderTarget::PushAxisAlignedClip. Never draws outside. Background #0f1117, grid 32px rgba(255,255,255,0.02). Page centered horizontally, 40px top margin, shadow, white.

## Thumbnail Area
Organize mode only: 3 columns, gap 24px, thumb aspect 1:1.4, 10px radius, title below hidden in grid but page number visible. Selected: blue border 2px, top-right actions.

## Right Side Controls
48px width, icons 32px centered, divider 16px gap, page indicator 4 (active) / 51, up/down arrows, hand, fit width, 77%, zoom in/out.

## Status Bar
24px height, bg #151821, text 11px #5a5f7a, left: page info, right: zoom.

## Search Controls
In toolbar right: "Search Tools" placeholder, when active: input bg #232636 border 0.10 radius 8, 13px text, results count 11px mono #8b90a8.

## Page Navigation
Right rail up/down, or Ctrl+G, or thumbnail click. Current 4 / 51.

## Zoom Controls
77% display, - + buttons, Ctrl+Wheel, pinch. Slider would be in status bar.

## Tool Controls
Comment/Edit tools have color picker, thickness. Dropdowns: bg #232636 border 0.10 radius 8 shadow, items 32px height.

## Active/Inactive/Hover/Disabled
- Normal: text #8b90a8, bg transparent
- Hover: bg #232636, text #e4e6eb, 100ms ease-out
- Pressed: bg #2a2e3d, scale 0.98
- Active: bg #232636 + border #7c9cff, text #e4e6eb, accent dot 3px #7c9cff
- Selected (thumbnail/page): border 2px #7c9cff, bg #1c1f2b
- Disabled: opacity 0.4, no hover
- Focused: ring 2px #7c9cff 0.5 opacity

## Modal/Popover
Dropdowns: bg #232636, border 0.10, radius 12, shadow 0 16px 48px rgba(0,0,0,0.5), items 36px.

## Responsive
- <1024px: LeftRail collapses to icons only, RightRail hidden, Quick Tools 2 cols
- 1024-1280: Quick Tools 3 cols, Recent Files size column hidden
- 1280-1440: 4 cols as designed
- >1600: Center max-width 1600, extra padding, thumbnails 4 cols in Organize
- All calculations via LayoutManager, no magic numbers scattered.

## Document Workflow
Home → Open PDF (or Quick Tool) → Viewer View mode → Comment/Edit/Organize via LeftRail → Save → Esc back to Home.

Shell vs Workspace: Shell = TopBar, Tabs, Toolbar, LeftRail, RightRail, StatusBar. Workspace = Center PDF Canvas / Thumbnail Grid. Engine never draws outside Canvas clip.
