#include "SearchBar.h"
#include <commctrl.h>

#define ID_EDIT 101

SearchBar::SearchBar() : ui::dialogs::ModernDialog(nullptr, L"Search", 280, 260) {}
SearchBar::~SearchBar() {}

bool SearchBar::Create(HWND parentHwnd) {
    m_parent = parentHwnd;
    
    RECT pr; GetClientRect(parentHwnd, &pr);
    int x = pr.right - 290;
    int y = 50;
    
    CreateModeless(x, y);
    return m_hwnd != nullptr;
}

void SearchBar::Show() {
    if (m_hwnd) ShowWindow(m_hwnd, SW_SHOW);
}

void SearchBar::Hide() {
    if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE);
}

bool SearchBar::IsVisible() const {
    return m_hwnd && IsWindowVisible(m_hwnd);
}

void SearchBar::SetFocus() {
    if (m_editHwnd) ::SetFocus(m_editHwnd);
}

void SearchBar::SetResultCount(int /*count*/, int /*current*/) {
    // optional
}

void SearchBar::OnCreate() {
    m_editHwnd = CreateWindowExW(
        0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
        0, 0, 0, 0, m_hwnd, (HMENU)ID_EDIT, GetModuleHandle(nullptr), nullptr
    );
    SendMessage(m_editHwnd, WM_SETFONT, (WPARAM)GetEditFont(), TRUE);
    SetWindowSubclass(m_editHwnd, EditSubclassProc, 0, (DWORD_PTR)this);
}

void SearchBar::OnLayout(float w, float /*h*/) {
    float py = 50.0f;
    m_rectEdit = {15.0f, py, w - 15.0f, py + 25.0f};
    SetChildPos(m_editHwnd, 15.0f, py, w - 30.0f, 25.0f);
    
    py += 35.0f;
    m_rectWholeWord = {15.0f, py, w - 15.0f, py + 20.0f}; py += 24.0f;
    m_rectCase = {15.0f, py, w - 15.0f, py + 20.0f}; py += 24.0f;
    m_rectComments = {15.0f, py, w - 15.0f, py + 20.0f}; py += 24.0f;
    m_rectForms = {15.0f, py, w - 15.0f, py + 20.0f}; py += 24.0f;
    m_rectBookmarks = {15.0f, py, w - 15.0f, py + 20.0f}; py += 24.0f;
    
    py += 10.0f;
    float btnW = (w - 40.0f) / 2.0f;
    m_rectPrev = {15.0f, py, 15.0f + btnW, py + 30.0f};
    m_rectNext = {15.0f + btnW + 10.0f, py, w - 15.0f, py + 30.0f};
}

void SearchBar::OnRender() {
    RenderBase();
    DrawCheckbox(m_rectWholeWord, L"Whole words only", 16, m_chkWholeWord);
    DrawCheckbox(m_rectCase, L"Case sensitive", 14, m_chkCase);
    DrawCheckbox(m_rectComments, L"Include comments", 16, m_chkComments);
    DrawCheckbox(m_rectForms, L"Include forms", 13, m_chkForms);
    DrawCheckbox(m_rectBookmarks, L"Include bookmarks", 17, m_chkBookmarks);
    
    DrawSecondaryButton(m_rectPrev, L"Previous", m_btnPrevHover, m_btnPrevDown);
    DrawSecondaryButton(m_rectNext, L"Next", m_btnNextHover, m_btnNextDown);
}

void SearchBar::OnMouseMove(float x, float y) {
    bool prevH = PtInR(x, y, m_rectPrev);
    bool nextH = PtInR(x, y, m_rectNext);
    if (prevH != m_btnPrevHover || nextH != m_btnNextHover) {
        m_btnPrevHover = prevH;
        m_btnNextHover = nextH;
        Invalidate();
    }
}

void SearchBar::OnMouseDown(float x, float y) {
    if (PtInR(x, y, m_rectPrev)) m_btnPrevDown = true;
    if (PtInR(x, y, m_rectNext)) m_btnNextDown = true;
    if (m_btnPrevDown || m_btnNextDown) Invalidate();
}

void SearchBar::OnMouseUp(float x, float y) {
    if (PtInR(x, y, m_rectWholeWord)) m_chkWholeWord = !m_chkWholeWord;
    if (PtInR(x, y, m_rectCase)) m_chkCase = !m_chkCase;
    if (PtInR(x, y, m_rectComments)) m_chkComments = !m_chkComments;
    if (PtInR(x, y, m_rectForms)) m_chkForms = !m_chkForms;
    if (PtInR(x, y, m_rectBookmarks)) m_chkBookmarks = !m_chkBookmarks;
    
    if (m_btnPrevDown && PtInR(x, y, m_rectPrev) && m_onPrev) m_onPrev();
    if (m_btnNextDown && PtInR(x, y, m_rectNext) && m_onNext) m_onNext();
    
    if (PtInR(x, y, m_rectClose) && m_onClose) m_onClose();
    
    m_btnPrevDown = false;
    m_btnNextDown = false;
    Invalidate();
}

LRESULT CALLBACK SearchBar::EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) {
    SearchBar* pThis = (SearchBar*)dwRefData;
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_RETURN) {
            wchar_t buf[256] = {0};
            GetWindowTextW(hWnd, buf, 256);
            if (pThis->m_onSearch) pThis->m_onSearch(buf);
            return 0;
        } else if (wParam == VK_ESCAPE) {
            if (pThis->m_onClose) pThis->m_onClose();
            return 0;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
