// LayoutManager.h - Deterministic layout system, no scattered magic numbers
#pragma once
#include "Theme.h"
#include <d2d1.h>

namespace PdfElite {

struct Layout {
    D2D1_RECT_F topBar;
    D2D1_RECT_F toolbar;
    D2D1_RECT_F leftRail;
    D2D1_RECT_F rightRail;
    D2D1_RECT_F center;      // PDF canvas + thumbnails
    D2D1_RECT_F pdfCanvas;   // Clipped area for PDF rendering - NEVER draws outside
    D2D1_RECT_F statusBar;

    // Home
    D2D1_RECT_F sidebarHome;
    D2D1_RECT_F mainHome;
    D2D1_RECT_F quickTools;
    D2D1_RECT_F recentFiles;
};

class LayoutManager {
public:
    static Layout CalculateViewer(const D2D1_RECT_F& client, float zoom = 1.0f);
    static Layout CalculateHome(const D2D1_RECT_F& client);
    static D2D1_RECT_F CalculatePdfPage(const D2D1_RECT_F& canvas, float pageW, float pageH, float zoom);

    static bool IsNarrow(float width) { return width < 1024.0f; }
    static bool IsMedium(float width) { return width >= 1024.0f && width < 1440.0f; }
    static bool IsWide(float width) { return width >= 1440.0f; }
};

} // namespace PdfElite
