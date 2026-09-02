# REDESIGN SPECIFICATION

This document describes the native UI equivalent of every major component extracted from the `Redesign.html` React artifact.

## 1. GLOBAL
- **Window Dimensions**: 1280x720 minimum reference, responsive.
- **Color Palette**:
  - Background (Main): `#0f1117`
  - Background (Sidebar/Surface): `#151821`
  - Background (Elevated/Card): `#1c1f2b`
  - Background (Hover/Active): `#232636`, `#2a2e3d`
  - Text (Primary): `#e4e6eb`, `#ffffff`
  - Text (Secondary): `#8b90a8`, `#5a5f7a`
  - Accent/Gradient (Brand): `#7c9cff` to `#a78bfa`
  - Accent (Success): `#34d399`
  - Border: `rgba(255,255,255,0.06)` or `rgba(255,255,255,0.08)`
- **Typography**: Inter (UI Sans-Serif). Font sizes typically `11px`, `12px`, `13px`, `14px`, `15px`, `22px`, `28px`, `32px`.
- **Radii**: 8px (`rounded-md`), 12px, 14px, 16px, 20px, full (capsule).

## 2. HOME WORKSPACE

### Window Structure
`AppShell`
 ├── `Sidebar` (Left)
 └── `MainContent` (Right)

### Sidebar (Left)
- **Dimensions**: `280px` width, full height.
- **Background**: `#151821`
- **Border**: Right border `1px solid rgba(255,255,255,0.06)`
- **Components**:
  1. **Top Header**: Height `72px`, padded `px-5`.
     - Brand Icon: `36x36px`, rounded-xl, gradient background, with shadow.
     - Brand Text: "PDF Elite" (15px bold).
     - Badge: "PRO" with pulse dot.
  2. **Action Area**: Height `76px`.
     - "New Document" Button: `44px` height, full width, gradient bg, rounded-xl, text white 13.5px. Contains "⌘N" shortcut badge.
  3. **Navigation List**: Scrollable flex column, `px-3`.
     - Sections: "Workspace", "Quick Access".
     - Items: 32px height (`h-8`), rounded-lg, 13px font. Default text `#8b90a8`, hover `#1c1f2b` background and `#e4e6eb` text. Selected item (Home) bg `#232636`.
  4. **Bottom Status**:
     - Storage meter: 68% used.
     - User Profile: Avatar (32x32), Name, Email.

### MainContent (Right)
- **Dimensions**: `flex-1`, full height.
- **Background**: `#0f1117`
- **Scroll**: Vertical.
- **Header Section**:
  - Greeting text (28-32px bold).
  - Status indicator ("All systems operational").
- **Quick Tools Grid**:
  - `grid-cols-4` with gap.
  - Tool Card: `h-[184px]`, rounded-16px, bg `#1c1f2b`, border `rgba(255,255,255,0.06)`. Hover: translate-y -2px, bg `#232636`, shadow.
  - Card Content: Icon (56x56, rounded-14px, gradient), Usage stat badge, Title (14px), Subtitle (12px).
- **Recent Files List**:
  - Container: rounded-20px, bg `#151821`, shadow `0 8px 40px rgba(0,0,0,0.25)`.
  - Header: `64px` height, Search box (h-9, rounded-full, bg `#1c1f2b`).
  - Column Headers: Name, Size, Modified. Height `36px`, text 11px uppercase.
  - Row Item: `52px` height, hover bg `#1c1f2b`. Includes file icon (36x36 rounded-10px bg `#232636`), file name (13px), path (11px).

## 3. DOCUMENT WORKSPACE

### Window Structure
`AppShell`
 ├── `ApplicationTopBar` (Tabs)
 ├── `ReadingToolbar` (Secondary Toolbar)
 └── `WorkspaceArea` (flex-1)
      ├── `LeftSidebar` (Tools)
      ├── `PDFCanvas` (Main Document)
      └── `RightSidebar` (Navigation)

### ApplicationTopBar (Tabs)
- **Dimensions**: Height `44px`.
- **Background**: `#151821`
- **Border**: Bottom border `rgba(255,255,255,0.06)`.
- **Components**:
  - Window Controls / Generic Buttons (32x32px).
  - Tab Scroll Area: `flex-1`.
  - Tab Item: Height `32px`, rounded-full, bg `#232636`, border `rgba(255,255,255,0.08)`. Active tab has top accent line (`#7c9cff`).
  - Right Actions: Account avatar, settings.

### ReadingToolbar
- **Dimensions**: Height `44px`.
- **Background**: `#1c1f2b`
- **Border**: Bottom border `rgba(255,255,255,0.06)`.
- **Components**:
  - Groups separated by `1x20px` dividers (`bg-[rgba(255,255,255,0.08)]`).
  - Buttons: Height `28px` (h-7). `px-2` for text buttons, `w-7` for icon buttons.
  - Active button (Select): bg `#232636`, text white.
  - Tool Search Box: `140x28px`, rounded-8px, bg `#0f1117`.
  - Export Button: bg `#7c9cff`, text white, shadow.

### LeftSidebar (Tool Switcher)
- **Dimensions**: Width `72px`.
- **Background**: `#151821`
- **Border**: Right border `rgba(255,255,255,0.06)`.
- **Components**:
  - Vertical list of tool modes (Home, Comment, Edit, Convert, View, Organize, Tools, Form).
  - Item: `56x56px` rounded-14px. Icon + text (10px).
  - Active Item (View): bg `#232636`, text `#e4e6eb`. Includes right-side accent marker (3x20px `#7c9cff`).

### PDFCanvas
- **Background**: `#0f1117` with `grid-bg` class (dotted/grid pattern).
- **Layout**: Centered flex column, `py-8 px-4` padding.
- **Document Page**:
  - Background: `white`, Text: `#0f1117`.
  - Shadow: `0 8px 40px rgba(0,0,0,0.5), 0 0 0 1px rgba(0,0,0,0.08)`.
  - Rounded: `12px`.
- **Floating Page Counter**: (Visible on smaller screens or when scrolling) Bottom center, pill shape, dark translucent bg.

### RightSidebar (Navigation / Zoom)
- **Dimensions**: Width `56px`.
- **Background**: `#151821`
- **Border**: Left border `rgba(255,255,255,0.06)`.
- **Components**:
  - Thumbnails toggle, Outline toggle.
  - Page Navigation: Up/Down buttons (36x36 rounded-10px bg `#232636`).
  - Page Indicator: `36x28px` rounded-full bg `#1c1f2b`, text `11px mono` (e.g. "4 / 51").
  - Zoom Controls: +/- buttons, Zoom percentage `36x28px` (e.g. "77").

## INTERACTION STATES
- **Hover**: Background shifts to lighter tones (e.g., `#1c1f2b` to `#232636`), opacity transitions, icons shift color from `#5a5f7a` to `#8b90a8` or white.
- **Pressed/Active**: Components scale down slightly `active:scale-[0.98]` or `0.96`.
- **Disabled**: Standard Windows disabled opacity (usually 0.4-0.5).
- **Selected**: Highlight backgrounds (`#232636`) and accented borders or markers (`#7c9cff`).
- **Scroll Behavior**: Smooth, `scrollbar-thin`. Layout remains static while content areas scroll (e.g., Tab bar fixed, canvas scrolls).

## IMPLEMENTATION NOTES
- **Magic Numbers**: Do not hardcode dimensions for the canvas or main view areas; use Flex-like layout principles (`Layout()` methods must calculate relative to parent bounds minus fixed-width sidebars/toolbars).
- **Hit Testing**: Native components must implement `HitTest()` corresponding to their rectangular regions matching the HTML specs above.
- **Direct2D Rendering**:
  - Use `ID2D1SolidColorBrush` initialized with the precise hex colors above.
  - Use `ID2D1RoundedRectangle` with radii matching `8px`, `12px`, `16px`, `20px`.
  - Text rendered with `IDWriteTextFormat` using "Inter" or system UI sans-serif at exact pixel sizes specified.
