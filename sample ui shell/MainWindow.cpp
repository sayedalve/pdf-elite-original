#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include <windowsx.h>
#include "IconSystem.h"
// MainWindow.cpp - Full implementation, no empty viewer, distinct icons, DrawText not DrawText
#include "MainWindow.h"
#include "IconSystem.h"
#include "Hugeicons.h"
#include <d2d1helper.h>
#include <windowsx.h>

namespace PdfElite {

MainWindow::MainWindow() {
    m_recent = {
        {L"Daryl L. Logan - Instructor's Solutions Manual to Accompany A First Course...", L"Today, 05:53", L"14.5 MB"},
        {L"Logan ch-2 Stiffness method.pdf", L"Aug 18", L"1.9 MB"},
        {L"Lecture 1_CEE433.pdf", L"Yesterday, 16:05", L"1.9 MB"},
    };
}
MainWindow::~MainWindow() {}

HRESULT MainWindow::Initialize(HINSTANCE hInst) {
    m_hInst=hInst;
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());
    if (FAILED(hr)) return hr;
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_dwFactory.GetAddressOf()));
    if (FAILED(hr)) return hr;
    WNDCLASSEXW wcex={sizeof(WNDCLASSEX)}; wcex.style=CS_HREDRAW|CS_VREDRAW; wcex.lpfnWndProc=WndProc; wcex.hInstance=hInst; wcex.hCursor=LoadCursor(nullptr, IDC_ARROW); wcex.lpszClassName=L"PDFEliteMain"; wcex.hbrBackground=nullptr;
    RegisterClassExW(&wcex);
    m_hwnd=CreateWindowW(L"PDFEliteMain", L"PDF Elite", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1440, 900, nullptr, nullptr, hInst, this);
    if(!m_hwnd) return E_FAIL;
    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(m_hwnd, 20, &useDarkMode, sizeof(useDarkMode));
    DwmSetWindowAttribute(m_hwnd, 19, &useDarkMode, sizeof(useDarkMode));
    ShowWindow(m_hwnd, SW_SHOW); UpdateWindow(m_hwnd); return S_OK;
}
int MainWindow::Run(){ MSG msg{}; while(GetMessage(&msg,nullptr,0,0)){ TranslateMessage(&msg); DispatchMessage(&msg);} return (int)msg.wParam; }
LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    MainWindow* p=nullptr;
    if(msg==WM_NCCREATE){ CREATESTRUCT* cs=reinterpret_cast<CREATESTRUCT*>(lParam); p=reinterpret_cast<MainWindow*>(cs->lpCreateParams); SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)p); p->m_hwnd=hwnd; } else p=reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if(p) return p->HandleMessage(msg,wParam,lParam);
    return DefWindowProc(hwnd,msg,wParam,lParam);
}
LRESULT MainWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
        case WM_CREATE: CreateDeviceResources(); break;
        case WM_SIZE: OnResize(LOWORD(lParam), HIWORD(lParam)); break;
        case WM_PAINT: { PAINTSTRUCT ps; BeginPaint(m_hwnd,&ps); OnRender(); EndPaint(m_hwnd,&ps);} break;
        case WM_MOUSEMOVE: OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); break;
        case WM_LBUTTONDOWN: OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); break;
        case WM_KEYDOWN: if(wParam==VK_ESCAPE && m_mode==AppMode::Viewer){ m_mode=AppMode::Home; InvalidateRect(m_hwnd,nullptr,FALSE);} break;
        case WM_DESTROY: DiscardDeviceResources(); PostQuitMessage(0); break;
        default: return DefWindowProc(m_hwnd,msg,wParam,lParam);
    } return 0;
}
HRESULT MainWindow::CreateDeviceResources(){
    if(m_rt) return S_OK;
    if (!m_d2dFactory) return E_FAIL;
    RECT rc; GetClientRect(m_hwnd,&rc); 
    if (rc.right==0 || rc.bottom==0) return S_OK;
    D2D1_SIZE_U sz=D2D1::SizeU(rc.right-rc.left, rc.bottom-rc.top);
    HRESULT hr = m_d2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_hwnd, sz), &m_rt);
    if (FAILED(hr)) return hr;
    hr = m_theme.Initialize(m_rt.Get(), m_dwFactory.Get());
    if (FAILED(hr)) return hr;
    hr = m_shell.Initialize(m_rt.Get(), m_dwFactory.Get(), &m_theme, &m_cmd);
    if (FAILED(hr)) return hr;
    m_cmd.SetHandler(CommandId::ModeView, [this](){ m_viewerMode=ViewerMode::View; if(m_hwnd) InvalidateRect(m_hwnd,nullptr,FALSE); });
    m_cmd.SetHandler(CommandId::ModeComment, [this](){ m_viewerMode=ViewerMode::Comment; if(m_hwnd) InvalidateRect(m_hwnd,nullptr,FALSE); });
    m_cmd.SetHandler(CommandId::ModeEdit, [this](){ m_viewerMode=ViewerMode::Edit; if(m_hwnd) InvalidateRect(m_hwnd,nullptr,FALSE); });
    m_cmd.SetHandler(CommandId::ModeOrganize, [this](){ m_viewerMode=ViewerMode::Organize; if(m_hwnd) InvalidateRect(m_hwnd,nullptr,FALSE); });
    return S_OK;
}
void MainWindow::DiscardDeviceResources(){ 
    if (m_rt) m_rt.Reset();
    m_shell.Release(); 
    m_theme.Release(); 
}
void MainWindow::OnResize(UINT w, UINT h){ 
    if(m_rt && w>0 && h>0){ 
        m_rt->Resize(D2D1::SizeU(w,h)); 
        if(m_hwnd) InvalidateRect(m_hwnd,nullptr,FALSE);
    } 
}
HRESULT MainWindow::OnRender(){
    if(!m_rt) return S_OK;
    if (!m_theme.brushAppBg) return S_OK;
    m_rt->BeginDraw(); 
    m_rt->Clear(Theme::FromHex(Colors::APP_BG));
    RECT rc; GetClientRect(m_hwnd,&rc); 
    if (rc.right==0 || rc.bottom==0) { m_rt->EndDraw(); return S_OK; }
    D2D1_RECT_F client=D2D1::RectF(0,0,(float)(rc.right-rc.left),(float)(rc.bottom-rc.top));
    if(m_mode==AppMode::Home) RenderHome(m_rt.Get(), client); else RenderViewer(m_rt.Get(), client);
    HRESULT hr=m_rt->EndDraw(); 
    if(hr==D2DERR_RECREATE_TARGET) DiscardDeviceResources(); 
    return hr;
}

void MainWindow::RenderHome(ID2D1RenderTarget* rt, const D2D1_RECT_F& client){
    if (!rt || !m_theme.brushSidebarBg) return;
    D2D1_RECT_F sidebar=D2D1::RectF(client.left, client.top, client.left+240, client.bottom);
    D2D1_RECT_F main=D2D1::RectF(sidebar.right, client.top, client.right, client.bottom);
    RenderHomeSidebar(rt, sidebar);
    RenderHomeMain(rt, main);
}

void MainWindow::RenderHomeSidebar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect){
    if (!rt || !m_theme.brushSidebarBg || !m_theme.brushBorder) return;
    rt->FillRectangle(rect, m_theme.brushSidebarBg);
    rt->DrawLine(D2D1::Point2F(rect.right, rect.top), D2D1::Point2F(rect.right, rect.bottom), m_theme.brushBorder, 1.0f);

    if (m_theme.brushAccent && m_theme.fmtSmall && m_theme.fmtBold) {
        D2D1_ROUNDED_RECT logoRR=D2D1::RoundedRect(D2D1::RectF(rect.left+16, rect.top+16, rect.left+48, rect.top+48), 8,8);
        rt->FillRoundedRectangle(logoRR, m_theme.brushAccent);
        D2D1_RECT_F logoText1=D2D1::RectF(rect.left+56, rect.top+16, rect.right-16, rect.top+32);
        rt->DrawText(L"Wondershare", 11, m_theme.fmtSmall, logoText1, m_theme.brushTextSecondary);
        D2D1_RECT_F logoText2=D2D1::RectF(rect.left+56, rect.top+30, rect.right-16, rect.top+48);
        rt->DrawText(L"PDFelement", 10, m_theme.fmtBold, logoText2, m_theme.brushTextPrimary);
    }

    // Open PDF WHITE button
    {
        D2D1_ROUNDED_RECT openRR=D2D1::RoundedRect(D2D1::RectF(rect.left+12, rect.top+72, rect.right-12, rect.top+116), 10,10);
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> whiteBrush; 
        rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &whiteBrush);
        if (whiteBrush) {
            rt->FillRoundedRectangle(openRR, whiteBrush.Get());
            D2D1_RECT_F openIconRect=D2D1::RectF(rect.left+24, rect.top+84, rect.left+44, rect.top+104);
            if (m_d2dFactory && m_theme.brushTextTertiary) {
                IconSystem::DrawIcon(rt, openIconRect, IconId::OpenFolder, m_theme.brushTextTertiary, 1.2f);
            }
            D2D1_RECT_F openText=D2D1::RectF(rect.left+48, rect.top+84, rect.right-24, rect.top+104);
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> blackBrush; 
            rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &blackBrush);
            if (blackBrush && m_theme.fmtMedium) {
                rt->DrawText(L"Open PDF", 8, m_theme.fmtMedium, openText, blackBrush.Get());
            }
        }
    }

    // Create PDF
    if (m_theme.brushBorderStrong && m_theme.fmtMedium) {
        D2D1_ROUNDED_RECT createRR=D2D1::RoundedRect(D2D1::RectF(rect.left+12, rect.top+124, rect.right-12, rect.top+168), 10,10);
        rt->DrawRoundedRectangle(createRR, m_theme.brushBorderStrong, 1.2f);
        D2D1_RECT_F createText=D2D1::RectF(rect.left+24, rect.top+136, rect.right-24, rect.top+156);
        rt->DrawText(L"Create PDF", 10, m_theme.fmtMedium, createText, m_theme.brushTextPrimary);
    }

    // Nav with Hugeicons distinct
    if (m_theme.fmtBody && m_theme.brushSurface && m_theme.brushTextSecondary) {
        float y=rect.top+200;
        struct Nav { IconId icon; const wchar_t* label; };
        Nav navs[]={{IconId::RecentFiles, L"Recent Files"}, {IconId::StarredFiles, L"Starred Files"}, {IconId::RecentFolders, L"Recent Folders"}};
        for(int i=0;i<3;++i){
            D2D1_RECT_F item=D2D1::RectF(rect.left+8, y, rect.right-8, y+36);
            if(i==0){
                D2D1_ROUNDED_RECT rr=D2D1::RoundedRect(item, 8,8);
                rt->FillRoundedRectangle(rr, m_theme.brushSurface);
            }
            D2D1_RECT_F iconRect=D2D1::RectF(item.left+12, item.top+10, item.left+28, item.top+26);
            if (m_d2dFactory) IconSystem::DrawIcon(rt, iconRect, navs[i].icon, m_theme.brushTextSecondary, 1.2f);
            D2D1_RECT_F tr=D2D1::RectF(item.left+36, item.top+8, item.right-12, item.bottom-8);
            rt->DrawText(navs[i].label, (UINT32)wcslen(navs[i].label), m_theme.fmtBody, tr, m_theme.brushTextSecondary);
            y+=40;
        }
    }
}

void MainWindow::RenderHomeMain(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect){
    if (!rt || !m_theme.brushAppBg || !m_theme.fmtTitle) return;
    rt->FillRectangle(rect, m_theme.brushAppBg);

    D2D1_RECT_F titleRect=D2D1::RectF(rect.left+32, rect.top+20, rect.left+200, rect.top+44);
    rt->DrawText(L"Quick Tools", 11, m_theme.fmtTitle, titleRect, m_theme.brushTextPrimary);
    D2D1_RECT_F allToolsRect=D2D1::RectF(rect.right-160, rect.top+20, rect.right-32, rect.top+44);
    if (m_theme.fmtSmall) rt->DrawText(L"All Tools", 9, m_theme.fmtSmall, allToolsRect, m_theme.brushTextSecondary);

    struct Tool { const wchar_t* name; const wchar_t* desc; uint32_t color; IconId icon; };
    Tool tools[]={
        {L"Edit PDF", L"Edit text and images in files.", 0xf59e0b, IconId::ToolEdit},
        {L"Convert PDF", L"Convert PDFs to Word, Excel, PPT, etc.", 0x10b981, IconId::ToolConvert},
        {L"OCR PDF", L"Convert scanned files to searchable and editable PDF files.", 0x14b8a6, IconId::ToolOcr},
        {L"Add Comments", L"Add highlights, notes, pencil, and other comments.", 0xf87171, IconId::ToolComment},
        {L"Translate PDF", L"Let AI translate the PDF with the original formatting.", 0x8b5cf6, IconId::ToolTranslate},
        {L"Combine Files", L"Combine multiple files into a single PDF.", 0x3b82f6, IconId::ToolCombine},
        {L"Compress PDF", L"Reduce PDF file size.", 0x22c55e, IconId::ToolCompress},
        {L"Batch PDFs", L"Batch convert, create, print, OCR PDFs, etc.", 0x10b981, IconId::ToolBatch},
    };

    float cols=4, gap=16;
    float contentLeft=rect.left+32, contentRight=rect.right-32;
    if (contentRight <= contentLeft) return;
    float cardW=(contentRight-contentLeft-gap*(cols-1))/cols;
    if (cardW <= 0) return;
    float y=rect.top+60;
    for(int i=0;i<8;++i){
        int col=i%4, row=i/4;
        float x=contentLeft+col*(cardW+gap);
        float cy=y+row*(120+gap);
        D2D1_RECT_F card=D2D1::RectF(x, cy, x+cardW, cy+120);
        if (card.right > rect.right || card.bottom > rect.bottom) continue;
        auto bg = (i==m_hoveredTool) ? m_theme.brushElevated : m_theme.brushCard;
        if (!bg || !m_theme.brushBorder) continue;
        D2D1_ROUNDED_RECT rr=D2D1::RoundedRect(card, 12,12);
        rt->FillRoundedRectangle(rr, bg);
        rt->DrawRoundedRectangle(rr, m_theme.brushBorder, 1.0f);

        // Colored icon 36px radius 10 with HUGEICONS distinct
        D2D1_ROUNDED_RECT iconRR=D2D1::RoundedRect(D2D1::RectF(card.left+16, card.top+16, card.left+52, card.top+52), 10,10);
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> iconBrush; 
        HRESULT hr = rt->CreateSolidColorBrush(Theme::FromHex(tools[i].color), &iconBrush);
        if (SUCCEEDED(hr) && iconBrush) {
            rt->FillRoundedRectangle(iconRR, iconBrush.Get());
            D2D1_RECT_F iconInner=D2D1::RectF(card.left+20, card.top+20, card.left+48, card.top+48);
            if (m_d2dFactory && m_theme.brushWhite) {
                IconSystem::DrawIcon(rt, iconInner, tools[i].icon, m_theme.brushWhite, 1.8f);
            }
        }

        if (m_theme.fmtMedium && m_theme.fmtSmall && m_theme.brushTextPrimary) {
            D2D1_RECT_F nameRect=D2D1::RectF(card.left+64, card.top+16, card.right-16, card.top+36);
            rt->DrawText(tools[i].name, (UINT32)wcslen(tools[i].name), m_theme.fmtMedium, nameRect, m_theme.brushTextPrimary);
            D2D1_RECT_F descRect=D2D1::RectF(card.left+64, card.top+38, card.right-16, card.bottom-12);
            if (m_dwFactory) {
                Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
                m_dwFactory->CreateTextLayout(tools[i].desc, (UINT32)wcslen(tools[i].desc), m_theme.fmtSmall, descRect.right-descRect.left, descRect.bottom-descRect.top, &layout);
                if (layout) rt->DrawTextLayout(D2D1::Point2F(descRect.left, descRect.top), layout.Get(), m_theme.brushTextSecondary);
            }
        }
    }

    // Recent Files - exact target
    float contentLeft2=rect.left+32, contentRight2=rect.right-32;
    float recentY=y+2*(120+gap)+32;
    D2D1_RECT_F recentHeader=D2D1::RectF(contentLeft2, recentY, contentRight2, recentY+32);
    if (m_theme.fmtTitle) rt->DrawText(L"Recent Files", 12, m_theme.fmtTitle, D2D1::RectF(recentHeader.left, recentHeader.top, recentHeader.left+200, recentHeader.bottom), m_theme.brushTextPrimary);
    
    float thY=recentY+48;
    if (m_theme.fmtSmall && m_theme.brushTextSecondary) {
        rt->DrawText(L"Name", 4, m_theme.fmtSmall, D2D1::RectF(contentLeft2+40, thY, contentLeft2+400, thY+20), m_theme.brushTextSecondary);
        rt->DrawText(L"Modified Time", 13, m_theme.fmtSmall, D2D1::RectF(contentLeft2+500, thY, contentLeft2+700, thY+20), m_theme.brushTextSecondary);
        rt->DrawText(L"Size", 4, m_theme.fmtSmall, D2D1::RectF(contentRight2-100, thY, contentRight2, thY+20), m_theme.brushTextSecondary);
    }

    float ry=thY+32;
    for(size_t i=0;i<m_recent.size();++i){
        D2D1_RECT_F row=D2D1::RectF(contentLeft2, ry, contentRight2, ry+48);
        if((int)i==m_hoveredFile && m_theme.brushSurface){
            D2D1_ROUNDED_RECT hrr=D2D1::RoundedRect(row, 8,8);
            rt->FillRoundedRectangle(hrr, m_theme.brushSurface);
        }
        if (m_theme.brushAccent) {
            D2D1_ROUNDED_RECT fileIcon=D2D1::RoundedRect(D2D1::RectF(row.left+8, row.top+12, row.left+32, row.top+36), 4,4);
            rt->FillRoundedRectangle(fileIcon, m_theme.brushAccent);
            if (m_theme.brushWhite) {
                IconSystem::DrawIcon(rt, D2D1::RectF(row.left+12, row.top+16, row.left+28, row.top+32), IconId::CreateDoc, m_theme.brushWhite, 1.8f);
            }
            if (m_theme.brushWhite) {
                IconSystem::DrawIcon(rt, D2D1::RectF(row.left+12, row.top+16, row.left+28, row.top+32), IconId::CreateDoc, m_theme.brushWhite, 1.8f);
            }
        }
        if (m_theme.fmtBody && m_theme.fmtSmall && m_theme.brushTextPrimary) {
            D2D1_RECT_F nameRect=D2D1::RectF(row.left+40, row.top+12, row.left+480, row.top+32);
            Microsoft::WRL::ComPtr<IDWriteTextLayout> nameLayout;
            if (m_dwFactory) {
                m_dwFactory->CreateTextLayout(m_recent[i].name.c_str(), (UINT32)m_recent[i].name.length(), m_theme.fmtBody, nameRect.right-nameRect.left, 20, &nameLayout);
                if (nameLayout) {
                    DWRITE_TRIMMING trim={DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
                    nameLayout->SetTrimming(&trim, nullptr);
                    rt->DrawTextLayout(D2D1::Point2F(nameRect.left, nameRect.top), nameLayout.Get(), m_theme.brushTextPrimary);
                }
            }
            D2D1_RECT_F modRect=D2D1::RectF(row.left+500, row.top+12, row.left+700, row.top+32);
            rt->DrawText(m_recent[i].modified.c_str(), (UINT32)m_recent[i].modified.length(), m_theme.fmtSmall, modRect, m_theme.brushTextSecondary);
            D2D1_RECT_F sizeRect=D2D1::RectF(row.right-100, row.top+12, row.right, row.top+32);
            rt->DrawText(m_recent[i].size.c_str(), (UINT32)m_recent[i].size.length(), m_theme.fmtSmall, sizeRect, m_theme.brushTextSecondary);
        }
        ry+=48;
    }
}

void MainWindow::RenderViewer(ID2D1RenderTarget* rt, const D2D1_RECT_F& client){
    if (!rt) return;
    D2D1_RECT_F leftRail=D2D1::RectF(client.left, client.top, client.left+68, client.bottom);
    D2D1_RECT_F topBar=D2D1::RectF(leftRail.right, client.top, client.right, client.top+48);
    D2D1_RECT_F rightRail=D2D1::RectF(client.right-68, topBar.bottom, client.right, client.bottom);
    D2D1_RECT_F toolbar=D2D1::RectF(leftRail.right, topBar.bottom, rightRail.left, topBar.bottom+48);
    D2D1_RECT_F status=D2D1::RectF(leftRail.right, client.bottom-24, rightRail.left, client.bottom);
    D2D1_RECT_F center=D2D1::RectF(leftRail.right, toolbar.bottom, rightRail.left, status.top);

    if (m_theme.brushAppBg) {
        struct TabInfo { const wchar_t* title; bool active; };
        std::vector<TabInfo> tabs = { {L"Daryl L. Logan - Instructor's Solutions Manual to Accompany A First Course...", true}, {L"Logan ch-2 Stiffness method.pdf", false} };
        // TopBar
        rt->FillRectangle(topBar, m_theme.brushTopBar);
        if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(topBar.left, topBar.bottom), D2D1::Point2F(topBar.right, topBar.bottom), m_theme.brushBorder, 1.0f);
        
        // Draw tabs
        float tx = topBar.left + 8;
        for (size_t i=0;i<tabs.size();++i) {
            D2D1_RECT_F tabRect = D2D1::RectF(tx, topBar.top+8, tx+260, topBar.bottom-8);
            D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(tabRect, 8,8);
            auto bg = tabs[i].active ? m_theme.brushSurface : m_theme.brushElevated;
            if (bg) rt->FillRoundedRectangle(rr, bg);
            if (m_theme.brushBorder) rt->DrawRoundedRectangle(rr, m_theme.brushBorder, 1.0f);
            if (m_theme.fmtBody && m_theme.brushTextPrimary) {
                D2D1_RECT_F textRect = D2D1::RectF(tabRect.left+12, tabRect.top+6, tabRect.right-28, tabRect.bottom-6);
                std::wstring title = tabs[i].title;
                if (title.length() > 26) title = title.substr(0,23) + L"...";
                rt->DrawText(title.c_str(), (UINT32)title.length(), m_theme.fmtBody, textRect, m_theme.brushTextPrimary);
            }
            if (m_d2dFactory && m_theme.brushWhite) {
                IconSystem::DrawIcon(rt, D2D1::RectF(tabRect.right-24, tabRect.top+6, tabRect.right-8, tabRect.bottom-6), IconId::Close, m_theme.brushWhite);
            }
            tx += 268;
        }

        // Toolbar
        rt->FillRectangle(toolbar, m_theme.brushToolbar);
        if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(toolbar.left, toolbar.bottom), D2D1::Point2F(toolbar.right, toolbar.bottom), m_theme.brushBorder, 1.0f);
        
        float x = toolbar.left + 16;
        auto drawToolItem = [&](IconId icon, const wchar_t* label, bool active=false) {
            float labelW = wcslen(label) > 0 ? (wcslen(label)*6.0f + 8.0f) : 0.0f;
            float iconW = (icon != IconId::None) ? 24.0f : 0.0f;
            float w = labelW + iconW + 16.0f;
            D2D1_RECT_F tr = D2D1::RectF(x, toolbar.top+8, x+w, toolbar.bottom-8);
            if (active && m_theme.brushElevated) {
                rt->FillRoundedRectangle(D2D1::RoundedRect(tr, 6,6), m_theme.brushElevated);
            }
            if (icon != IconId::None && m_d2dFactory) {
                IconSystem::DrawIcon(rt, D2D1::RectF(tr.left+8, tr.top+4, tr.left+32, tr.top+28), icon, active ? m_theme.brushTextPrimary : m_theme.brushWhite, 1.5f);
            }
            if (labelW > 0 && m_theme.fmtBody && m_theme.brushWhite) {
                rt->DrawText(label, (UINT32)wcslen(label), m_theme.fmtBody, D2D1::RectF(tr.left+8+iconW+4, tr.top+6, tr.right, tr.bottom), active ? m_theme.brushTextPrimary : m_theme.brushWhite);
            }
            x += w + 8; // Uniform distance
        };
        auto drawSep = [&]() {
            x += 4;
            if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(x+2, toolbar.top+12), D2D1::Point2F(x+2, toolbar.bottom-12), m_theme.brushBorder, 1.0f);
            x += 16;
        };
        
        if (m_viewerMode == ViewerMode::View) {
            drawToolItem(IconId::Undo, L""); drawToolItem(IconId::Redo, L""); drawSep();
            drawToolItem(IconId::ZoomOut, L""); drawToolItem(IconId::ZoomIn, L""); drawSep();
            drawToolItem(IconId::Hand, L"Hand", true); drawToolItem(IconId::RectSelect, L"Select"); drawSep();
            drawToolItem(IconId::AddText, L"Add Text"); drawToolItem(IconId::Crop, L"Crop"); 
            drawToolItem(IconId::Combine, L"Combine"); drawToolItem(IconId::Compress, L"Compress");
        } else if (m_viewerMode == ViewerMode::Comment) {
            drawToolItem(IconId::Undo, L""); drawToolItem(IconId::Redo, L""); drawSep();
            drawToolItem(IconId::Highlight, L"Highlight"); drawToolItem(IconId::Underline, L"Underline"); drawToolItem(IconId::Strikethrough, L"Strikethrough"); drawSep();
            drawToolItem(IconId::Pencil, L"Pencil"); drawToolItem(IconId::Eraser, L"Eraser"); drawSep();
            drawToolItem(IconId::Text, L"Text"); drawToolItem(IconId::TextBox, L"Text Box"); drawToolItem(IconId::Rectangle, L"Rectangle");
        } else if (m_viewerMode == ViewerMode::Edit) {
            drawToolItem(IconId::Undo, L""); drawToolItem(IconId::Redo, L""); drawSep();
            drawToolItem(IconId::EditAll, L"Edit All", true); drawToolItem(IconId::AddText, L"Add Text"); drawToolItem(IconId::AddLink, L"Add Link"); drawSep();
            drawToolItem(IconId::Image, L"Image"); drawToolItem(IconId::Watermark, L"Watermark"); drawToolItem(IconId::Background, L"Background");
        } else {
            drawToolItem(IconId::Undo, L""); drawToolItem(IconId::Redo, L""); drawSep();
            drawToolItem(IconId::ZoomOut, L""); drawToolItem(IconId::ZoomIn, L""); drawSep();
            drawToolItem(IconId::Hand, L"Hand"); drawToolItem(IconId::RectSelect, L"Select"); drawSep();
            drawToolItem(IconId::EditAll, L"Edit All", true); drawToolItem(IconId::AddText, L"Add Text");
        }

        // LeftRail with icons
        rt->FillRectangle(leftRail, m_theme.brushSidebarBg);
        if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(leftRail.right, leftRail.top), D2D1::Point2F(leftRail.right, leftRail.bottom), m_theme.brushBorder, 1.0f);
        const wchar_t* railLabels[] = {L"Home", L"Comment", L"Edit", L"Convert", L"View", L"Organize", L"Tools", L"Form"};
        IconId railIcons[] = {IconId::Home, IconId::Comment, IconId::Edit, IconId::Convert, IconId::View, IconId::Organize, IconId::Tools, IconId::Form};
        
        float lr_width = leftRail.right - leftRail.left;
        auto drawLeftRailItem = [&](int idx, float y, bool active) {
            D2D1_RECT_F itemRect = D2D1::RectF(leftRail.left+4, y, leftRail.right-4, y+68);
            auto brush = active ? m_theme.brushAccent : m_theme.brushWhite;
            if (active && m_theme.brushSurface && m_theme.brushAccent) {
                D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(itemRect, 8,8);
                rt->FillRoundedRectangle(rr, m_theme.brushSurface);
            }
            if (m_d2dFactory) {
                float iconSize = 24.0f;
                float ix = leftRail.left + (lr_width - iconSize) / 2.0f;
                D2D1_RECT_F iconRect = D2D1::RectF(ix, itemRect.top+12, ix+iconSize, itemRect.top+12+iconSize);
                IconSystem::DrawIcon(rt, iconRect, railIcons[idx], brush, 1.5f);
            }
            if (m_theme.fmtSmall) {
                float labelW = wcslen(railLabels[idx])*6.0f;
                float ctx = leftRail.left + (lr_width - labelW) / 2.0f;
                D2D1_RECT_F labelRect = D2D1::RectF(ctx, itemRect.top+42, leftRail.right, itemRect.bottom);
                rt->DrawText(railLabels[idx], (UINT32)wcslen(railLabels[idx]), m_theme.fmtSmall, labelRect, brush);
            }
        };

        drawLeftRailItem(0, client.top + 8, false);
        if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(leftRail.left+12, topBar.bottom), D2D1::Point2F(leftRail.right-12, topBar.bottom), m_theme.brushBorder, 1.0f);

        float ly = topBar.bottom + 16;
        for (int i=1;i<8;++i) { // Skip Home
            bool active = (i==1 && m_viewerMode==ViewerMode::Comment) || (i==2 && m_viewerMode==ViewerMode::Edit) || (i==4 && m_viewerMode==ViewerMode::View) || (i==5 && m_viewerMode==ViewerMode::Organize);
            drawLeftRailItem(i, ly, active);
            ly += 76; // Uniform spacing
        }
        
        // RightRail (icons on right)
        rt->FillRectangle(rightRail, m_theme.brushSidebarBg);
        if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(rightRail.left, rightRail.top), D2D1::Point2F(rightRail.left, rightRail.bottom), m_theme.brushBorder, 1.0f);
        if (m_d2dFactory) {
            float ry = rightRail.top + 16;
            auto drawRIcon = [&](IconId id, bool active=false) {
                D2D1_RECT_F ir = D2D1::RectF(rightRail.left+14, ry, rightRail.left+54, ry+40);
                if (active && m_theme.brushSurface) rt->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(rightRail.left+10, ry-4, rightRail.right-10, ry+44), 6,6), m_theme.brushSurface);
                IconSystem::DrawIcon(rt, D2D1::RectF(rightRail.left+20, ry+6, rightRail.left+48, ry+34), id, active ? m_theme.brushAccent : m_theme.brushWhite, 1.8f);
                ry+=56; // Much larger uniform spacing
            };
            drawRIcon(IconId::Thumbnails, true);
            drawRIcon(IconId::Bookmark);
            drawRIcon(IconId::CommentBubble);
            drawRIcon(IconId::Fields);
            drawRIcon(IconId::More);
            
            // Separator
            ry += 4;
            if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(rightRail.left+16, ry), D2D1::Point2F(rightRail.right-16, ry), m_theme.brushBorder, 1.0f);
            ry += 20;

            // Bottom navigation icons
            auto drawRLabel = [&](const wchar_t* lbl) {
                if (m_theme.fmtSmall) {
                    float wl = wcslen(lbl)*6.0f;
                    rt->DrawText(lbl, (UINT32)wcslen(lbl), m_theme.fmtSmall, D2D1::RectF(rightRail.left + (68-wl)/2, ry, rightRail.right, ry+20), m_theme.brushWhite);
                }
                ry+=32; // Larger text spacing
            };
            drawRIcon(IconId::Up);
            drawRLabel(L"1");
            drawRLabel(L"564");
            drawRIcon(IconId::Down);
            drawRIcon(IconId::Fit);
            drawRLabel(L"100%");
            drawRIcon(IconId::ZoomIn);
            drawRIcon(IconId::ZoomOut);
        }

        // Center with grid and PDF page
        rt->FillRectangle(center, m_theme.brushAppBg);
        // Grid 24px
        if (m_theme.brushBorder) {
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> gridBrush;
            rt->CreateSolidColorBrush(D2D1::ColorF(1,1,1,0.03f), &gridBrush);
            if (gridBrush) {
                for (float gx=center.left; gx<center.right; gx+=24) {
                    rt->DrawLine(D2D1::Point2F(gx, center.top), D2D1::Point2F(gx, center.bottom), gridBrush.Get(), 0.5f);
                }
                for (float gy=center.top; gy<center.bottom; gy+=24) {
                    rt->DrawLine(D2D1::Point2F(center.left, gy), D2D1::Point2F(center.right, gy), gridBrush.Get(), 0.5f);
                }
            }
        }
        // PDF page centered with shadow
        D2D1_RECT_F page=D2D1::RectF(center.left+80, center.top+20, center.right-80, center.bottom-20);
        if (page.right > page.left && page.bottom > page.top) {
            // Shadow
            for (int i=1;i<=4;++i) {
                Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> shadow;
                rt->CreateSolidColorBrush(D2D1::ColorF(0,0,0,0.08f / i), &shadow);
                if (shadow) {
                    D2D1_RECT_F sr = D2D1::RectF(page.left+i, page.top+i, page.right+i, page.bottom+i);
                    rt->FillRectangle(sr, shadow.Get());
                }
            }
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> white; rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &white);
            if (white) {
                D2D1_RECT_F whitePage = D2D1::RectF(page.left, page.top, page.right, page.bottom);
                rt->FillRectangle(whitePage, white.Get());
                Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> black; rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &black);
                if (black) {
                    // Simulate black PDF content as in target
                    D2D1_RECT_F blackContent = D2D1::RectF(whitePage.left+20, whitePage.top+20, whitePage.right-20, whitePage.bottom-20);
                    rt->FillRectangle(blackContent, black.Get());
                    if (m_theme.fmtTitle) {
                        D2D1_RECT_F titleRect=D2D1::RectF(blackContent.left+20, blackContent.top+20, blackContent.right-20, blackContent.top+60);
                        rt->DrawText(L"INSTRUCTOR'S SOLUTIONS MANUAL TO ACCOMPANY", 40, m_theme.fmtTitle, titleRect, white.Get());
                    }
                }
            }
        }

        // StatusBar
        rt->FillRectangle(status, m_theme.brushSidebarBg);
        if (m_theme.fmtSmall && m_theme.brushWhite) {
            std::wstring stat = std::to_wstring(m_curPage) + L" / " + std::to_wstring(m_totalPages) + L"  200%";
            rt->DrawText(stat.c_str(), (UINT32)stat.length(), m_theme.fmtSmall, D2D1::RectF(status.left+12, status.top+4, status.right-12, status.bottom), m_theme.brushWhite);
        }
    }
}
void MainWindow::OnMouseMove(int x, int y){ if(m_hwnd) InvalidateRect(m_hwnd,nullptr,FALSE); }
void MainWindow::OnLButtonDown(int x, int y){
    if(m_mode==AppMode::Home){
        if(x>240+32 && y>60 && y<300){
            m_mode=AppMode::Viewer; m_viewerMode=ViewerMode::View; 
            if(m_hwnd) InvalidateRect(m_hwnd,nullptr,FALSE);
        }
    } else {
        if(x<64 && y>48){
            int idx=int((y-48-8)/60);
            if(idx==1){ m_viewerMode=ViewerMode::Comment; }
            if(idx==2){ m_viewerMode=ViewerMode::Edit; }
            if(idx==4){ m_viewerMode=ViewerMode::View; }
            if(idx==5){ m_viewerMode=ViewerMode::Organize; }
            if(m_hwnd) InvalidateRect(m_hwnd,nullptr,FALSE);
        }
    }
}

} // namespace PdfElite




