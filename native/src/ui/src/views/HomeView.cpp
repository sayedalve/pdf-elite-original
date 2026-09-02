#include "HomeView.h"
#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include "../controls/IconRenderer.h"
#include "../NativeDesignSystem.h"
#include "../GraphicsDevice.h"
#include "../../../utils/Logger.h"
#include "../../../core/RecentFilesManager.h"

namespace views {

std::wstring FormatFileSize2(uint64_t bytes) {
    double size = static_cast<double>(bytes);
    const wchar_t* units[] = { L" B", L" KB", L" MB", L" GB" };
    int unit = 0;
    while (size >= 1024.0 && unit < 3) {
        size /= 1024.0;
        unit++;
    }
    std::wstringstream wss;
    wss << std::fixed << std::setprecision(1) << size << units[unit];
    return wss.str();
}

std::wstring FormatDate2(uint64_t ms) {
    if (ms == 0) return L"Unknown";
    auto timePoint = std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds(ms));
    auto t = std::chrono::system_clock::to_time_t(timePoint);
    struct tm tm;
    localtime_s(&tm, &t);
    wchar_t buf[64];
    wcsftime(buf, sizeof(buf), L"%b %d, %Y", &tm);
    return std::wstring(buf);
}

static D2D1_COLOR_F HexToColor(uint32_t hex) {
    return D2D1::ColorF(((hex >> 16) & 0xFF) / 255.0f, ((hex >> 8) & 0xFF) / 255.0f, (hex & 0xFF) / 255.0f, 1.0f);
}

HomeView::HomeView() {
    SetBackgroundColor(design::Colors::Background);
}

void HomeView::Layout(const D2D1_RECT_F& bounds) {
    UIElement::Layout(bounds);
}

void HomeView::Render(ComPtr<ID2D1RenderTarget> target) {
    auto bounds = GetBounds();
    ComPtr<ID2D1SolidColorBrush> bgBrush;
    target->CreateSolidColorBrush(design::Colors::Background, &bgBrush);
    target->FillRectangle(bounds, bgBrush.Get());

    RenderHomeSidebar(target);
    RenderHomeMain(target);
}

void HomeView::RenderHomeSidebar(ComPtr<ID2D1RenderTarget> rt) {
    auto bounds = GetBounds();
    D2D1_RECT_F rect = D2D1::RectF(bounds.left, bounds.top, bounds.left + 240, bounds.bottom);

    ComPtr<ID2D1SolidColorBrush> sidebarBg;
    rt->CreateSolidColorBrush(design::Colors::SidebarBg, &sidebarBg); // SidebarBg is SurfaceElevated in the sample, let's use SidebarBg if it exists, or SurfaceElevated
    // Oh wait, in NativeDesignSystem.h we have SurfaceElevated, not SidebarBg. Let me use SurfaceElevated.
    rt->CreateSolidColorBrush(design::Colors::SurfaceElevated, &sidebarBg);
    if (sidebarBg) rt->FillRectangle(rect, sidebarBg.Get());

    ComPtr<ID2D1SolidColorBrush> borderBrush;
    rt->CreateSolidColorBrush(design::Colors::BorderSubtle, &borderBrush);
    if (borderBrush) rt->DrawLine(D2D1::Point2F(rect.right, rect.top), D2D1::Point2F(rect.right, rect.bottom), borderBrush.Get(), 1.0f);

    ComPtr<ID2D1SolidColorBrush> accent;
    rt->CreateSolidColorBrush(design::Colors::AccentPrimary, &accent);
    ComPtr<ID2D1SolidColorBrush> textPri;
    rt->CreateSolidColorBrush(design::Colors::TextPrimary, &textPri);
    ComPtr<ID2D1SolidColorBrush> textSec;
    rt->CreateSolidColorBrush(design::Colors::TextSecondary, &textSec);

    auto fmtBold = design::FontManager::Instance().GetSectionHeading();
    auto fmtMedium = design::FontManager::Instance().GetToolbar();
    auto fmtBody = design::FontManager::Instance().GetBody();

    if (accent && fmtBold && textPri) {
        D2D1_ROUNDED_RECT logoRR = D2D1::RoundedRect(D2D1::RectF(rect.left+16, rect.top+16, rect.left+48, rect.top+48), 8, 8);
        rt->FillRoundedRectangle(logoRR, accent.Get());
        controls::IconRenderer::DrawIcon(rt, controls::IconType::PDFDocument, D2D1::RectF(rect.left+22, rect.top+22, rect.left+42, rect.top+42), D2D1::ColorF(D2D1::ColorF::White));
        
        D2D1_RECT_F logoText = D2D1::RectF(rect.left+56, rect.top+20, rect.right-16, rect.top+46);
        rt->DrawText(L"PDF Elite", 9, fmtBold, logoText, textPri.Get());
    }

    // Open PDF BLUE button
    m_openBtnRect = D2D1::RectF(rect.left+12, rect.top+72, rect.right-12, rect.top+116);
    float btnWidth = m_openBtnRect.right - m_openBtnRect.left;
    float contentWidth = 20.0f + 8.0f + 65.0f; // icon(20) + padding(8) + approx text width(65)
    float startX = m_openBtnRect.left + (btnWidth - contentWidth) / 2.0f;

    if (accent) {
        D2D1_ROUNDED_RECT openRR = D2D1::RoundedRect(m_openBtnRect, 10, 10);
        rt->FillRoundedRectangle(openRR, accent.Get());
        
        if (fmtMedium) {
            controls::IconRenderer::DrawIcon(rt, controls::IconType::Open, D2D1::RectF(startX, rect.top+84, startX+20, rect.top+104), D2D1::ColorF(D2D1::ColorF::White));
            ComPtr<ID2D1SolidColorBrush> whiteTextBrush;
            rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &whiteTextBrush);
            D2D1_RECT_F openText = D2D1::RectF(startX+28, rect.top+84, m_openBtnRect.right, rect.top+104);
            rt->DrawText(L"Open PDF", 8, fmtMedium, openText, whiteTextBrush.Get());
        }
    }

    // Create PDF
    m_createBtnRect = D2D1::RectF(rect.left+12, rect.top+124, rect.right-12, rect.top+168);
    ComPtr<ID2D1SolidColorBrush> borderStrong;
    rt->CreateSolidColorBrush(design::Colors::BorderStrong, &borderStrong);
    if (borderStrong && fmtMedium && textPri) {
        D2D1_ROUNDED_RECT createRR = D2D1::RoundedRect(m_createBtnRect, 10, 10);
        rt->DrawRoundedRectangle(createRR, borderStrong.Get(), 1.2f);
        
        float createContentWidth = 20.0f + 8.0f + 75.0f; // approx text width "Create PDF"
        float createStartX = m_createBtnRect.left + (btnWidth - createContentWidth) / 2.0f;
        
        controls::IconRenderer::DrawIcon(rt, controls::IconType::Insert, D2D1::RectF(createStartX, rect.top+136, createStartX+20, rect.top+156), design::Colors::TextPrimary);
        D2D1_RECT_F createText = D2D1::RectF(createStartX+28, rect.top+136, m_createBtnRect.right, rect.top+156);
        rt->DrawText(L"Create PDF", 10, fmtMedium, createText, textPri.Get());
    }

    // Nav
    if (fmtBody && textSec) {
        float y = rect.top + 200;
        struct Nav { controls::IconType icon; const wchar_t* label; };
        Nav navs[] = {
            {controls::IconType::Recent, L"Recent Files"},
            {controls::IconType::Star, L"Starred Files"},
            {controls::IconType::Folder, L"Recent Folders"}
        };
        ComPtr<ID2D1SolidColorBrush> surfaceBrush;
        rt->CreateSolidColorBrush(design::Colors::Surface, &surfaceBrush);
        
        for (int i = 0; i < 3; ++i) {
            D2D1_RECT_F item = D2D1::RectF(rect.left+8, y, rect.right-8, y+36);
            m_navRects[i] = item;
            
            bool isSelected = (m_selectedNav >= 0 ? (i == m_selectedNav) : (i == 0));
            if ((isSelected || i == m_hoveredNav) && surfaceBrush) {
                rt->FillRoundedRectangle(D2D1::RoundedRect(item, 8, 8), surfaceBrush.Get());
            }
            
            D2D1_RECT_F iconRect = D2D1::RectF(item.left+12, item.top+10, item.left+28, item.top+26);
            controls::IconRenderer::DrawIcon(rt, navs[i].icon, iconRect, isSelected ? design::Colors::AccentPrimary : design::Colors::TextSecondary);
            
            D2D1_RECT_F tr = D2D1::RectF(item.left+36, item.top+8, item.right-12, item.bottom-8);
            rt->DrawText(navs[i].label, (UINT32)wcslen(navs[i].label), fmtBody, tr, isSelected ? textPri.Get() : textSec.Get());
            y += 40;
        }
    }
}

void HomeView::RenderHomeMain(ComPtr<ID2D1RenderTarget> rt) {
    auto bounds = GetBounds();
    D2D1_RECT_F rect = D2D1::RectF(bounds.left + 240, bounds.top, bounds.right, bounds.bottom);

    ComPtr<ID2D1SolidColorBrush> bgBrush;
    rt->CreateSolidColorBrush(design::Colors::Background, &bgBrush);
    if (bgBrush) rt->FillRectangle(rect, bgBrush.Get());

    ComPtr<ID2D1SolidColorBrush> textPri;
    rt->CreateSolidColorBrush(design::Colors::TextPrimary, &textPri);
    ComPtr<ID2D1SolidColorBrush> textSec;
    rt->CreateSolidColorBrush(design::Colors::TextSecondary, &textSec);

    auto fmtTitle = design::FontManager::Instance().GetPageTitle();
    auto fmtSmall = design::FontManager::Instance().GetCaption();
    auto fmtMedium = design::FontManager::Instance().GetSectionHeading();
    auto fmtBody = design::FontManager::Instance().GetBody();

    if (fmtTitle && textPri) {
        D2D1_RECT_F titleRect = D2D1::RectF(rect.left+32, rect.top+20, rect.left+200, rect.top+44);
        rt->DrawText(L"Quick Tools", 11, fmtTitle, titleRect, textPri.Get());
    }
    
    m_allToolsRect = D2D1::RectF(rect.right-160, rect.top+20, rect.right-32, rect.top+44);
    if (fmtSmall) {
        ComPtr<ID2D1SolidColorBrush> linkBrush;
        rt->CreateSolidColorBrush(m_hoveredAllTools ? design::Colors::AccentPrimary : design::Colors::TextSecondary, &linkBrush);
        if (linkBrush) {
            rt->DrawText(L"All Tools", 9, fmtSmall, m_allToolsRect, linkBrush.Get());
        }
    }

    struct Tool { const wchar_t* name; const wchar_t* desc; uint32_t color; controls::IconType icon; };
    Tool tools[] = {
        {L"Edit PDF", L"Edit text and images in files.", 0xf59e0b, controls::IconType::Edit},
        {L"Convert PDF", L"Convert PDFs to Word, Excel, PPT, etc.", 0x10b981, controls::IconType::Convert},
        {L"OCR PDF", L"Recognize text from scanned files.", 0x8b5cf6, controls::IconType::OCR},
        {L"Add Comments", L"Add highlights, notes, pencil, and other comments.", 0xf87171, controls::IconType::Comment},
        {L"Translate PDF", L"Translate text to other languages.", 0x06b6d4, controls::IconType::Translate},
        {L"Combine Files", L"Combine multiple files into a single PDF.", 0x3b82f6, controls::IconType::Combine},
        {L"Compress PDF", L"Reduce PDF file size.", 0x22c55e, controls::IconType::Compress},
        {L"Batch PDFs", L"Batch convert, create, print, OCR PDFs, etc.", 0x10b981, controls::IconType::Batch},
    };

    float cols = 4, gap = 16;
    float contentLeft = rect.left+32, contentRight = rect.right-32;
    float cardW = 0;
    if (contentRight > contentLeft) {
        cardW = (contentRight - contentLeft - gap*(cols-1)) / cols;
    }
    
    ComPtr<ID2D1SolidColorBrush> cardBg;
    rt->CreateSolidColorBrush(design::Colors::SurfaceElevated, &cardBg);
    ComPtr<ID2D1SolidColorBrush> cardHover;
    rt->CreateSolidColorBrush(design::Colors::SurfaceHover, &cardHover);
    ComPtr<ID2D1SolidColorBrush> borderBrush;
    rt->CreateSolidColorBrush(design::Colors::BorderSubtle, &borderBrush);

    float y = rect.top + 60;
    for (int i = 0; i < 8; ++i) {
        if (cardW <= 0) break;
        int col = i % 4, row = i / 4;
        float x = contentLeft + col * (cardW + gap);
        float cy = y + row * (120 + gap);
        D2D1_RECT_F card = D2D1::RectF(x, cy, x + cardW, cy + 120);
        m_toolRects[i] = card;
        
        if (card.right > rect.right || card.bottom > rect.bottom) continue;
        
        auto bg = (i == m_hoveredTool) ? cardHover.Get() : cardBg.Get();
        if (bg && borderBrush) {
            D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(card, 12, 12);
            rt->FillRoundedRectangle(rr, bg);
            rt->DrawRoundedRectangle(rr, borderBrush.Get(), 1.0f);
        }

        D2D1_ROUNDED_RECT iconRR = D2D1::RoundedRect(D2D1::RectF(card.left+16, card.top+16, card.left+52, card.top+52), 10, 10);
        ComPtr<ID2D1SolidColorBrush> iconBrush;
        rt->CreateSolidColorBrush(HexToColor(tools[i].color), &iconBrush);
        if (iconBrush) {
            rt->FillRoundedRectangle(iconRR, iconBrush.Get());
            D2D1_RECT_F iconInner = D2D1::RectF(card.left+20, card.top+20, card.left+48, card.top+48);
            controls::IconRenderer::DrawIcon(rt, tools[i].icon, iconInner, D2D1::ColorF(D2D1::ColorF::White));
        }

        if (fmtMedium && fmtSmall && textPri && textSec) {
            D2D1_RECT_F nameRect = D2D1::RectF(card.left+64, card.top+16, card.right-16, card.top+36);
            rt->DrawText(tools[i].name, (UINT32)wcslen(tools[i].name), fmtMedium, nameRect, textPri.Get());
            
            D2D1_RECT_F descRect = D2D1::RectF(card.left+64, card.top+38, card.right-16, card.bottom-12);
            float dw = descRect.right - descRect.left;
            float dh = descRect.bottom - descRect.top;
            auto factory = GraphicsDevice::Instance().GetDWriteFactory();
            if (factory && dw > 0.0f && dh > 0.0f) {
                ComPtr<IDWriteTextLayout> layout;
                factory->CreateTextLayout(tools[i].desc, (UINT32)wcslen(tools[i].desc), fmtSmall, dw, dh, &layout);
                if (layout) rt->DrawTextLayout(D2D1::Point2F(descRect.left, descRect.top), layout.Get(), textSec.Get());
            }
        }
    }

    // Dynamic Navigation Content: Recent Files (0), Starred Files (1), Recent Folders (2)
    float contentLeft2 = rect.left+32, contentRight2 = rect.right-32;
    float recentY = y + 2*(120+gap) + 32;
    D2D1_RECT_F recentHeader = D2D1::RectF(contentLeft2, recentY, contentRight2, recentY+32);

    const wchar_t* navTitle = L"Recent Files";
    if (m_selectedNav == 1) navTitle = L"Starred Files";
    else if (m_selectedNav == 2) navTitle = L"Recent Folders";

    if (fmtTitle && textPri) {
        rt->DrawText(navTitle, (UINT32)wcslen(navTitle), fmtTitle, D2D1::RectF(recentHeader.left, recentHeader.top, recentHeader.left+250, recentHeader.bottom), textPri.Get());
    }

    float thY = recentY + 48;
    rt->PushAxisAlignedClip(D2D1::RectF(contentLeft2, thY, contentRight2, bounds.bottom), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    if (fmtSmall && textSec) {
        if (m_selectedNav == 2) {
            rt->DrawText(L"Folder Name", 11, fmtSmall, D2D1::RectF(contentLeft2+40, thY, contentLeft2+400, thY+20), textSec.Get());
            rt->DrawText(L"Path", 4, fmtSmall, D2D1::RectF(contentLeft2+450, thY, contentLeft2+750, thY+20), textSec.Get());
            rt->DrawText(L"Items", 5, fmtSmall, D2D1::RectF(contentRight2-100, thY, contentRight2, thY+20), textSec.Get());
        } else {
            rt->DrawText(L"Name", 4, fmtSmall, D2D1::RectF(contentLeft2+40, thY, contentLeft2+400, thY+20), textSec.Get());
            rt->DrawText(L"Modified Time", 13, fmtSmall, D2D1::RectF(contentLeft2+500, thY, contentLeft2+700, thY+20), textSec.Get());
            rt->DrawText(L"Size", 4, fmtSmall, D2D1::RectF(contentRight2-100, thY, contentRight2, thY+20), textSec.Get());
        }
    }

    m_fileRects.clear();
    m_starRects.clear();
    m_displayedPaths.clear();

    float ry = thY + 32 + m_scrollOffset; printf("[HOMEVIEW] ry = %f, m_scrollOffset = %f\n", ry, m_scrollOffset); fflush(stdout);
    ComPtr<ID2D1SolidColorBrush> surfaceBrush;
    rt->CreateSolidColorBrush(design::Colors::SurfaceHover, &surfaceBrush);
    ComPtr<ID2D1SolidColorBrush> accentBrush;
    rt->CreateSolidColorBrush(design::Colors::AccentPrimary, &accentBrush);

    auto renderEmptyState = [&](const wchar_t* emptyTitle, const wchar_t* emptySubtitle) {
        if (fmtMedium && fmtSmall && textPri && textSec) {
            D2D1_RECT_F card = D2D1::RectF(contentLeft2, ry, contentRight2, ry + 100);
            D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(card, 12, 12);
            if (cardBg && borderBrush) {
                rt->FillRoundedRectangle(rr, cardBg.Get());
                rt->DrawRoundedRectangle(rr, borderBrush.Get(), 1.0f);
            }
            D2D1_RECT_F tRect = D2D1::RectF(contentLeft2 + 24, ry + 24, contentRight2 - 24, ry + 48);
            rt->DrawText(emptyTitle, (UINT32)wcslen(emptyTitle), fmtMedium, tRect, textPri.Get());
            D2D1_RECT_F sRect = D2D1::RectF(contentLeft2 + 24, ry + 52, contentRight2 - 24, ry + 76);
            rt->DrawText(emptySubtitle, (UINT32)wcslen(emptySubtitle), fmtSmall, sRect, textSec.Get());
        }
    };

    if (m_selectedNav == 1) {
        // Starred Files
        auto starredFiles = core::RecentFilesManager::Instance().GetStarredFiles();
        if (starredFiles.empty()) {
            renderEmptyState(L"No starred files yet", L"Click the star icon next to any recent file to keep it easily accessible.");
        } else {
            m_fileRects.resize(starredFiles.size());
            m_starRects.resize(starredFiles.size());
            m_displayedPaths.resize(starredFiles.size());

            for (size_t i = 0; i < starredFiles.size(); ++i) {
                m_displayedPaths[i] = starredFiles[i].path;
                D2D1_RECT_F row = D2D1::RectF(contentLeft2, ry, contentRight2, ry+48);
                m_fileRects[i] = row;
                m_starRects[i] = D2D1::RectF(contentRight2 - 36, ry + 12, contentRight2 - 12, ry + 36);
                
                if ((int)i == m_hoveredFile && surfaceBrush) {
                    D2D1_ROUNDED_RECT hrr = D2D1::RoundedRect(row, 8, 8);
                    rt->FillRoundedRectangle(hrr, surfaceBrush.Get());
                }
                
                if (accentBrush) {
                    D2D1_ROUNDED_RECT fileIcon = D2D1::RoundedRect(D2D1::RectF(row.left+8, row.top+12, row.left+32, row.top+36), 4, 4);
                    rt->FillRoundedRectangle(fileIcon, accentBrush.Get());
                    controls::IconRenderer::DrawIcon(rt, controls::IconType::PDFDocument, D2D1::RectF(row.left+12, row.top+16, row.left+28, row.top+32), D2D1::ColorF(D2D1::ColorF::White));
                }
                
                if (fmtBody && fmtSmall && textPri && textSec) {
                    D2D1_RECT_F nameRect = D2D1::RectF(row.left+40, row.top+12, row.left+480, row.top+32);
                    auto factory = GraphicsDevice::Instance().GetDWriteFactory();
                    if (factory) {
                        ComPtr<IDWriteTextLayout> nameLayout;
                        factory->CreateTextLayout(starredFiles[i].filename.c_str(), (UINT32)starredFiles[i].filename.length(), fmtBody, nameRect.right-nameRect.left, 20, &nameLayout);
                        if (nameLayout) {
                            DWRITE_TRIMMING trim = {DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
                            nameLayout->SetTrimming(&trim, nullptr);
                            rt->DrawTextLayout(D2D1::Point2F(nameRect.left, nameRect.top), nameLayout.Get(), textPri.Get());
                        }
                    }
                    
                    std::wstring dateStr = FormatDate2(starredFiles[i].lastAccessed);
                    D2D1_RECT_F modRect = D2D1::RectF(row.left+500, row.top+12, row.left+700, row.top+32);
                    rt->DrawText(dateStr.c_str(), (UINT32)dateStr.length(), fmtSmall, modRect, textSec.Get());
                    
                    std::wstring sizeStr = FormatFileSize2(starredFiles[i].fileSize);
                    D2D1_RECT_F sizeRect = D2D1::RectF(row.right-100, row.top+12, row.right-40, row.top+32);
                    rt->DrawText(sizeStr.c_str(), (UINT32)sizeStr.length(), fmtSmall, sizeRect, textSec.Get());

                    controls::IconRenderer::DrawIcon(rt, controls::IconType::Star, m_starRects[i], HexToColor(0xf59e0b));
                }
                ry += 48;
            }
        }
    } else if (m_selectedNav == 2) {
        // Recent Folders
        auto folders = core::RecentFilesManager::Instance().GetRecentFolders();
        if (folders.empty()) {
            renderEmptyState(L"No recent folders", L"Folders containing your opened documents will appear here.");
        } else {
            m_fileRects.resize(folders.size());
            m_displayedPaths.resize(folders.size());

            for (size_t i = 0; i < folders.size(); ++i) {
                m_displayedPaths[i] = folders[i].path;
                D2D1_RECT_F row = D2D1::RectF(contentLeft2, ry, contentRight2, ry+48);
                m_fileRects[i] = row;

                if ((int)i == m_hoveredFile && surfaceBrush) {
                    D2D1_ROUNDED_RECT hrr = D2D1::RoundedRect(row, 8, 8);
                    rt->FillRoundedRectangle(hrr, surfaceBrush.Get());
                }

                D2D1_RECT_F folderIconRect = D2D1::RectF(row.left+8, row.top+12, row.left+32, row.top+36);
                controls::IconRenderer::DrawIcon(rt, controls::IconType::Folder, folderIconRect, design::Colors::AccentPrimary);

                if (fmtBody && fmtSmall && textPri && textSec) {
                    D2D1_RECT_F nameRect = D2D1::RectF(row.left+40, row.top+12, row.left+430, row.top+32);
                    auto factory = GraphicsDevice::Instance().GetDWriteFactory();
                    if (factory) {
                        ComPtr<IDWriteTextLayout> nameLayout;
                        factory->CreateTextLayout(folders[i].folderName.c_str(), (UINT32)folders[i].folderName.length(), fmtBody, nameRect.right-nameRect.left, 20, &nameLayout);
                        if (nameLayout) {
                            DWRITE_TRIMMING trim = {DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
                            nameLayout->SetTrimming(&trim, nullptr);
                            rt->DrawTextLayout(D2D1::Point2F(nameRect.left, nameRect.top), nameLayout.Get(), textPri.Get());
                        }
                    }

                    D2D1_RECT_F pathRect = D2D1::RectF(row.left+450, row.top+12, row.left+750, row.top+32);
                    if (factory) {
                        ComPtr<IDWriteTextLayout> pathLayout;
                        factory->CreateTextLayout(folders[i].path.c_str(), (UINT32)folders[i].path.length(), fmtSmall, pathRect.right-pathRect.left, 20, &pathLayout);
                        if (pathLayout) {
                            DWRITE_TRIMMING trim = {DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
                            pathLayout->SetTrimming(&trim, nullptr);
                            rt->DrawTextLayout(D2D1::Point2F(pathRect.left, pathRect.top), pathLayout.Get(), textSec.Get());
                        }
                    }

                    std::wstring itemsStr = std::to_wstring(folders[i].fileCount) + (folders[i].fileCount == 1 ? L" file" : L" files");
                    D2D1_RECT_F itemsRect = D2D1::RectF(row.right-100, row.top+12, row.right, row.top+32);
                    rt->DrawText(itemsStr.c_str(), (UINT32)itemsStr.length(), fmtSmall, itemsRect, textSec.Get());
                }
                ry += 48;
            }
        }
    } else {
        // Recent Files (0)
        auto recentFiles = core::RecentFilesManager::Instance().GetRecentFiles();
for(int i=0; i<30; ++i) { 
    core::RecentFile rf; 
    rf.path = L"C:\\Fake\\File" + std::to_wstring(i) + L".pdf"; 
    rf.filename = L"Fake File " + std::to_wstring(i) + L".pdf";
    rf.isStarred = false; 
    recentFiles.push_back(rf); 
}
        if (recentFiles.empty()) {
            renderEmptyState(L"No recent files yet", L"Open a PDF file to start reading and editing.");
        } else {
            m_fileRects.resize(recentFiles.size());
            m_starRects.resize(recentFiles.size());
            m_displayedPaths.resize(recentFiles.size());

            for (size_t i = 0; i < recentFiles.size(); ++i) {
                m_displayedPaths[i] = recentFiles[i].path;
                D2D1_RECT_F row = D2D1::RectF(contentLeft2, ry, contentRight2, ry+48);
                m_fileRects[i] = row;
                m_starRects[i] = D2D1::RectF(contentRight2 - 36, ry + 12, contentRight2 - 12, ry + 36);
                
                if ((int)i == m_hoveredFile && surfaceBrush) {
                    D2D1_ROUNDED_RECT hrr = D2D1::RoundedRect(row, 8, 8);
                    rt->FillRoundedRectangle(hrr, surfaceBrush.Get());
                }
                
                if (accentBrush) {
                    D2D1_ROUNDED_RECT fileIcon = D2D1::RoundedRect(D2D1::RectF(row.left+8, row.top+12, row.left+32, row.top+36), 4, 4);
                    rt->FillRoundedRectangle(fileIcon, accentBrush.Get());
                    controls::IconRenderer::DrawIcon(rt, controls::IconType::PDFDocument, D2D1::RectF(row.left+12, row.top+16, row.left+28, row.top+32), D2D1::ColorF(D2D1::ColorF::White));
                }
                
                if (fmtBody && fmtSmall && textPri && textSec) {
                    D2D1_RECT_F nameRect = D2D1::RectF(row.left+40, row.top+12, row.left+480, row.top+32);
                    auto factory = GraphicsDevice::Instance().GetDWriteFactory();
                    if (factory) {
                        ComPtr<IDWriteTextLayout> nameLayout;
                        factory->CreateTextLayout(recentFiles[i].filename.c_str(), (UINT32)recentFiles[i].filename.length(), fmtBody, nameRect.right-nameRect.left, 20, &nameLayout);
                        if (nameLayout) {
                            DWRITE_TRIMMING trim = {DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
                            nameLayout->SetTrimming(&trim, nullptr);
                            rt->DrawTextLayout(D2D1::Point2F(nameRect.left, nameRect.top), nameLayout.Get(), textPri.Get());
                        }
                    }
                    
                    std::wstring dateStr = FormatDate2(recentFiles[i].lastAccessed);
                    D2D1_RECT_F modRect = D2D1::RectF(row.left+500, row.top+12, row.left+700, row.top+32);
                    rt->DrawText(dateStr.c_str(), (UINT32)dateStr.length(), fmtSmall, modRect, textSec.Get());
                    
                    std::wstring sizeStr = FormatFileSize2(recentFiles[i].fileSize);
                    D2D1_RECT_F sizeRect = D2D1::RectF(row.right-100, row.top+12, row.right-40, row.top+32);
                    rt->DrawText(sizeStr.c_str(), (UINT32)sizeStr.length(), fmtSmall, sizeRect, textSec.Get());

                    controls::IconRenderer::DrawIcon(rt, controls::IconType::Star, m_starRects[i], recentFiles[i].isStarred ? HexToColor(0xf59e0b) : design::Colors::TextSecondary);
                }
                ry += 48;
            }
        }
    }
    rt->PopAxisAlignedClip();

    // Scrollbar
    auto rfiles = core::RecentFilesManager::Instance().GetRecentFiles();
    float totalHeight = 484 + static_cast<float>(rfiles.size()) * 48.0f; // roughly where items start + items
    float viewHeight = m_bounds.bottom - m_bounds.top;
    if (totalHeight > viewHeight && viewHeight > 0) {
        float maxScroll = totalHeight - viewHeight;
        float scrollbarHeight = std::max(40.0f, (viewHeight / totalHeight) * viewHeight);
        float scrollbarY = (-m_scrollOffset / maxScroll) * (viewHeight - scrollbarHeight);
        
        D2D1_RECT_F sbRect = D2D1::RectF(m_bounds.right - 8, m_bounds.top + scrollbarY, m_bounds.right - 2, m_bounds.top + scrollbarY + scrollbarHeight);
        ComPtr<ID2D1SolidColorBrush> sbBrush;
        rt->CreateSolidColorBrush(D2D1::ColorF(0,0,0,0.2f), &sbBrush);
        rt->FillRectangle(sbRect, sbBrush.Get());
    }
}

bool HomeView::HitTest(float x, float y) {
    if (!framework::Panel::HitTest(x, y)) return false;
    return true; // We take up full bounds and respond to clicks
}

void HomeView::OnMouseMove(float x, float y) {
    bool dirty = false;
    
    // Sidebar
    int newHoveredNav = -1;
    for (int i=0; i<3; ++i) {
        if (x >= m_navRects[i].left && x <= m_navRects[i].right && y >= m_navRects[i].top && y <= m_navRects[i].bottom) {
            newHoveredNav = i;
            break;
        }
    }
    if (newHoveredNav != m_hoveredNav) {
        m_hoveredNav = newHoveredNav;
        dirty = true;
    }
    
    // All Tools Link
    bool newHoveredAllTools = (x >= m_allToolsRect.left && x <= m_allToolsRect.right && y >= m_allToolsRect.top && y <= m_allToolsRect.bottom);
    if (newHoveredAllTools != m_hoveredAllTools) {
        m_hoveredAllTools = newHoveredAllTools;
        dirty = true;
    }

    // Main tools
    int newHoveredTool = -1;
    for (int i=0; i<8; ++i) {
        if (x >= m_toolRects[i].left && x <= m_toolRects[i].right && y >= m_toolRects[i].top && y <= m_toolRects[i].bottom) {
            newHoveredTool = i;
            break;
        }
    }
    if (newHoveredTool != m_hoveredTool) {
        m_hoveredTool = newHoveredTool;
        dirty = true;
    }
    
    // Star icons
    int newHoveredStar = -1;
    for (int i=0; i<(int)m_starRects.size(); ++i) {
        if (x >= m_starRects[i].left && x <= m_starRects[i].right && y >= m_starRects[i].top && y <= m_starRects[i].bottom) {
            newHoveredStar = i;
            break;
        }
    }
    if (newHoveredStar != m_hoveredStar) {
        m_hoveredStar = newHoveredStar;
        dirty = true;
    }

    // Recent files / folders
    int newHoveredFile = -1;
    float yGapStart = m_bounds.top + 64 + 2*(120+16) + 32 + 48; // equivalent to thY
    if (y >= yGapStart) {
        for (int i=0; i<(int)m_fileRects.size(); ++i) {
            if (x >= m_fileRects[i].left && x <= m_fileRects[i].right && y >= m_fileRects[i].top && y <= m_fileRects[i].bottom) {
                newHoveredFile = i;
                break;
            }
        }
    }
    if (newHoveredFile != m_hoveredFile) {
        m_hoveredFile = newHoveredFile;
        dirty = true;
    }
}

void HomeView::OnMouseDown(float x, float y) {
    if (x >= m_openBtnRect.left && x <= m_openBtnRect.right && y >= m_openBtnRect.top && y <= m_openBtnRect.bottom) {
        if (onOpenRequest) onOpenRequest();
        return;
    }

    if (x >= m_createBtnRect.left && x <= m_createBtnRect.right && y >= m_createBtnRect.top && y <= m_createBtnRect.bottom) {
        if (onCreateRequest) {
            onCreateRequest();
        } else if (onToolRequest) {
            onToolRequest(L"Create PDF");
        }
        return;
    }

    // Sidebar navigation items
    for (int i = 0; i < 3; ++i) {
        if (x >= m_navRects[i].left && x <= m_navRects[i].right && y >= m_navRects[i].top && y <= m_navRects[i].bottom) {
            m_selectedNav = i;
            if (onNavRequest) onNavRequest(i);
            return;
        }
    }

    // "All Tools" header link
    if (x >= m_allToolsRect.left && x <= m_allToolsRect.right && y >= m_allToolsRect.top && y <= m_allToolsRect.bottom) {
        if (onToolRequest) onToolRequest(L"All Tools");
        return;
    }

    // Main Quick Tools cards
    struct Tool { const wchar_t* name; const wchar_t* desc; uint32_t color; controls::IconType icon; };
    Tool tools[] = {
        {L"Edit PDF", L"Edit text and images in files.", 0xf59e0b, controls::IconType::Edit},
        {L"Convert PDF", L"Convert PDFs to Word, Excel, PPT, etc.", 0x10b981, controls::IconType::Convert},
        {L"OCR PDF", L"Recognize text from scanned files.", 0x8b5cf6, controls::IconType::OCR},
        {L"Add Comments", L"Add highlights, notes, pencil, and other comments.", 0xf87171, controls::IconType::Comment},
        {L"Translate PDF", L"Translate text to other languages.", 0x06b6d4, controls::IconType::Translate},
        {L"Combine Files", L"Combine multiple files into a single PDF.", 0x3b82f6, controls::IconType::Combine},
        {L"Compress PDF", L"Reduce PDF file size.", 0x22c55e, controls::IconType::Compress},
        {L"Batch PDFs", L"Batch convert, create, print, OCR PDFs, etc.", 0x10b981, controls::IconType::Batch},
    };

    for (int i = 0; i < 8; ++i) {
        if (x >= m_toolRects[i].left && x <= m_toolRects[i].right && y >= m_toolRects[i].top && y <= m_toolRects[i].bottom) {
            if (onToolRequest) onToolRequest(tools[i].name);
            return;
        }
    }

    // Star icon clicks
    for (size_t i = 0; i < m_starRects.size() && i < m_displayedPaths.size(); ++i) {
        if (x >= m_starRects[i].left && x <= m_starRects[i].right && y >= m_starRects[i].top && y <= m_starRects[i].bottom) {
            core::RecentFilesManager::Instance().ToggleStar(m_displayedPaths[i]);
            return;
        }
    }

    // Row clicks (Recent Files / Starred Files / Recent Folders)
    float yGapStartClick = m_bounds.top + 64 + 2*(120+16) + 32 + 48;
    if (y >= yGapStartClick) {
        for (size_t i = 0; i < m_fileRects.size() && i < m_displayedPaths.size(); ++i) {
            if (x >= m_fileRects[i].left && x <= m_fileRects[i].right && y >= m_fileRects[i].top && y <= m_fileRects[i].bottom) {
            if (m_selectedNav == 2) {
                if (onOpenFolderRequest) {
                    onOpenFolderRequest(m_displayedPaths[i]);
                } else {
                    ShellExecuteW(nullptr, L"open", m_displayedPaths[i].c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                }
            } else {
                if (onOpenFileRequest) onOpenFileRequest(m_displayedPaths[i]);
            }
            return;
        }
    }
    } // close yGapStartClick
}





void HomeView::OnMouseWheel(float delta) {
    auto recentFiles = core::RecentFilesManager::Instance().GetRecentFiles();
    auto rfiles = core::RecentFilesManager::Instance().GetRecentFiles();
    float totalHeight = 484 + static_cast<float>(rfiles.size()) * 48.0f;
    float viewHeight = m_bounds.bottom - m_bounds.top;
    float maxScroll = std::max(0.0f, totalHeight - viewHeight);

    m_scrollOffset += delta;
    if (m_scrollOffset > 0) m_scrollOffset = 0.0f;
    if (m_scrollOffset < -maxScroll) m_scrollOffset = -maxScroll;
    
    // Invalidate to repaint
    
}
} // namespace views
