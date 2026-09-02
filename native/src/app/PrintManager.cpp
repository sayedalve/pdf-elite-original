#include "PrintManager.h"
#include <windows.h>
#include <commdlg.h>
#include <algorithm>

namespace app {

bool PrintManager::PrintDocument(HWND hwndOwner, std::shared_ptr<core::interfaces::dom::IDocument> doc) {
    if (!doc || doc->PageCount() <= 0) return false;

    PRINTDLGW pd = {0};
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner   = hwndOwner;
    pd.hDevMode    = NULL;     
    pd.hDevNames   = NULL;     
    pd.Flags       = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC | PD_NOSELECTION; 
    pd.nCopies     = 1;
    pd.nFromPage   = 1; 
    pd.nToPage     = static_cast<WORD>(doc->PageCount()); 
    pd.nMinPage    = 1; 
    pd.nMaxPage    = static_cast<WORD>(doc->PageCount()); 

    if (PrintDlgW(&pd) == TRUE) {
        if (!pd.hDC) return false;

        int copies = pd.nCopies;
        int fromPage = (pd.Flags & PD_PAGENUMS) ? pd.nFromPage : 1;
        int toPage = (pd.Flags & PD_PAGENUMS) ? pd.nToPage : doc->PageCount();

        // Sanitize bounds
        fromPage = std::max(1, fromPage);
        toPage = std::min(doc->PageCount(), toPage);

        DOCINFOW di = {0};
        di.cbSize = sizeof(DOCINFO);
        di.lpszDocName = L"PDF Elite Document";

        if (StartDocW(pd.hDC, &di) > 0) {
            for (int copy = 0; copy < copies; copy++) {
                for (int p = fromPage; p <= toPage; p++) {
                    if (StartPage(pd.hDC) > 0) {
                        auto page = doc->GetPage(p - 1); // 0-indexed
                        if (page) {
                            int physicalWidth = GetDeviceCaps(pd.hDC, PHYSICALWIDTH);
                            int physicalHeight = GetDeviceCaps(pd.hDC, PHYSICALHEIGHT);
                            int offsetX = GetDeviceCaps(pd.hDC, PHYSICALOFFSETX);
                            int offsetY = GetDeviceCaps(pd.hDC, PHYSICALOFFSETY);

                            auto size = page->GetSize();
                            int dpiX = GetDeviceCaps(pd.hDC, LOGPIXELSX);
                            int dpiY = GetDeviceCaps(pd.hDC, LOGPIXELSY);

                            // Auto-rotate logic: if paper aspect ratio doesn't match PDF aspect ratio, rotate the PDF
                            bool pdfIsLandscape = size.width > size.height;
                            bool paperIsLandscape = physicalWidth > physicalHeight;
                            int rotate = 0;
                            
                            double pdfW = size.width;
                            double pdfH = size.height;
                            
                            if (pdfIsLandscape != paperIsLandscape) {
                                rotate = 1; // 90 degrees
                                pdfW = size.height;
                                pdfH = size.width;
                            }

                            double pdfWidthPx = pdfW * dpiX / 72.0;
                            double pdfHeightPx = pdfH * dpiY / 72.0;

                            double scaleX = physicalWidth / pdfWidthPx;
                            double scaleY = physicalHeight / pdfHeightPx;
                            double scale = std::min(scaleX, scaleY);

                            int outW = (int)(pdfWidthPx * scale);
                            int outH = (int)(pdfHeightPx * scale);

                            int outX = (physicalWidth - outW) / 2 - offsetX;
                            int outY = (physicalHeight - outH) / 2 - offsetY;

                            page->RenderForPrint(pd.hDC, outX, outY, outW, outH, rotate);
                        }
                        EndPage(pd.hDC);
                    }
                }
            }
            EndDoc(pd.hDC);
        }
        DeleteDC(pd.hDC);
        
        if (pd.hDevMode != NULL) GlobalFree(pd.hDevMode);
        if (pd.hDevNames != NULL) GlobalFree(pd.hDevNames);
        return true;
    }
    return false;
}

}
