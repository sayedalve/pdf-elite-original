#include "BookmarkPanel.h"
#include <windowsx.h>
#include <fstream>
#include <chrono>

namespace components {

BookmarkPanel::BookmarkPanel() {
}

BookmarkPanel::~BookmarkPanel() {
    if (m_hwndTree) DestroyWindow(m_hwndTree);
    if (m_hwndContainer) DestroyWindow(m_hwndContainer);
}

LRESULT CALLBACK BookmarkPanel::ContainerWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BookmarkPanel* pThis = nullptr;
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<BookmarkPanel*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<BookmarkPanel*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (pThis) {
        switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            // Draw dark background
            HBRUSH bgBrush = CreateSolidBrush(RGB(26, 28, 36)); // #1a1c24
            FillRect(hdc, &rc, bgBrush);
            DeleteObject(bgBrush);
            
            // Draw title text
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            HFONT hFont = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
            RECT textRc = { 15, 10, rc.right, 40 };
            DrawTextW(hdc, L"Bookmarks", -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_SIZE: {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            if (pThis->m_hwndAddButton) {
                SetWindowPos(pThis->m_hwndAddButton, nullptr, w - 110, 10, 100, 30, SWP_NOZORDER);
            }
            if (pThis->m_hwndTree) {
                SetWindowPos(pThis->m_hwndTree, nullptr, 0, 50, w, h - 50, SWP_NOZORDER);
            }
            return 0;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1001 && HIWORD(wParam) == BN_CLICKED) {
                if (pThis->m_onAddBookmark) {
                    pThis->m_onAddBookmark();
                }
            }
            return 0;
        }
        case WM_NOTIFY: {
            LPNMHDR lpnmh = (LPNMHDR)lParam;
            if (lpnmh->hwndFrom == pThis->m_hwndTree && lpnmh->code == TVN_SELCHANGEDW) {
                LPNMTREEVIEWW lpnmtv = reinterpret_cast<LPNMTREEVIEWW>(lParam);
                size_t destIndex = static_cast<size_t>(lpnmtv->itemNew.lParam);
                if (destIndex != (size_t)-1 && destIndex < pThis->m_destinations.size()) {
                    if (pThis->m_onNavigate) {
                        pThis->m_onNavigate(pThis->m_destinations[destIndex]);
                    }
                }
            }
            return 0;
        }
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool BookmarkPanel::Create(HWND parentHwnd) {
    m_parentHwnd = parentHwnd;
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = ContainerWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = TEXT("BookmarkPanelContainer");
    RegisterClass(&wc); // Safe to fail if already registered
    
    m_hwndContainer = CreateWindowEx(
        0, wc.lpszClassName, TEXT(""),
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, 100, 100, parentHwnd, nullptr, wc.hInstance, this);
        
    if (!m_hwndContainer) return false;
    
    m_hwndAddButton = CreateWindowExW(
        0, L"BUTTON", L"Add Bookmark",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 100, 30, m_hwndContainer, (HMENU)1001, wc.hInstance, nullptr);
    
    // Create TreeView control
    m_hwndTree = CreateWindowEx(
        0, WC_TREEVIEWW, TEXT("Tree View"),
        WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        0, 0, 100, 100, m_hwndContainer, nullptr, wc.hInstance, nullptr);

    if (m_hwndTree) {
        TreeView_SetBkColor(m_hwndTree, RGB(26, 28, 36));
        TreeView_SetTextColor(m_hwndTree, RGB(220, 220, 220));
        // Remove white border
        SetWindowLong(m_hwndTree, GWL_STYLE, GetWindowLong(m_hwndTree, GWL_STYLE) & ~WS_BORDER);
    }

    return m_hwndTree != nullptr;
}

void BookmarkPanel::Show() {
    if (m_hwndContainer) ShowWindow(m_hwndContainer, SW_SHOW);
}

void BookmarkPanel::Hide() {
    if (m_hwndContainer) ShowWindow(m_hwndContainer, SW_HIDE);
}

void BookmarkPanel::SetBounds(int x, int y, int width, int height) {
    if (m_hwndContainer) {
        SetWindowPos(m_hwndContainer, nullptr, x, y, width, height, SWP_NOZORDER);
    }
}

bool BookmarkPanel::IsVisible() const {
    return m_hwndContainer && IsWindowVisible(m_hwndContainer);
}

void BookmarkPanel::LoadBookmarks(const std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>>& bookmarks) {
    if (!m_hwndTree) return;
    
    SendMessageW(m_hwndTree, WM_SETREDRAW, FALSE, 0);
    TreeView_DeleteAllItems(m_hwndTree);
    m_destinations.clear();
    
    PopulateTree(TVI_ROOT, bookmarks);
    
    SendMessageW(m_hwndTree, WM_SETREDRAW, TRUE, 0);
}

void BookmarkPanel::PopulateTree(HTREEITEM hParent, const std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>>& bookmarks) {
    struct QueueEntry {
        HTREEITEM parent;
        const std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>>* bms;
    };
    std::vector<QueueEntry> queue;
    queue.push_back({hParent, &bookmarks});
    
    size_t head = 0;
    int processedCount = 0;
    const int MAX_BOOKMARKS = 50000;
    
    while (head < queue.size()) {
        auto entry = queue[head++];
        
        for (const auto& bm : *entry.bms) {
            if (++processedCount > MAX_BOOKMARKS) {
                std::ofstream out("C:\\Users\\sayed\\Downloads\\PDF-Elite\\pipeline.log", std::ios_base::app);
                out << "[" << std::chrono::system_clock::now().time_since_epoch().count() << "] " << "PopulateTree hit MAX_BOOKMARKS\n";
                return;
            }
            if (processedCount % 1000 == 0) {
                std::ofstream out("C:\\Users\\sayed\\Downloads\\PDF-Elite\\pipeline.log", std::ios_base::app);
                out << "[" << std::chrono::system_clock::now().time_since_epoch().count() << "] " << "PopulateTree processed " << processedCount << "\n";
            }
            
            TVITEMW tvi = {0};
            tvi.mask = TVIF_TEXT | TVIF_PARAM;
            tvi.pszText = const_cast<LPWSTR>(bm->title.c_str());
            tvi.cchTextMax = static_cast<int>(bm->title.length());
            
            size_t destIndex = (size_t)-1;
            if (bm->destination) {
                destIndex = m_destinations.size();
                m_destinations.push_back(bm->destination.value());
            }
            tvi.lParam = static_cast<LPARAM>(destIndex);

            TVINSERTSTRUCTW tvins = {0};
            tvins.item = tvi;
            tvins.hInsertAfter = TVI_LAST;
            tvins.hParent = entry.parent;
            
            HTREEITEM hItem = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwndTree, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&tvins)));
            
            if (!bm->children.empty()) {
                queue.push_back({hItem, &bm->children});
            }
            
            if (bm->expanded) {
                SendMessageW(m_hwndTree, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(hItem));
            }
        }
    }
}

} // namespace components
