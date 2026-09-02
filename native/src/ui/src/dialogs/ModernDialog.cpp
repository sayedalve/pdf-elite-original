#include "ui/dialogs/ModernDialog.h"
#include "../GraphicsDevice.h"
#include <windowsx.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace ui::dialogs {

ModernDialog::ModernDialog(HWND parent, const std::wstring& title, int width, int height)
    : m_parent(parent), m_title(title), m_width(width), m_height(height) {}

ModernDialog::~ModernDialog() { if (m_editFont) DeleteObject(m_editFont); }
HFONT ModernDialog::GetEditFont() {
    if (!m_editFont) {
        int height = -MulDiv(11, GetDpiForWindow(m_hwnd), 72);
        m_editFont = CreateFontW(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                                 DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    }
    return m_editFont;
}


bool ModernDialog::DoModal() {
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"PDFEliteModernModal";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    m_dpiScale = GetDpiForWindow(m_parent) / 96.0f;
    int scaledW = static_cast<int>(m_width * m_dpiScale);
    int scaledH = static_cast<int>(m_height * m_dpiScale);

    RECT pr; GetWindowRect(m_parent, &pr);
    int cx = pr.left + (pr.right - pr.left)/2 - scaledW/2;
    int cy = pr.top + (pr.bottom - pr.top)/2 - scaledH/2;

    m_hwnd = CreateWindowExW(
        0, // No border!
        L"PDFEliteModernModal", m_title.c_str(),
        WS_POPUP | WS_CLIPCHILDREN,
        cx, cy, scaledW, scaledH,
        m_parent, nullptr, wc.hInstance, this
    );

    EnableWindow(m_parent, FALSE);
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    m_running = true;
    MSG msg;
    while (m_running && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(m_hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(m_parent, TRUE);
    SetForegroundWindow(m_parent);
    DestroyWindow(m_hwnd);
    return m_resultOk;
}

LRESULT CALLBACK ModernDialog::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ModernDialog* pThis = nullptr;
    if (uMsg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<ModernDialog*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<ModernDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (pThis) return pThis->HandleMessage(uMsg, wParam, lParam);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ModernDialog::CreateDeviceResources() {
    if (m_target) return;
    RECT rc; GetClientRect(m_hwnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(static_cast<UINT32>(rc.right - rc.left), static_cast<UINT32>(rc.bottom - rc.top));
    
    D2D1_RENDER_TARGET_PROPERTIES rtp = D2D1::RenderTargetProperties();
    rtp.dpiX = m_dpiScale * 96.0f;
    rtp.dpiY = m_dpiScale * 96.0f;
    GraphicsDevice::Instance().GetD2DFactory()->CreateHwndRenderTarget(
        rtp,
        D2D1::HwndRenderTargetProperties(m_hwnd, size),
        &m_target
    );
    
    // Solid background matching screenshot
    m_target->CreateSolidColorBrush(D2D1::ColorF(0x1E1E2F), &m_bgBrush);
    m_target->CreateSolidColorBrush(D2D1::ColorF(0x1E1E2F), &m_surfaceBrush);
    m_target->CreateSolidColorBrush(D2D1::ColorF(0xFFFFFF), &m_textBrush);
    m_target->CreateSolidColorBrush(D2D1::ColorF(0x000000), &m_textDarkBrush);
    m_target->CreateSolidColorBrush(D2D1::ColorF(0xFFFFFF, 0.3f), &m_borderBrush);
    m_target->CreateSolidColorBrush(D2D1::ColorF(0xFFFFFF), &m_primaryBrush);
    m_target->CreateSolidColorBrush(D2D1::ColorF(0xDDDDDD), &m_hoverBrush);
    m_target->CreateSolidColorBrush(D2D1::ColorF(0xE81123), &m_closeHoverBrush);
    m_target->CreateSolidColorBrush(D2D1::ColorF(0x191A20), &m_inputBgBrush);
    m_target->CreateSolidColorBrush(D2D1::ColorF(0x4E8FF9), &m_accentBrush);
    
    auto dwrite = GraphicsDevice::Instance().GetDWriteFactory();
    dwrite->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-US", &m_formatTitle);
    dwrite->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.0f, L"en-US", &m_formatHeader);
    dwrite->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.0f, L"en-US", &m_formatLabel);
    dwrite->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"en-US", &m_formatValue);
}

void ModernDialog::RenderBase() {
    float w = m_target->GetSize().width;

    m_target->Clear(D2D1::ColorF(0x1E1E2F));
    
    // Custom Titlebar text
    m_target->DrawTextW(m_title.c_str(), (UINT32)m_title.length(), m_formatTitle.Get(), {15, 10, w - 50, 30}, m_textBrush.Get());

    // Close Button
    m_rectClose = {w - 40, 0, w, 30};
    if (m_closeDown) m_target->FillRectangle(m_rectClose, m_closeHoverBrush.Get());
    else if (m_closeHover) m_target->FillRectangle(m_rectClose, m_closeHoverBrush.Get());
    
    float cx = m_rectClose.left + 20.0f;
    float cy = 15.0f;
    m_target->DrawLine(D2D1::Point2F(cx - 4, cy - 4), D2D1::Point2F(cx + 4, cy + 4), m_textBrush.Get(), 1.0f);
    m_target->DrawLine(D2D1::Point2F(cx - 4, cy + 4), D2D1::Point2F(cx + 4, cy - 4), m_textBrush.Get(), 1.0f);
}

bool ModernDialog::PtInR(float x, float y, const D2D1_RECT_F& r) {
    return x >= r.left && x <= r.right && y >= r.top && y <= r.bottom;
}

void ModernDialog::SetChildPos(HWND child, float x, float y, float w, float h) {
    if (child) {
        SetWindowPos(child, nullptr, (int)(x * m_dpiScale), (int)(y * m_dpiScale), (int)(w * m_dpiScale), (int)(h * m_dpiScale), SWP_NOZORDER);
    }
}

void ModernDialog::SetPlaceholder(HWND edit, const std::wstring& placeholder) {
    if (edit) {
        SendMessageW(edit, EM_SETCUEBANNER, FALSE, (LPARAM)placeholder.c_str());
    }
}


void ModernDialog::DrawPrimaryButton(const D2D1_RECT_F& r, const wchar_t* txt, bool hover, bool down) {
    auto bg = down ? m_hoverBrush : (hover ? m_hoverBrush : m_primaryBrush);
    m_target->FillRoundedRectangle(D2D1::RoundedRect(r, 6, 6), bg.Get());
    m_formatValue->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_formatValue->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    m_target->DrawTextW(txt, (UINT32)wcslen(txt), m_formatValue.Get(), r, m_textDarkBrush.Get());
    m_formatValue->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_formatValue->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
}

void ModernDialog::DrawSecondaryButton(const D2D1_RECT_F& r, const wchar_t* txt, bool hover, bool down) {
    auto bg = down ? m_borderBrush : (hover ? m_borderBrush : m_bgBrush);
    m_target->FillRoundedRectangle(D2D1::RoundedRect(r, 6, 6), bg.Get());
    m_target->DrawRoundedRectangle(D2D1::RoundedRect(r, 6, 6), m_borderBrush.Get(), 1.0f);
    m_formatValue->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_formatValue->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    m_target->DrawTextW(txt, (UINT32)wcslen(txt), m_formatValue.Get(), r, m_textBrush.Get());
    m_formatValue->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_formatValue->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
}

void ModernDialog::DrawRadio(const D2D1_RECT_F& r, const wchar_t* txt, int len, bool checked) {
    float cy = r.top + (r.bottom - r.top) / 2.0f;
    m_target->DrawEllipse(D2D1::Ellipse({r.left + 8, cy}, 7, 7), m_borderBrush.Get(), 1.5f);
    if (checked) {
        m_target->FillEllipse(D2D1::Ellipse({r.left + 8, cy}, 4.0f, 4.0f), m_accentBrush.Get());
        m_target->DrawEllipse(D2D1::Ellipse({r.left + 8, cy}, 7, 7), m_accentBrush.Get(), 1.5f);
    }
    m_formatLabel->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    m_target->DrawTextW(txt, len, m_formatLabel.Get(), {r.left + 24, r.top, r.right, r.bottom}, m_textBrush.Get());
    m_formatLabel->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
}

void ModernDialog::DrawCheckbox(const D2D1_RECT_F& r, const wchar_t* txt, int len, bool checked) {
    float cy = r.top + (r.bottom - r.top) / 2.0f;
    D2D1_RECT_F box = {r.left + 1, cy - 7, r.left + 15, cy + 7};
    m_target->DrawRoundedRectangle(D2D1::RoundedRect(box, 3, 3), m_borderBrush.Get(), 1.5f);
    if (checked) {
        m_target->FillRoundedRectangle(D2D1::RoundedRect(box, 3, 3), m_accentBrush.Get());
        m_target->DrawRoundedRectangle(D2D1::RoundedRect(box, 3, 3), m_accentBrush.Get(), 1.5f);
        m_target->DrawLine({box.left + 3, cy}, {box.left + 6, cy + 3}, m_textDarkBrush.Get(), 1.5f);
        m_target->DrawLine({box.left + 6, cy + 3}, {box.right - 3, cy - 4}, m_textDarkBrush.Get(), 1.5f);
    }
    m_formatLabel->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    m_target->DrawTextW(txt, len, m_formatLabel.Get(), {r.left + 24, r.top, r.right, r.bottom}, m_textBrush.Get());
    m_formatLabel->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
}

void ModernDialog::DrawEditBorder(const D2D1_RECT_F& r) {
    m_target->FillRoundedRectangle(D2D1::RoundedRect(r, 4, 4), m_inputBgBrush.Get());
    m_target->DrawRoundedRectangle(D2D1::RoundedRect(r, 4, 4), m_borderBrush.Get(), 1.0f);
}

LRESULT ModernDialog::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            int policy = 2; // DWMWCP_ROUND
            DwmSetWindowAttribute(m_hwnd, 33, &policy, sizeof(policy));
            CreateDeviceResources();
            OnCreate();
            
            D2D1_SIZE_F logicalSize = m_target->GetSize();
            OnLayout(logicalSize.width, logicalSize.height);
            return 0;
        }
        case WM_SIZE: {
            if (m_target) {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                m_target->Resize(D2D1::SizeU(width, height));
                
                D2D1_SIZE_F logicalSize = m_target->GetSize();
                OnLayout(logicalSize.width, logicalSize.height);
            }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(m_hwnd, &ps);
            if (m_target) {
                m_target->BeginDraw();
                m_target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
                RenderBase();
                OnRender();
                m_target->EndDraw();
            }
            EndPaint(m_hwnd, &ps);
            return 0;
        }
        case WM_NCHITTEST: {
            LRESULT hit = DefWindowProc(m_hwnd, uMsg, wParam, lParam);
            if (hit == HTCLIENT) {
                POINT pt;
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
                ScreenToClient(m_hwnd, &pt);
                float logicalX = pt.x / m_dpiScale;
                float logicalY = pt.y / m_dpiScale;
                if (logicalY < 30 && logicalX < m_rectClose.left) return HTCAPTION;
            }
            return hit;
        }
        case WM_MOUSEMOVE: {
            float x = (float)GET_X_LPARAM(lParam) / m_dpiScale;
            float y = (float)GET_Y_LPARAM(lParam) / m_dpiScale;
            
            bool oldCloseHover = m_closeHover;
            m_closeHover = PtInR(x, y, m_rectClose);
            if (oldCloseHover != m_closeHover) Invalidate();
            
            OnMouseMove(x, y);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            float x = (float)GET_X_LPARAM(lParam) / m_dpiScale;
            float y = (float)GET_Y_LPARAM(lParam) / m_dpiScale;
            if (m_closeHover) {
                m_closeDown = true;
                Invalidate();
            } else {
                OnMouseDown(x, y);
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            float x = (float)GET_X_LPARAM(lParam) / m_dpiScale;
            float y = (float)GET_Y_LPARAM(lParam) / m_dpiScale;
            if (m_closeDown) {
                m_closeDown = false;
                if (m_closeHover) {
                    m_resultOk = false;
                    m_running = false;
                }
                Invalidate();
            }
            OnMouseUp(x, y);
            return 0;
        }
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(255,255,255));
            SetBkColor(hdc, RGB(25,26,32)); // 0x191A20
            static HBRUSH hbr = CreateSolidBrush(RGB(25,26,32));
            return (LRESULT)hbr;
        }
        case WM_CLOSE:
            m_resultOk = false;
            m_running = false;
            return 0;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}


HWND ModernDialog::CreateModeless(int x, int y) {
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"PDFEliteModernModal";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    m_dpiScale = GetDpiForWindow(m_parent) / 96.0f;
    int scaledW = static_cast<int>(m_width * m_dpiScale);
    int scaledH = static_cast<int>(m_height * m_dpiScale);

    m_hwnd = CreateWindowExW(
        0,
        L"PDFEliteModernModal", m_title.c_str(),
        WS_CHILD | WS_CLIPCHILDREN,
        x, y, scaledW, scaledH,
        m_parent, nullptr, wc.hInstance, this
    );

    ShowWindow(m_hwnd, SW_HIDE);
    return m_hwnd;
}

} // namespace ui::dialogs

