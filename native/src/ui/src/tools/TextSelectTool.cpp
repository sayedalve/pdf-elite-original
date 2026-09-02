#include "../../include/tools/TextSelectTool.h"
#include "../../../core/interfaces/dom/ITextPage.h"
#include "../../../core/interfaces/dom/IAnnotation.h"
#include "../../../pdf_engine/src/commands/AnnotationCommands.h"
#include "../../../core/Clipboard.h"
#include "../controls/IconRenderer.h"
#include <cmath>
#include <algorithm>

namespace ui::tools {

static constexpr float kMultiClickDistanceThresholdDip = 5.0f;
static constexpr uint64_t kDefaultDoubleClickTimeMs = 500;

TextSelectTool::TextSelectTool(AutoAction autoAction) : m_autoAction(autoAction) {
    BuildToolbar();
}
TextSelectTool::~TextSelectTool() = default;

void TextSelectTool::BuildToolbar() {
    m_toolbarButtons.clear();
    m_toolbarButtons.push_back({ToolbarButton::Action::Highlight, L"Highlight", {}, false, false, 255, 255, 0, 255, static_cast<int>(::controls::IconType::Highlight), false});
    
    m_toolbarButtons.push_back({ToolbarButton::Action::Underline, L"Underline", {}, false, false, 0, 180, 0, 255, static_cast<int>(::controls::IconType::Underline), false});
    m_toolbarButtons.push_back({ToolbarButton::Action::StrikeOut, L"Strikethrough", {}, false, false, 255, 0, 0, 255, static_cast<int>(::controls::IconType::Strikethrough), false});
// separator
    m_toolbarButtons.push_back({ToolbarButton::Action::Copy, L"", {}, false, false, 0, 0, 0, 0, static_cast<int>(::controls::IconType::None), true});
    
    m_toolbarButtons.push_back({ToolbarButton::Action::Comment, L"Comment", {}, false, false, 0, 0, 0, 0, static_cast<int>(::controls::IconType::Chat), false});
    m_toolbarButtons.push_back({ToolbarButton::Action::Copy, L"Copy", {}, false, false, 0, 0, 0, 0, static_cast<int>(::controls::IconType::Copy), false});
    
    m_colorButtons.clear();
    // Typical highlight colors: Yellow, Green, Blue, Pink, Purple
    m_colorButtons.push_back({ToolbarButton::Action::ColorPick, L"Yellow", {}, false, false, 255, 255, 0, 255, static_cast<int>(::controls::IconType::None), false});
    m_colorButtons.push_back({ToolbarButton::Action::ColorPick, L"Green", {}, false, false, 0, 255, 0, 255, static_cast<int>(::controls::IconType::None), false});
    m_colorButtons.push_back({ToolbarButton::Action::ColorPick, L"Blue", {}, false, false, 0, 180, 255, 255, static_cast<int>(::controls::IconType::None), false});
    m_colorButtons.push_back({ToolbarButton::Action::ColorPick, L"Pink", {}, false, false, 255, 105, 180, 255, static_cast<int>(::controls::IconType::None), false});
    m_colorButtons.push_back({ToolbarButton::Action::ColorPick, L"Purple", {}, false, false, 180, 0, 255, 255, static_cast<int>(::controls::IconType::None), false});
}

void TextSelectTool::UpdateToolbarLayout(float minX, float minY, float maxX, float maxY, ToolContext& context) {
    if (!context.canvasToDip) return;
    
    // Width calculations
    float btnSize = 32.0f;
    float padding = 4.0f;
    float sepWidth = 12.0f;
    
    float totalW = padding;
    for (const auto& btn : m_toolbarButtons) {
        if (btn.isSeparator) totalW += sepWidth;
        else totalW += btnSize + padding;
    }
    
    float cx = (minX + maxX) * 0.5f;
    float tbLeft = cx - totalW * 0.5f;
    float tbTop = minY - btnSize - 16.0f; // 16px above selection
    
    // If it goes above the window, put it below
    if (tbTop < 50.0f) {
        tbTop = maxY + 16.0f;
    }
    
    m_toolbarPosDip = { tbLeft, tbTop };
    
    float currentX = tbLeft + padding;
    for (auto& btn : m_toolbarButtons) {
        if (btn.isSeparator) {
            btn.rect = { currentX, tbTop, currentX + sepWidth, tbTop + btnSize };
            currentX += sepWidth;
        } else {
            
          if (btn.action == ToolbarButton::Action::Highlight) {
              btn.rect = { currentX, tbTop, currentX + btnSize + 12.0f, tbTop + btnSize };
              currentX += btnSize + 12.0f + padding;
          } else {
              btn.rect = { currentX, tbTop, currentX + btnSize, tbTop + btnSize };
              currentX += btnSize + padding;
          }
            currentX += btnSize + padding;
        }
    }
    
    if (m_colorPaletteOpen) {
        float paletteTotalW = padding + (btnSize + padding) * m_colorButtons.size();
        float palLeft = currentX - paletteTotalW; // align right
        float palTop = tbTop + btnSize + 8.0f;
        
        float palX = palLeft + padding;
        for (auto& btn : m_colorButtons) {
            btn.rect = { palX, palTop, palX + btnSize, palTop + btnSize };
            palX += btnSize + padding;
        }
    }
}

void TextSelectTool::OnActivate(ToolContext& context) {
    (void)context;
    m_state = ToolState::Idle;
    m_isSelecting = false;
    m_clickCount = 1;
    m_lastClickTimeMs = 0;
    m_anchorPageIndex = -1;
    m_anchorCharIndex = -1;
}

void TextSelectTool::OnDeactivate(ToolContext& context) {
    Cancel(context);
}

void TextSelectTool::Cancel(ToolContext& context) {
    if (m_isSelecting && context.captureService) {
        context.captureService->ReleaseCapture(this);
    }
    m_isSelecting = false;
    m_state = ToolState::Idle;
    m_clickCount = 1;
    m_anchorPageIndex = -1;
    m_anchorCharIndex = -1;
    if (context.invalidateView) {
        context.invalidateView();
    }
}

input::EventResult TextSelectTool::OnPointerDown(const input::PointerEvent& event, ToolContext& context) {
    if (event.button != input::PointerButton::Left) {
        return input::EventResult::Ignored;
    }

    // 0. Check mini-toolbar clicks first
    if (m_selectionModel.HasTextSelection() && context.canvasToDip) {
        PointF dipPt = context.canvasToDip(event.canvasPoint);
        
        auto checkClick = [&](std::vector<ToolbarButton>& buttons) -> ToolbarButton* {
            for (auto& btn : buttons) {
                if (!btn.isSeparator && dipPt.x >= btn.rect.left && dipPt.x <= btn.rect.right &&
                    dipPt.y >= btn.rect.top && dipPt.y <= btn.rect.bottom) {
                    return &btn;
                }
            }
            return nullptr;
        };

        auto addAnnot = [&](core::interfaces::dom::AnnotationType type) {
            CreateAnnotation(type, context);
        };

        ToolbarButton* clickedColor = m_colorPaletteOpen ? checkClick(m_colorButtons) : nullptr;
        ToolbarButton* clickedBtn = checkClick(m_toolbarButtons);
        
        if (clickedColor) {
            m_currentColor[0] = clickedColor->r;
            m_currentColor[1] = clickedColor->g;
            m_currentColor[2] = clickedColor->b;
            m_currentColor[3] = clickedColor->a;
            m_colorPaletteOpen = false;
            
            // Also apply highlight if color is clicked
            addAnnot(core::interfaces::dom::AnnotationType::Highlight);
            return input::EventResult::Handled;
        } else if (clickedBtn) {
            if (clickedBtn->action == ToolbarButton::Action::Copy) {
                std::wstring text = m_selectionModel.GetSelectedText();
                if (!text.empty()) {
                    core::Clipboard::SetText(context.hwnd, text);
                }
                m_selectionModel.ClearTextSelection();
                if (context.invalidateView) context.invalidateView();
                return input::EventResult::Handled;
            } else if (clickedBtn->action == ToolbarButton::Action::ColorPick) {
                m_colorPaletteOpen = !m_colorPaletteOpen;
                if (context.invalidateView) context.invalidateView();
                return input::EventResult::Handled;
            } else if (clickedBtn->action == ToolbarButton::Action::Highlight) {
                // If clicked right side (arrow)
                if (dipPt.x > clickedBtn->rect.right - 16.0f) {
                    m_colorPaletteOpen = !m_colorPaletteOpen;
                } else {
                    addAnnot(core::interfaces::dom::AnnotationType::Highlight);
                }
                if (context.invalidateView) context.invalidateView();
                return input::EventResult::Handled;
            } else if (clickedBtn->action == ToolbarButton::Action::Underline) {
                addAnnot(core::interfaces::dom::AnnotationType::Underline);
                return input::EventResult::Handled;
            } else if (clickedBtn->action == ToolbarButton::Action::StrikeOut) {
                addAnnot(core::interfaces::dom::AnnotationType::StrikeOut);
                return input::EventResult::Handled;
} else if (clickedBtn->action == ToolbarButton::Action::Comment) {
                addAnnot(core::interfaces::dom::AnnotationType::Text); // Stick note
                return input::EventResult::Handled;
            }
        }
        
        // If they click outside the toolbar while it's open, hide color palette
        if (m_colorPaletteOpen) {
            m_colorPaletteOpen = false;
            if (context.invalidateView) context.invalidateView();
        }
    }

    uint64_t nowMs = event.timestampMs > 0 ? event.timestampMs : static_cast<uint64_t>(::GetTickCount64());
    uint64_t maxDcTime = static_cast<uint64_t>(::GetDoubleClickTime());
    if (maxDcTime == 0) maxDcTime = kDefaultDoubleClickTimeMs;

    float dx = event.clientDip.x - m_lastClickDip.x;
    float dy = event.clientDip.y - m_lastClickDip.y;
    float distSq = dx * dx + dy * dy;

    if (nowMs >= m_lastClickTimeMs && (nowMs - m_lastClickTimeMs) <= maxDcTime &&
        distSq <= (kMultiClickDistanceThresholdDip * kMultiClickDistanceThresholdDip)) {
        m_clickCount = (m_clickCount % 4) + 1; // 1 -> 2 -> 3 -> 4 -> 1
    } else {
        m_clickCount = 1;
    }

    m_lastClickTimeMs = nowMs;
    m_lastClickDip = event.clientDip;

    int pageIndex = event.pageIndex;
    PointF pdfPt = event.pagePoint;

    // Quad-click reset
    if (m_clickCount == 4) {
        m_clickCount = 1; // Reset for next click
        m_selectionModel.ClearTextSelection();
        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }

    // If page index not set in event, try resolving via context
    if (pageIndex < 0 && context.canvasToPdf) {
        pdfPt = context.canvasToPdf(event.canvasPoint, pageIndex);
    }

    if (pageIndex < 0 || !context.getTextPage) {
        m_selectionModel.ClearTextSelection();
        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Ignored;
    }

    auto* textPage = context.getTextPage(pageIndex);
    if (!textPage) {
        m_selectionModel.ClearTextSelection();
        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Ignored;
    }

    int charIndex = textPage->GetCharIndexAtPos(pdfPt.x, pdfPt.y, 5.0, 5.0);
    if (charIndex < 0) {
        m_selectionModel.ClearTextSelection();
        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Ignored;
    }

    m_isSelecting = true;
    m_state = ToolState::Dragging;
    m_anchorPageIndex = pageIndex;
    m_anchorCharIndex = charIndex;

    if (m_clickCount == 1) {
        m_activeClickType = selection::TextClickType::Single;
        m_selectionModel.SelectCharacterAt(pageIndex, charIndex, textPage);
    } else if (m_clickCount == 2) {
        m_activeClickType = selection::TextClickType::Double;
        m_selectionModel.SelectWordAt(pageIndex, charIndex, textPage);
    } else if (m_clickCount == 3) {
        m_activeClickType = selection::TextClickType::Triple;
        m_selectionModel.SelectLineAt(pageIndex, charIndex, textPage);
    }

    if (context.captureService && context.hwnd) {
        context.captureService->AcquireCapture(context.hwnd, this);
    }
    if (context.invalidateView) context.invalidateView();

    return input::EventResult::Handled;
}

input::EventResult TextSelectTool::OnPointerMove(const input::PointerEvent& event, ToolContext& context) {
    if (!m_isSelecting) {
        if (m_state == ToolState::Idle && m_selectionModel.HasTextSelection() && context.canvasToDip) {
            PointF dipPt = context.canvasToDip(event.canvasPoint);
            bool needsDraw = false;
            auto checkHover = [&](std::vector<ToolbarButton>& buttons) {
                for (auto& btn : buttons) {
                    if (btn.isSeparator) continue;
                    bool hover = (dipPt.x >= btn.rect.left && dipPt.x <= btn.rect.right &&
                                  dipPt.y >= btn.rect.top && dipPt.y <= btn.rect.bottom);
                    if (hover != btn.isHovered) {
                        btn.isHovered = hover;
                        needsDraw = true;
                    }
                }
            };
            checkHover(m_toolbarButtons);
            if (m_colorPaletteOpen) checkHover(m_colorButtons);
            if (needsDraw && context.invalidateView) context.invalidateView();
        }
        
        m_state = ToolState::Hovering;
        return input::EventResult::Ignored;
    }

    int pageIndex = event.pageIndex;
    PointF pdfPt = event.pagePoint;

    if (pageIndex < 0 && context.canvasToPdf) {
        pdfPt = context.canvasToPdf(event.canvasPoint, pageIndex);
    }

    if (pageIndex < 0 || !context.getTextPage) {
        return input::EventResult::Handled;
    }

    auto* textPage = context.getTextPage(pageIndex);
    if (!textPage) {
        return input::EventResult::Handled;
    }

    int charIndex = textPage->GetCharIndexAtPos(pdfPt.x, pdfPt.y, 10.0, 10.0);
    if (charIndex >= 0) {
        m_selectionModel.ExpandSelectionTo(pageIndex, charIndex, textPage, m_activeClickType);
        if (context.invalidateView) context.invalidateView();
    }

    return input::EventResult::Handled;
}

input::EventResult TextSelectTool::OnPointerUp(const input::PointerEvent& event, ToolContext& context) {
    if (m_isSelecting) {
        if (context.captureService) {
            context.captureService->ReleaseCapture(this);
        }
        m_isSelecting = false;
        m_state = ToolState::Committed;
        m_state = ToolState::Idle;
        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }
    (void)event;
    return input::EventResult::Ignored;
}

input::EventResult TextSelectTool::OnPointerDoubleClick(const input::PointerEvent& event, ToolContext& context) {
    // Already handled by multi-click tracking in OnPointerDown
    (void)event;
    (void)context;
    return input::EventResult::Handled;
}

input::EventResult TextSelectTool::OnKeyDown(const input::KeyEvent& event, ToolContext& context) {
    if (event.virtualKey == VK_ESCAPE) {
        Cancel(context);
        m_selectionModel.ClearTextSelection();
        return input::EventResult::Handled;
    }

    // Ctrl+C Copy
    if (event.virtualKey == 'C' && input::HasModifier(event.modifiers, input::KeyModifier::Control)) {
        if (m_selectionModel.HasTextSelection()) {
            std::wstring selText = m_selectionModel.GetSelectedText();
            if (!selText.empty()) {
                core::Clipboard::SetText(context.hwnd, selText);
                return input::EventResult::Handled;
            }
        }
    }

    return input::EventResult::Ignored;
}

HCURSOR TextSelectTool::GetCursor(const PointF& point, ToolContext& context) const {
    if (m_state == ToolState::Idle && m_selectionModel.HasTextSelection() && context.canvasToDip) {
        PointF dipPt = context.canvasToDip(point);
        if (HasToolbarHit(dipPt)) {
            return ::LoadCursorW(NULL, IDC_HAND);
        }
    }
    return selection::CursorResolver::ResolveTextCursor();
}

bool TextSelectTool::HasToolbarHit(const PointF& dipPt) const {
    if (m_state != ToolState::Idle || !m_selectionModel.HasTextSelection()) return false;
    
    auto checkHit = [&](const std::vector<ToolbarButton>& buttons) {
        for (const auto& btn : buttons) {
            if (!btn.isSeparator && dipPt.x >= btn.rect.left && dipPt.x <= btn.rect.right &&
                dipPt.y >= btn.rect.top && dipPt.y <= btn.rect.bottom) {
                return true;
            }
        }
        return false;
    };
    
    if (checkHit(m_toolbarButtons)) return true;
    if (m_colorPaletteOpen && checkHit(m_colorButtons)) return true;
    
    return false;
}

void TextSelectTool::RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) {
    if (!renderTarget || !m_selectionModel.HasTextSelection()) {
        return;
    }

    const auto& sel = m_selectionModel.GetTextSelection();
    if (!sel.IsValid() || sel.rects.empty()) {
        return;
    }

    ID2D1SolidColorBrush* highlightBrush = nullptr;
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0078D7, 0.30f), &highlightBrush);
    if (!highlightBrush) return;

    float minX = 9999999.0f, minY = 9999999.0f;
    float maxX = -9999999.0f, maxY = -9999999.0f;

    for (const auto& r : sel.rects) {
        PointF p1 = { r.left, r.top };
        PointF p2 = { r.right, r.bottom };
        if (context.pdfToCanvas && sel.pageIndex >= 0) {
            p1 = context.pdfToCanvas(sel.pageIndex, p1);
            p2 = context.pdfToCanvas(sel.pageIndex, p2);
        }

        if (context.canvasToDip) {
            p1 = context.canvasToDip(p1);
            p2 = context.canvasToDip(p2);
        }

        float l = (std::min)(p1.x, p2.x);
        float t = (std::min)(p1.y, p2.y);
        float right = (std::max)(p1.x, p2.x);
        float b = (std::max)(p1.y, p2.y);

        D2D1_RECT_F d2dRect = D2D1::RectF(l, t, right, b);
        renderTarget->FillRectangle(d2dRect, highlightBrush);

        // Calculate bounds in DIPs for the toolbar
        minX = (std::min)(minX, l);
        minY = (std::min)(minY, t);
        maxX = (std::max)(maxX, right);
        maxY = (std::max)(maxY, b);
    }

    highlightBrush->Release();

    // Render Mini-Toolbar if idle
    if (m_state == ToolState::Idle && context.canvasToDip && minX < maxX) {
        UpdateToolbarLayout(minX, minY, maxX, maxY, context);
        DrawToolbar(renderTarget, context);
    }
}

void TextSelectTool::DrawToolbar(ID2D1RenderTarget* renderTarget, ToolContext& context) {
    (void)context;
    ID2D1SolidColorBrush* bgBrush = nullptr;
    ID2D1SolidColorBrush* sepBrush = nullptr;
    ID2D1SolidColorBrush* hoverBrush = nullptr;
    ID2D1SolidColorBrush* textBrush = nullptr;

    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.12f, 0.12f, 0.12f, 1.0f), &bgBrush);
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.3f, 0.3f, 0.3f, 1.0f), &sepBrush);
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.15f), &hoverBrush);
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.9f, 0.9f, 0.9f, 1.0f), &textBrush);

    if (!bgBrush || !sepBrush || !hoverBrush || !textBrush) return;

    // Background pill
    if (!m_toolbarButtons.empty()) {
        float left = m_toolbarButtons.front().rect.left - 4.0f;
        float right = m_toolbarButtons.back().rect.right + 4.0f;
        float top = m_toolbarButtons.front().rect.top - 4.0f;
        float bottom = m_toolbarButtons.front().rect.bottom + 4.0f;

        D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), 8.0f, 8.0f);
        renderTarget->FillRoundedRectangle(&roundedRect, bgBrush);
    }

    auto drawButtons = [&](std::vector<ToolbarButton>& buttons) {
        for (const auto& btn : buttons) {
            if (btn.isSeparator) {
                float mx = (btn.rect.left + btn.rect.right) * 0.5f;
                renderTarget->DrawLine(D2D1::Point2F(mx, btn.rect.top + 4.0f), D2D1::Point2F(mx, btn.rect.bottom - 4.0f), sepBrush);
                continue;
            }

            if (btn.isHovered || btn.isPressed) {
                D2D1_ROUNDED_RECT bgRect = D2D1::RoundedRect(D2D1::RectF(btn.rect.left, btn.rect.top, btn.rect.right, btn.rect.bottom), 4.0f, 4.0f);
                renderTarget->FillRoundedRectangle(&bgRect, hoverBrush);
            }

            if (btn.action == ToolbarButton::Action::ColorPick && btn.iconId == static_cast<int>(::controls::IconType::None)) {
                ID2D1SolidColorBrush* colorBrush = nullptr;
                if (SUCCEEDED(renderTarget->CreateSolidColorBrush(D2D1::ColorF(btn.r / 255.0f, btn.g / 255.0f, btn.b / 255.0f, btn.a / 255.0f), &colorBrush))) {
                    float cx = (btn.rect.left + btn.rect.right) * 0.5f;
                    float cy = (btn.rect.top + btn.rect.bottom) * 0.5f;
                    D2D1_ELLIPSE ell = D2D1::Ellipse(D2D1::Point2F(cx, cy), 10.0f, 10.0f);
                    renderTarget->FillEllipse(&ell, colorBrush);
                    if (btn.r == m_currentColor[0] && btn.g == m_currentColor[1] && btn.b == m_currentColor[2]) {
                        renderTarget->DrawEllipse(&ell, textBrush, 2.0f);
                    }
                    colorBrush->Release();
                }
            } else if (btn.iconId != static_cast<int>(::controls::IconType::None)) {
                D2D1_COLOR_F iconColor = D2D1::ColorF(btn.r / 255.0f, btn.g / 255.0f, btn.b / 255.0f, btn.a / 255.0f);
                if (btn.r == 0 && btn.g == 0 && btn.b == 0) {
                    iconColor = D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f);
                }
                D2D1_RECT_F r = D2D1::RectF(btn.rect.left + 4, btn.rect.top + 4, btn.rect.right - 4, btn.rect.bottom - 4);
                
                if (btn.action == ToolbarButton::Action::Highlight) {
                    D2D1_RECT_F iconR = D2D1::RectF(btn.rect.left + 2, btn.rect.top + 4, btn.rect.left + 2 + (btn.rect.bottom - btn.rect.top - 8), btn.rect.bottom - 4);
                    ::controls::IconRenderer::DrawIcon(renderTarget, static_cast<::controls::IconType>(btn.iconId), iconR, iconColor);
                    
                    // Draw tiny arrow
                    D2D1_RECT_F arrowR = D2D1::RectF(btn.rect.right - 14, btn.rect.top + 4, btn.rect.right - 2, btn.rect.bottom - 4);
                    ::controls::IconRenderer::DrawIcon(renderTarget, ::controls::IconType::ArrowDown, arrowR, iconColor);
                } else {
                    ::controls::IconRenderer::DrawIcon(renderTarget, static_cast<::controls::IconType>(btn.iconId), r, iconColor);
                }
            }
        }
    };

    drawButtons(m_toolbarButtons);
    
    if (m_colorPaletteOpen && !m_colorButtons.empty()) {
        float left = m_colorButtons.front().rect.left - 4.0f;
        float right = m_colorButtons.back().rect.right + 4.0f;
        float top = m_colorButtons.front().rect.top - 4.0f;
        float bottom = m_colorButtons.front().rect.bottom + 4.0f;

        D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), 8.0f, 8.0f);
        renderTarget->FillRoundedRectangle(&roundedRect, bgBrush);
        
        drawButtons(m_colorButtons);
    }

    if (bgBrush) bgBrush->Release();
    if (sepBrush) sepBrush->Release();
    if (hoverBrush) hoverBrush->Release();
    if (textBrush) textBrush->Release();
}

void TextSelectTool::CreateAnnotation(core::interfaces::dom::AnnotationType type, ToolContext& context) {
    auto sel = m_selectionModel.GetTextSelection();
    if (sel.pageIndex >= 0 && context.document && context.executeCommand) {
        float minX = 999999.0f, minY = 999999.0f;
        float maxX = -999999.0f, maxY = -999999.0f;
        std::vector<QuadF> quads;
        for (const auto& r : sel.rects) {
            float l = std::min(r.left, r.right);
            float right = std::max(r.left, r.right);
            float b = std::min(r.bottom, r.top);
            float t = std::max(r.bottom, r.top);

            minX = std::min(minX, l);
            minY = std::min(minY, b);
            maxX = std::max(maxX, right);
            maxY = std::max(maxY, t);

            quads.push_back({
                PointF{l, t},
                PointF{right, t},
                PointF{l, b},
                PointF{right, b}
            });
        }
        auto cmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
            context.document, sel.pageIndex, type, RectF{minX, minY, maxX, maxY}
        );
        cmd->SetQuads(quads);
        
        if (type == core::interfaces::dom::AnnotationType::Highlight ||
            type == core::interfaces::dom::AnnotationType::Underline ||
            type == core::interfaces::dom::AnnotationType::StrikeOut ) {
            cmd->SetColor(m_currentColor[0], m_currentColor[1], m_currentColor[2], m_currentColor[3]);
        }
        
        if (context.executeCommand(std::move(cmd))) {
            m_selectionModel.ClearTextSelection();
            if (context.invalidateView) context.invalidateView();
        }
    }
}

} // namespace ui::tools






