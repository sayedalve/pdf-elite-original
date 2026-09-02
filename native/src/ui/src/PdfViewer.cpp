#include "ui/dialogs/MessageDialog.h"
#include "PdfViewer.h"

#include <fstream>
#include <chrono>

static void LogPipeline(const std::string& msg) {
    std::ofstream out("pipeline.log", std::ios_base::app);
    out << "[" << std::chrono::system_clock::now().time_since_epoch().count() << "] " << msg << "\n";
}

#include <cmath>
#include <cstdio>
#include "../../pdf_engine/src/PdfDocument.h"
#include "../../core/models/RenderResult.h"
#include "../../core/CoordinateConverter.h"
#include "ThemeManager.h"
#include "../../core/RenderController.h"
#include "utils/Logger.h"
#include "NativeDesignSystem.h"
#include "interaction/TextSelectableObject.h"
#include "interaction/ImageSelectableObject.h"
#include "interaction/AnnotationSelectableObject.h"
#include "../../pdf_engine/src/commands/ImageCommands.h"
#include "../../pdf_engine/src/commands/TextCommands.h"
#include "../../pdf_engine/src/commands/AnnotationCommands.h"
#include "../../pdf_engine/src/commands/PageCommands.h"
#include "../../pdf_engine/src/commands/MacroCommand.h"
#include "../../pdf_engine/src/commands/ImageCommands.h"
#include "../../core/Clipboard.h"
#include "../../core/interfaces/dom/ITextPage.h"
#include "annotation/IAnnotationHandler.h"
#include "annotation/AnnotationHandlerFactory.h"
#include "menu/ContextMenuManager.h"
#include "CommandManager.h"
#include "search/SearchHighlightOverlay.h"
#include "components/AnnotationOverlay.h"
#include "tools/PanTool.h"
#include "tools/SelectTool.h"
#include "tools/TextSelectTool.h"
#include "tools/ShapeTool.h"
#include "tools/MarkupTool.h"
#include "tools/InkTool.h"
#include "tools/FreeTextTool.h"
#include "tools/StickyNoteTool.h"
#include "tools/StampTool.h"
#include "tools/EraserTool.h"
#include "tools/InsertImageTool.h"
#include <commdlg.h>
#include <fstream>

using namespace ui::interaction;
using namespace Microsoft::WRL;
using namespace core::interfaces::dom;


namespace {
    inline float snapPx(float val, float dpiScale) {
        return std::round(val * dpiScale) / dpiScale;
    }
}


PdfViewer::PdfViewer() {
    m_tileCache = std::make_unique<TileCache>();
    m_interactionManager.pageToView = [](double px, double py, int, double& vx, double& vy) { vx = px; vy = py; };
    m_interactionManager.viewToPage = [](double vx, double vy, double& px, double& py, int& p) { px = vx; py = vy; p = 0; };
    m_interactionManager.invalidateView = [this]() { InvalidateView(); };
    SetupInputRouting();
}

PdfViewer::~PdfViewer() {
    printf("~PdfViewer start\n"); fflush(stdout);
    // ...
    printf("~PdfViewer end\n"); fflush(stdout);
}

void PdfViewer::SetupInputRouting() {
    m_captureService = std::make_shared<ui::input::PointerCaptureService>();
    
    ui::tools::ToolContext ctx;
    ctx.document = m_doc.get();
    ctx.captureService = m_captureService.get();
    ctx.hwnd = m_hwnd;
    ctx.invalidateView = [this]() { InvalidateView(); };
    ctx.requestToolSwitch = [this](ui::tools::ToolType tool) {
        switch (tool) {
        case ui::tools::ToolType::Pan: SetToolMode(ToolMode::Pan); break;
        case ui::tools::ToolType::Select: SetToolMode(ToolMode::Select); break;
        case ui::tools::ToolType::Highlight: SetToolMode(ToolMode::Highlight); break;
        case ui::tools::ToolType::Underline: SetToolMode(ToolMode::Underline); break;
        case ui::tools::ToolType::Strikeout: SetToolMode(ToolMode::Strikeout); break;
        case ui::tools::ToolType::Rectangle: SetToolMode(ToolMode::Rectangle); break;
        case ui::tools::ToolType::Ellipse: SetToolMode(ToolMode::Ellipse); break;
        case ui::tools::ToolType::Line: SetToolMode(ToolMode::Line); break;
        case ui::tools::ToolType::Arrow: SetToolMode(ToolMode::Arrow); break;
        case ui::tools::ToolType::Ink: SetToolMode(ToolMode::Ink); break;
        case ui::tools::ToolType::FreeText: SetToolMode(ToolMode::FreeText); break;
        case ui::tools::ToolType::StickyNote: SetToolMode(ToolMode::StickyNote); break;
        case ui::tools::ToolType::AddText: SetToolMode(ToolMode::AddText); break;
        case ui::tools::ToolType::EditText: SetToolMode(ToolMode::EditText); break;
        case ui::tools::ToolType::Stamp: SetToolMode(ToolMode::Stamp); break;
        case ui::tools::ToolType::Eraser: SetToolMode(ToolMode::Eraser); break;
        case ui::tools::ToolType::InsertImage: SetToolMode(ToolMode::InsertImage); break;
        default: break;
        }
    };
    ctx.scrollViewport = [this](float dx, float dy) {
        if (dx != 0.0f) OnScrollX(dx);
        if (dy != 0.0f) OnScroll(dy);
    };
    ctx.zoomViewport = [this](double factor, double mouseX, double mouseY) {
        OnZoom(factor, mouseX, mouseY);
    };
    ctx.getZoom = [this]() {
        return m_zoom;
    };
    ctx.dipToCanvas = [this](const PointF& dipPt) -> PointF {
        return PointF{ dipPt.x - m_bounds.left + static_cast<float>(m_scrollX), dipPt.y - m_bounds.top + m_scrollY };
    };
    ctx.canvasToDip = [this](const PointF& canvasPt) -> PointF {
        return PointF{ canvasPt.x - static_cast<float>(m_scrollX) + m_bounds.left, canvasPt.y - m_scrollY + m_bounds.top };
    };
    ctx.canvasToPdf = [this](const PointF& canvasPt, int& outPageIndex) -> PointF {
        outPageIndex = -1;
        float width = m_bounds.right - m_bounds.left;
        float screenX = canvasPt.x + m_bounds.left - static_cast<float>(m_scrollX);
        float screenY = canvasPt.y + m_bounds.top - m_scrollY;
        
        for (const auto& page : m_layout) {
            float pageX = m_bounds.left - m_scrollX + std::max(0.0f, (width - page.width) / 2.0f);
            float pageY = m_bounds.top + page.yOffset - m_scrollY;

            if (screenX >= pageX && screenX <= pageX + page.width && screenY >= pageY && screenY <= pageY + page.height) {
                outPageIndex = page.index;
                float unscaledW = (page.index < static_cast<int>(m_cachedPageSizes.size()))
                    ? m_cachedPageSizes[page.index].first
                    : (page.width / static_cast<float>(m_zoom));
                float unscaledH = (page.index < static_cast<int>(m_cachedPageSizes.size()))
                    ? m_cachedPageSizes[page.index].second
                    : (page.height / static_cast<float>(m_zoom));

                int rot = 0;
                if (m_doc) {
                    auto docPage = m_doc->GetPage(page.index);
                    if (docPage) rot = docPage->GetRotation();
                }

                CoordinateConverter::PageContext pageCtx{ unscaledW, unscaledH, rot };
                CoordinateConverter::ViewContext viewCtx{
                    m_zoom,
                    static_cast<double>(m_scrollX),
                    static_cast<double>(m_scrollY),
                    pageX + m_scrollX,
                    pageY + m_scrollY
                };

                return CoordinateConverter::ScreenToPdf(pageCtx, viewCtx, screenX, screenY);
            }
        }
        return PointF{ 0.0f, 0.0f };
    };
    ctx.pdfToCanvas = [this](int pageIndex, const PointF& pdfPt) -> PointF {
        float width = m_bounds.right - m_bounds.left;
        for (const auto& page : m_layout) {
            if (page.index == pageIndex) {
                float pageX = m_bounds.left - m_scrollX + std::max(0.0f, (width - page.width) / 2.0f);
                float pageY = m_bounds.top + page.yOffset - m_scrollY;
                
                float unscaledW = (page.index < static_cast<int>(m_cachedPageSizes.size()))
                    ? m_cachedPageSizes[page.index].first
                    : (page.width / static_cast<float>(m_zoom));
                float unscaledH = (page.index < static_cast<int>(m_cachedPageSizes.size()))
                    ? m_cachedPageSizes[page.index].second
                    : (page.height / static_cast<float>(m_zoom));

                int rot = 0;
                if (m_doc) {
                    auto docPage = m_doc->GetPage(page.index);
                    if (docPage) rot = docPage->GetRotation();
                }

                CoordinateConverter::PageContext pageCtx{ unscaledW, unscaledH, rot };
                CoordinateConverter::ViewContext viewCtx{
                    m_zoom,
                    static_cast<double>(m_scrollX),
                    static_cast<double>(m_scrollY),
                    pageX + m_scrollX,
                    pageY + m_scrollY
                };

                PointF screenPt = CoordinateConverter::PdfToScreen(pageCtx, viewCtx, pdfPt.x, pdfPt.y);
                float canvasX = screenPt.x - m_bounds.left + static_cast<float>(m_scrollX);
                float canvasY = screenPt.y - m_bounds.top + m_scrollY;
                return PointF{ canvasX, canvasY };
            }
        }
        return PointF{ 0.0f, 0.0f };
    };
    ctx.getTextPage = [this](int pageIndex) -> core::interfaces::dom::ITextPage* {
        return GetTextPage(pageIndex);
    };
    ctx.hitTestObjects = [this](const PointF& canvasPt) -> std::vector<ui::selection::SelectedObject> {
        std::vector<ui::selection::SelectedObject> results;
        if (!m_doc) return results;

        float width = m_bounds.right - m_bounds.left;
        for (const auto& layout : m_layout) {
            float centerOffset = std::max(0.0f, (width - layout.width) / 2.0f);
            if (canvasPt.y < layout.yOffset || canvasPt.y > layout.yOffset + layout.height ||
                canvasPt.x < centerOffset || canvasPt.x > centerOffset + layout.width) {
                continue;
            }

            auto it = m_activePages.find(layout.index);
            auto page = (it != m_activePages.end()) ? it->second : nullptr;
            if (!page) continue;

            float pageX = m_bounds.left - m_scrollX + centerOffset;
            float pageY = m_bounds.top + layout.yOffset - m_scrollY;
            float unscaledW = (layout.index < static_cast<int>(m_cachedPageSizes.size())) ? m_cachedPageSizes[layout.index].first : (layout.width / static_cast<float>(m_zoom));
            float unscaledH = (layout.index < static_cast<int>(m_cachedPageSizes.size())) ? m_cachedPageSizes[layout.index].second : (layout.height / static_cast<float>(m_zoom));
            int rot = (m_doc && m_doc->GetPage(layout.index)) ? m_doc->GetPage(layout.index)->GetRotation() : 0;
            
            CoordinateConverter::PageContext pageCtx{ unscaledW, unscaledH, rot };
            CoordinateConverter::ViewContext viewCtx{ m_zoom, static_cast<double>(m_scrollX), static_cast<double>(m_scrollY), pageX + m_scrollX, pageY + m_scrollY };

            float screenX = canvasPt.x + m_bounds.left - static_cast<float>(m_scrollX);
            float screenY = canvasPt.y + m_bounds.top - m_scrollY;

            // 1. Annotations
            auto annots = page->GetAnnotations();
            for (const auto& a : annots) {
                if (!a) continue;
                auto rect = a->GetBounds();
                RectF screenRect = CoordinateConverter::PdfToScreenRect(pageCtx, viewCtx, rect.left, rect.top, rect.right, rect.bottom);
                
                if (screenX >= screenRect.left && screenX <= screenRect.right &&
                    screenY >= screenRect.top && screenY <= screenRect.bottom) {
                    
                    RectF canvasRect = {
                        screenRect.left - m_bounds.left + static_cast<float>(m_scrollX),
                        screenRect.top - m_bounds.top + static_cast<float>(m_scrollY),
                        screenRect.right - m_bounds.left + static_cast<float>(m_scrollX),
                        screenRect.bottom - m_bounds.top + static_cast<float>(m_scrollY)
                    };

                    ui::selection::SelectedObject obj;
                    obj.id = a->GetId();
                    obj.pageIndex = layout.index;
                    obj.pageBounds = canvasRect;
                    obj.rotationDegrees = 0.0f;
                    
                    auto t = a->GetType();
                    if (t == core::interfaces::dom::AnnotationType::Highlight ||
                        t == core::interfaces::dom::AnnotationType::Underline ||
                        t == core::interfaces::dom::AnnotationType::StrikeOut ||
                        t == core::interfaces::dom::AnnotationType::Squiggly) {
                        obj.isTextMarkup = true;
                    obj.userData = a;
                    }
                    
                    results.push_back(obj);
                }
            }

            // 2. Images
            auto images = page->GetImages();
            for (const auto& img : images) {
                if (!img) continue;
                auto rect = img->GetBounds();
                RectF screenRect = CoordinateConverter::PdfToScreenRect(pageCtx, viewCtx, rect.left, rect.top, rect.right, rect.bottom);

                if (screenX >= screenRect.left && screenX <= screenRect.right &&
                    screenY >= screenRect.top && screenY <= screenRect.bottom) {
                    
                    RectF canvasRect = {
                        screenRect.left - m_bounds.left + static_cast<float>(m_scrollX),
                        screenRect.top - m_bounds.top + static_cast<float>(m_scrollY),
                        screenRect.right - m_bounds.left + static_cast<float>(m_scrollX),
                        screenRect.bottom - m_bounds.top + static_cast<float>(m_scrollY)
                    };

                    ui::selection::SelectedObject obj;
                    obj.id = img->GetId();
                    obj.pageIndex = layout.index;
                    obj.pageBounds = canvasRect;
                    obj.rotationDegrees = 0.0f;
                    results.push_back(obj);
                }
            }

            // 3. Text objects (ONLY in explicit Edit Text mode)
            if (m_currentTool == ToolMode::EditText || m_currentTool == ToolMode::AddText) {
                auto texts = page->GetTextObjects();
                for (const auto& txt : texts) {
                    if (!txt) continue;
                    auto rect = txt->GetBounds();
                    RectF screenRect = CoordinateConverter::PdfToScreenRect(pageCtx, viewCtx, rect.left, rect.top, rect.right, rect.bottom);

                    if (screenX >= screenRect.left && screenX <= screenRect.right &&
                        screenY >= screenRect.top && screenY <= screenRect.bottom) {
                        
                        RectF canvasRect = {
                            screenRect.left - m_bounds.left + static_cast<float>(m_scrollX),
                            screenRect.top - m_bounds.top + static_cast<float>(m_scrollY),
                            screenRect.right - m_bounds.left + static_cast<float>(m_scrollX),
                            screenRect.bottom - m_bounds.top + static_cast<float>(m_scrollY)
                        };

                        ui::selection::SelectedObject obj;
                        obj.id = std::to_string(txt->GetId());
                        obj.pageIndex = layout.index;
                        obj.pageBounds = canvasRect;
                        obj.rotationDegrees = 0.0f;
                        results.push_back(obj);
                    }
                }
            }
        }
        return results;
    };
    ctx.executeCommand = [this](std::unique_ptr<core::interfaces::dom::ICommand> cmd) -> bool {
        if (m_doc && cmd) {
            bool ok = m_doc->GetCommandStack().ExecuteCommand(std::move(cmd));
            if (ok) {
                // DO NOT clear active pages so we don't lose dynamically created annotations in PDFium!
                m_generation++;
                core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                UpdateVisibleTiles();
                ReloadInteractableObjects();
                InvalidateView();
            }
            return ok;
        }
        return false;
    };
    ctx.enterAnnotationEditMode = [this](std::shared_ptr<core::interfaces::dom::IAnnotation> annot) {
        if (!annot || !m_doc) return;
        
        // Find the wrapper in InteractionManager objects and call EnterAnnotationEditMode
        for (const auto& obj : m_interactionManager.GetObjects()) {
            if (auto annotObj = std::dynamic_pointer_cast<ui::interaction::AnnotationSelectableObject>(obj)) {
                if (annotObj->GetAnnotation()->GetId() == annot->GetId()) {
                    m_interactionManager.EnterAnnotationEditMode(annotObj);
                    break;
                }
            }
        }
    };
    ctx.openAnnotationPopup = [this](const std::string& annotId) {
        if (!m_doc) return;
        for (const auto& obj : m_interactionManager.GetObjects()) {
            if (auto annotObj = std::dynamic_pointer_cast<ui::interaction::AnnotationSelectableObject>(obj)) {
                if (annotObj->GetAnnotation()->GetId() == annotId) {
                    m_interactionManager.EnterAnnotationEditMode(annotObj);
                    break;
                }
            }
        }
    };

    m_toolStateMachine = std::make_shared<ui::tools::ToolStateMachine>(ctx);
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::PanTool>());
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::SelectTool>());
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::TextSelectTool>());
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::ShapeTool>(ui::tools::ToolType::Rectangle));
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::ShapeTool>(ui::tools::ToolType::Ellipse));
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::ShapeTool>(ui::tools::ToolType::Line));
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::ShapeTool>(ui::tools::ToolType::Arrow));
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::MarkupTool>(ui::tools::ToolType::Highlight));
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::ShapeTool>(ui::tools::ToolType::AreaHighlight));
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::MarkupTool>(ui::tools::ToolType::Underline));
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::MarkupTool>(ui::tools::ToolType::Strikeout));
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::InkTool>());
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::FreeTextTool>(ui::tools::ToolType::FreeText));
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::FreeTextTool>(ui::tools::ToolType::AddText));
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::FreeTextTool>(ui::tools::ToolType::EditText));
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::StickyNoteTool>());
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::StampTool>());
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::EraserTool>());
    m_toolStateMachine->RegisterTool(std::make_unique<ui::tools::InsertImageTool>());

    m_inputRouter = std::make_unique<ui::input::InputRouter>(m_captureService, m_toolStateMachine);

    // Initialize default tool
    m_toolStateMachine->SetActiveTool(ui::tools::ToolType::Select);

    if (m_hwnd) {
        m_inputRouter->SetHwnd(m_hwnd);
    }
}

ui::input::PointerEvent PdfViewer::CreatePointerEvent(
    ui::input::PointerEventType type, float x, float y, ui::input::PointerButton btn) const {
    ui::input::PointerEvent pe;
    pe.type = type;
    pe.button = btn;

    // Buttons Down
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) pe.buttonsDown |= ui::input::PointerButton::Left;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) pe.buttonsDown |= ui::input::PointerButton::Right;
    if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) pe.buttonsDown |= ui::input::PointerButton::Middle;
    if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) pe.buttonsDown |= ui::input::PointerButton::XButton1;
    if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) pe.buttonsDown |= ui::input::PointerButton::XButton2;

    if (type == ui::input::PointerEventType::Down && btn != ui::input::PointerButton::None) {
        pe.buttonsDown |= btn;
    } else if (type == ui::input::PointerEventType::Up && btn != ui::input::PointerButton::None) {
        pe.buttonsDown &= ~btn;
    }

    // Key Modifiers
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) pe.modifiers |= ui::input::KeyModifier::Shift;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) pe.modifiers |= ui::input::KeyModifier::Control;
    if (GetAsyncKeyState(VK_MENU) & 0x8000) pe.modifiers |= ui::input::KeyModifier::Alt;
    if ((GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000)) {
        pe.modifiers |= ui::input::KeyModifier::Windows;
    }

    // 1. Client DIPs
    pe.clientDip = PointF{ x, y };

    // 2. Physical Screen Pixels
    float dpiScale = m_hwnd ? (GetDpiForWindow(m_hwnd) / 96.0f) : 1.0f;
    if (dpiScale <= 0.0f) dpiScale = 1.0f;
    pe.physicalScreen = PointF{ x * dpiScale, y * dpiScale };

    // 3. Continuous Canvas space
    float canvasX = x - m_bounds.left + static_cast<float>(m_scrollX);
    float canvasY = y - m_bounds.top + m_scrollY;
    pe.canvasPoint = PointF{ canvasX, canvasY };

    // 4. PDF Page Point & Target Page Index
    pe.pageIndex = -1;
    pe.pagePoint = PointF{ 0.0f, 0.0f };
    float width = m_bounds.right - m_bounds.left;
    for (const auto& page : m_layout) {
        float pageX = m_bounds.left - m_scrollX + std::max(0.0f, (width - page.width) / 2.0f);
        float pageY = m_bounds.top + page.yOffset - m_scrollY;

        if (x >= pageX && x <= pageX + page.width && y >= pageY && y <= pageY + page.height) {
            pe.pageIndex = page.index;
            float unscaledW = (page.index < static_cast<int>(m_cachedPageSizes.size()))
                ? m_cachedPageSizes[page.index].first
                : (page.width / static_cast<float>(m_zoom));
            float unscaledH = (page.index < static_cast<int>(m_cachedPageSizes.size()))
                ? m_cachedPageSizes[page.index].second
                : (page.height / static_cast<float>(m_zoom));

            int rot = 0;
            if (m_doc) {
                auto docPage = m_doc->GetPage(page.index);
                if (docPage) rot = docPage->GetRotation();
            }

            CoordinateConverter::PageContext pageCtx{ unscaledW, unscaledH, rot };
            CoordinateConverter::ViewContext viewCtx{
                m_zoom,
                static_cast<double>(m_scrollX),
                static_cast<double>(m_scrollY),
                pageX + m_scrollX,
                pageY + m_scrollY
            };

            pe.pagePoint = CoordinateConverter::ScreenToPdf(pageCtx, viewCtx, x, y);
            break;
        }
    }

    pe.timestampMs = ::GetTickCount64();
    pe.pressure = 1.0f;
    return pe;
}

bool PdfViewer::Initialize(HWND parentHwnd) {
    printf("[TRACE_PV 1: Initialize start]\n"); fflush(stdout);
    m_hwnd = parentHwnd;
    if (m_inputRouter) {
        m_inputRouter->SetHwnd(parentHwnd);
    }
    if (m_toolStateMachine) {
        auto& c = m_toolStateMachine->GetContext();
        c.hwnd = parentHwnd;
    }
    
    m_interactionManager.viewToPage = [this](double vx, double vy, double& px, double& py, int& pageIndex) {
        float width = m_bounds.right - m_bounds.left;
        float screenX = static_cast<float>(vx);
        float screenY = static_cast<float>(vy);

        for (const auto& page : m_layout) {
            float pageX = m_bounds.left - m_scrollX + std::max(0.0f, (width - page.width) / 2.0f);
            float pageY = m_bounds.top + page.yOffset - m_scrollY;
            
            if (screenX >= pageX && screenX <= pageX + page.width && screenY >= pageY && screenY <= pageY + page.height) {
                pageIndex = page.index;
                
                float unscaledW = (page.index < static_cast<int>(m_cachedPageSizes.size())) ? m_cachedPageSizes[page.index].first : (page.width / static_cast<float>(m_zoom));
                float unscaledH = (page.index < static_cast<int>(m_cachedPageSizes.size())) ? m_cachedPageSizes[page.index].second : (page.height / static_cast<float>(m_zoom));
                
                int rot = 0;
                if (m_doc) {
                    auto docPage = m_doc->GetPage(page.index);
                    if (docPage) rot = docPage->GetRotation();
                }
                
                CoordinateConverter::PageContext pageCtx{ unscaledW, unscaledH, rot };
                CoordinateConverter::ViewContext viewCtx{ m_zoom, static_cast<double>(m_scrollX), static_cast<double>(m_scrollY), pageX + m_scrollX, pageY + m_scrollY };
                
                PointF pdfPt = CoordinateConverter::ScreenToPdf(pageCtx, viewCtx, screenX, screenY);
                px = pdfPt.x;
                py = pdfPt.y;
                return;
            }
        }
        pageIndex = -1;
    };
    
    m_interactionManager.pageToView = [this](double px, double py, int pageIndex, double& vx, double& vy) {
        float width = m_bounds.right - m_bounds.left;
        for (const auto& page : m_layout) {
            if (page.index == pageIndex) {
                float pageX = m_bounds.left - m_scrollX + std::max(0.0f, (width - page.width) / 2.0f);
                float pageY = m_bounds.top + page.yOffset - m_scrollY;
                
                float unscaledW = (page.index < static_cast<int>(m_cachedPageSizes.size())) ? m_cachedPageSizes[page.index].first : (page.width / static_cast<float>(m_zoom));
                float unscaledH = (page.index < static_cast<int>(m_cachedPageSizes.size())) ? m_cachedPageSizes[page.index].second : (page.height / static_cast<float>(m_zoom));
                
                int rot = 0;
                if (m_doc) {
                    auto docPage = m_doc->GetPage(page.index);
                    if (docPage) rot = docPage->GetRotation();
                }
                
                CoordinateConverter::PageContext pageCtx{ unscaledW, unscaledH, rot };
                CoordinateConverter::ViewContext viewCtx{ m_zoom, static_cast<double>(m_scrollX), static_cast<double>(m_scrollY), pageX + m_scrollX, pageY + m_scrollY };
                
                PointF screenPt = CoordinateConverter::PdfToScreen(pageCtx, viewCtx, px, py);
                vx = screenPt.x;
                vy = screenPt.y;
                return;
            }
        }
    };
    
    m_interactionManager.invalidateView = [this]() {
        InvalidateView();
    };
    
    m_interactionManager.onObjectCommitted = [this](std::shared_ptr<ui::interaction::ISelectableObject> obj, const Rect& oldB, const Rect& newB, bool /*hasOldLineGeom*/, const core::interfaces::dom::LineGeometry& /*oldLineGeom*/) {
        if (auto textObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(obj)) {
            m_interactionManager.onCommandRequested(std::make_unique<pdf_engine::commands::MoveTextCommand>(
                textObj->GetTextObject(), RectF{(float)oldB.left, (float)oldB.top, (float)oldB.right, (float)oldB.bottom},
                RectF{(float)newB.left, (float)newB.top, (float)newB.right, (float)newB.bottom}
            ));
        }
        else if (auto imgObj = std::dynamic_pointer_cast<ui::interaction::ImageSelectableObject>(obj)) {
            m_interactionManager.onCommandRequested(std::make_unique<pdf_engine::commands::MoveImageCommand>(
                imgObj->GetImage(), RectF{(float)oldB.left, (float)oldB.top, (float)oldB.right, (float)oldB.bottom},
                RectF{(float)newB.left, (float)newB.top, (float)newB.right, (float)newB.bottom}
            ));
        }
    };
    
    m_interactionManager.onColorChangedRequested = [this](std::shared_ptr<ui::interaction::AnnotationSelectableObject> obj, int r, int g, int b, int a) {
        if (!m_doc || !obj) return;
        auto annot = obj->GetAnnotation();
        if (!annot) return;
        
        pdf_engine::commands::AnnotationState oldState;
        oldState.type = annot->GetType();
        oldState.bounds = annot->GetBounds();
        oldState.contents = annot->GetContents();
        oldState.quads = annot->GetQuadPoints();
        oldState.hasColor = annot->GetColor(oldState.colorR, oldState.colorG, oldState.colorB, oldState.colorA);
        oldState.hasFillColor = annot->GetFillColor(oldState.fillColorR, oldState.fillColorG, oldState.fillColorB, oldState.fillColorA);
        oldState.borderWidth = annot->GetBorderWidth();
        oldState.opacity = annot->GetOpacity();
        oldState.hasLineGeom = annot->GetLineGeometry(oldState.lineGeom);
        oldState.inkList = annot->GetInkList();
        
        pdf_engine::commands::AnnotationState newState = oldState;
        newState.hasColor = true;
        newState.colorR = r; newState.colorG = g; newState.colorB = b; newState.colorA = a;
        
        // Use fill color for shapes with fill color
        if (annot->GetType() == core::interfaces::dom::AnnotationType::Square || 
            annot->GetType() == core::interfaces::dom::AnnotationType::Circle) {
            newState.hasFillColor = true;
            newState.fillColorR = r; newState.fillColorG = g; newState.fillColorB = b; newState.fillColorA = a / 4;
        }

        m_doc->GetCommandStack().ExecuteCommand(std::make_unique<pdf_engine::commands::ModifyAnnotationPropertiesCommand>(annot, oldState, newState));
        
        m_activePages.clear();
        m_textPages.clear();
        m_generation++;
        core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
        UpdateVisibleTiles();
        ReloadInteractableObjects();
        InvalidateView();
    };
    
    m_interactionManager.onDeleteRequested = [this](const std::vector<std::shared_ptr<ui::interaction::ISelectableObject>>& objects) {
        if (objects.empty() || !m_doc) return;

        if (objects.size() == 1) {
            auto obj = objects[0];
            if (auto textObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(obj)) {
                if (auto t = textObj->GetTextObject()) {
                    auto cmd = std::make_unique<pdf_engine::commands::DeleteTextCommand>(
                        m_doc.get(), textObj->GetPageIndex(), t
                    );
                    m_doc->GetCommandStack().ExecuteCommand(std::move(cmd));
                    m_interactionManager.RemoveObject(textObj->GetId());
                    m_generation++;
                    core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                    ReloadInteractableObjects();
                    InvalidateView();
                }
            } else if (auto imgObj = std::dynamic_pointer_cast<ui::interaction::ImageSelectableObject>(obj)) {
                auto actualPdfImage = imgObj->GetImage();
                if (actualPdfImage) {
                    auto cmd = std::make_unique<pdf_engine::commands::DeleteImageCommand>(
                        m_doc.get(), imgObj->GetPageIndex(), actualPdfImage
                    );
                    m_doc->GetCommandStack().ExecuteCommand(std::move(cmd));
                    m_interactionManager.RemoveObject(imgObj->GetId());
                    m_generation++;
                    core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                    ReloadInteractableObjects();
                    InvalidateView();
                }
            } else if (auto annotObj = std::dynamic_pointer_cast<ui::interaction::AnnotationSelectableObject>(obj)) {
                if (auto a = annotObj->GetAnnotation()) {
                    auto cmd = std::make_unique<pdf_engine::commands::DeleteAnnotationCommand>(
                        m_doc.get(), annotObj->GetPageIndex(), a
                    );
                    m_doc->GetCommandStack().ExecuteCommand(std::move(cmd));
                    m_interactionManager.RemoveObject(annotObj->GetId());
                    m_generation++;
                    core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                    ReloadInteractableObjects();
                    InvalidateView();
                }
            }
        } else {
            auto macro = std::make_unique<pdf_engine::commands::MacroCommand>("Delete Objects");
            std::vector<std::string> idsToRemove;
            for (auto& obj : objects) {
                if (auto textObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(obj)) {
                    if (auto t = textObj->GetTextObject()) {
                        macro->AddCommand(std::make_unique<pdf_engine::commands::DeleteTextCommand>(
                            m_doc.get(), textObj->GetPageIndex(), t
                        ));
                        idsToRemove.push_back(textObj->GetId());
                    }
                } else if (auto imgObj = std::dynamic_pointer_cast<ui::interaction::ImageSelectableObject>(obj)) {
                    auto actualPdfImage = imgObj->GetImage();
                    if (actualPdfImage) {
                        macro->AddCommand(std::make_unique<pdf_engine::commands::DeleteImageCommand>(
                            m_doc.get(), imgObj->GetPageIndex(), actualPdfImage
                        ));
                        idsToRemove.push_back(imgObj->GetId());
                    }
                } else if (auto annotObj = std::dynamic_pointer_cast<ui::interaction::AnnotationSelectableObject>(obj)) {
                    if (auto a = annotObj->GetAnnotation()) {
                        macro->AddCommand(std::make_unique<pdf_engine::commands::DeleteAnnotationCommand>(
                            m_doc.get(), annotObj->GetPageIndex(), a
                        ));
                        idsToRemove.push_back(annotObj->GetId());
                    }
                }
            }
            if (!idsToRemove.empty()) {
                m_doc->GetCommandStack().ExecuteCommand(std::move(macro));
                for (const auto& id : idsToRemove) {
                    m_interactionManager.RemoveObject(id);
                }
                m_generation++;
                core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                ReloadInteractableObjects();
                InvalidateView();
            }
        }
        m_interactionManager.GetSelectionModel().Clear();
    };
    
    m_interactionManager.onCommandRequested = [this](std::unique_ptr<core::interfaces::dom::ICommand> cmd) {
        if (m_doc) {
            auto addTextCmd = dynamic_cast<pdf_engine::commands::AddTextCommand*>(cmd.get());
            if (addTextCmd) addTextCmd->SetDocument(m_doc.get());
            
            bool ok = m_doc->GetCommandStack().ExecuteCommand(std::move(cmd));
            if (ok) {
                m_generation++;
                core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                ReloadInteractableObjects();
                InvalidateView();
            }
        }
    };
    
    InitializeAnnotationHandlers();
    return true;
}

void PdfViewer::InitializeAnnotationHandlers() {
    ui::annotation::AnnotationHandlerContext ctx;
    ctx.hwnd = m_hwnd;
    ctx.viewToPage = [this](double vx, double vy, double& px, double& py, int& pageIndex) {
        m_interactionManager.viewToPage(vx, vy, px, py, pageIndex);
    };
    ctx.pageToView = [this](double px, double py, int pageIndex, double& vx, double& vy) {
        m_interactionManager.pageToView(px, py, pageIndex, vx, vy);
    };
    ctx.invalidateView = [this]() {
        InvalidateView();
    };
    ctx.executeCommand = [this, &ctx](std::unique_ptr<core::interfaces::dom::ICommand> cmd) -> bool {
        if (!m_doc) return false;
        bool ok = m_doc->GetCommandStack().ExecuteCommand(std::move(cmd));
        if (ok && ctx.onMutationCommitted) {
            ctx.onMutationCommitted();
        }
        return ok;
    };
    ctx.getDocument = [this]() -> core::interfaces::dom::IDocument* {
        return m_doc.get();
    };
    ctx.getTextPage = [this](int pageIndex) -> core::interfaces::dom::ITextPage* {
        return GetTextPage(pageIndex);
    };
    ctx.setToolMode = [this](ToolMode mode) {
        SetToolMode(mode);
    };
    ctx.reloadInteractables = [this]() {
        ReloadInteractableObjects();
    };
    ctx.onMutationCommitted = [this]() {
        m_activePages.clear();
        m_textPages.clear();
        m_generation++;
        core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
        UpdateVisibleTiles();
        ReloadInteractableObjects();
        InvalidateView();
    };

    m_handlers = ui::annotation::AnnotationHandlerFactory::CreateAllHandlers(ctx);
    auto it = m_handlers.find(m_currentTool);
    m_activeHandler = (it != m_handlers.end()) ? it->second : nullptr;
}

void PdfViewer::CachePageSizes() {
    LogPipeline("PdfViewer::CachePageSizes start");
    int count = m_doc->PageCount();
    LogPipeline("PageCount returned: " + std::to_string(count));
    
    // If sizes were pre-cached on the background thread, skip the expensive loop
    if (static_cast<int>(m_cachedPageSizes.size()) == count && count > 0) {
        if (m_cachedPageSizes[0].first != 0.0f) {
            return;
        }
    }
    
    m_cachedPageSizes.resize(count);
    if (count > 0) {
        for (int i = 0; i < count; ++i) {
            auto size = m_doc->GetPageSize(i);
            m_cachedPageSizes[i] = {static_cast<float>(size.width), static_cast<float>(size.height)};
        }
    }
    LogPipeline("PdfViewer::CachePageSizes finish");
}

void PdfViewer::SetDocument(std::shared_ptr<IDocument> doc) {
    printf("[TRACE_PV 2: SetDocument start]\n"); fflush(stdout);
    m_doc = doc;
    CachePageSizes();
    
    if (m_toolStateMachine) {
        auto& ctx = m_toolStateMachine->GetContext();
        ctx.document = m_doc.get();
    }
    
    if (m_doc) {
        int count = m_doc->PageCount();
        printf("[TRACE_PV 3: PageCount = %d]\n", count); fflush(stdout);
        
        m_fitWidthMode = true; // freshly opened document starts fit-to-width
        m_scrollX = 0;
        m_scrollY = 0;
        if (m_bounds.right > m_bounds.left && m_bounds.bottom > m_bounds.top) {
            ZoomToFitWidth();
        } else {
            m_zoom = 1.0; m_layoutDirty = true;
            m_maxPageWidth = m_cachedPageSizes.empty() ? 0.0f : m_cachedPageSizes[0].first;
        }
    } else {
        m_activePages.clear();
        m_layout.clear();
    }
    printf("[TRACE_PV 4: calling UpdateVisibleTiles]\n"); fflush(stdout);
    UpdateVisibleTiles();
    printf("[TRACE_PV 5: SetDocument done]\n"); fflush(stdout);
}




void PdfViewer::SetToolMode(ToolMode mode) {
    if (m_currentTool != mode) {
        CancelActiveInteractions();
        m_currentTool = mode;

        ui::tools::ToolType tt = ui::tools::ToolType::None;
        switch (mode) {
        case ToolMode::Pan: tt = ui::tools::ToolType::Pan; break;
        case ToolMode::Select: tt = ui::tools::ToolType::Select; break;
        case ToolMode::Highlight: tt = ui::tools::ToolType::Highlight; break;
        case ToolMode::Underline: tt = ui::tools::ToolType::Underline; break;
        case ToolMode::Strikeout: tt = ui::tools::ToolType::Strikeout; break;
        //case ToolMode::Squiggly: tt = ui::tools::ToolType::Squiggly; break;
        //case ToolMode::Caret: tt = ui::tools::ToolType::Caret; break;
        case ToolMode::AreaHighlight: tt = ui::tools::ToolType::AreaHighlight; break;
        case ToolMode::Rectangle: tt = ui::tools::ToolType::Rectangle; break;
        case ToolMode::Ellipse: tt = ui::tools::ToolType::Ellipse; break;
        case ToolMode::Line: tt = ui::tools::ToolType::Line; break;
        case ToolMode::Arrow: tt = ui::tools::ToolType::Arrow; break;
        case ToolMode::Ink: tt = ui::tools::ToolType::Ink; break;
        case ToolMode::FreeText: tt = ui::tools::ToolType::FreeText; break;
        case ToolMode::TypeWriter: tt = ui::tools::ToolType::TypeWriter; break;
        case ToolMode::TextBox: tt = ui::tools::ToolType::TextBox; break;
        case ToolMode::TextCallout: tt = ui::tools::ToolType::TextCallout; break;
        case ToolMode::StickyNote: tt = ui::tools::ToolType::StickyNote; break;
        case ToolMode::AddText: tt = ui::tools::ToolType::AddText; break;
        case ToolMode::EditText: tt = ui::tools::ToolType::EditText; break;
        case ToolMode::Stamp: tt = ui::tools::ToolType::Stamp; break;
        case ToolMode::Eraser: tt = ui::tools::ToolType::Eraser; break;
        case ToolMode::InsertImage: tt = ui::tools::ToolType::InsertImage; break;
        default: tt = ui::tools::ToolType::None; break;
        }

        if (m_toolStateMachine) {
            m_toolStateMachine->SetActiveTool(tt);
        }

        auto it = m_handlers.find(mode);
        m_activeHandler = (it != m_handlers.end()) ? it->second : nullptr;
        ReloadInteractableObjects();
        InvalidateView();
    }
}

InteractionManager& PdfViewer::GetInteractionManager() {
    return m_interactionManager;
}

void PdfViewer::ReloadInteractableObjects() {
    if (!m_doc) return;
    
    std::vector<std::shared_ptr<ui::interaction::ISelectableObject>> interactables;
    
    // Only load heavy interactables if the current tool actually needs them
    bool needsText = (m_currentTool == ToolMode::EditText || m_currentTool == ToolMode::AddText);
    bool needsImages = (m_currentTool == ToolMode::Select || m_currentTool == ToolMode::Eraser);
    
    // Load interactable objects from visible pages
    for (const auto& layout : m_layout) {
        if (layout.yOffset + layout.height < m_scrollY || layout.yOffset > m_scrollY + (m_bounds.bottom - m_bounds.top)) {
            continue; // CRITICAL: Only load for actually visible pages!
        }
        
        auto it = m_activePages.find(layout.index);
        auto page = (it != m_activePages.end()) ? it->second : nullptr;
        if (page) {
            if (needsText) {
                auto texts = page->GetTextObjects();
                for (auto& t : texts) {
                    interactables.push_back(std::make_shared<ui::interaction::TextSelectableObject>(t, layout.index));
                }
            }
            
            if (needsImages) {
                auto images = page->GetImages();
                for (auto& i : images) {
                    interactables.push_back(std::make_shared<ui::interaction::ImageSelectableObject>(i, layout.index));
                }
            }
            // Always load annotations so D2D fallback rendering can draw newly created annotations
            auto annots = page->GetAnnotations();
            for (auto& a : annots) {
                interactables.push_back(std::make_shared<ui::interaction::AnnotationSelectableObject>(a, layout.index));
            }
        }
    }
    
    m_interactionManager.SetObjects(interactables);
}

void PdfViewer::InvalidateView() {
    if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
}

void PdfViewer::InsertBlankPage(int index, double width, double height) {
    if (!m_doc) return;
    auto* pdfDoc = dynamic_cast<PdfDocument*>(m_doc.get());
    if (!pdfDoc) return;
    auto cmd = std::make_unique<pdf_engine::commands::InsertBlankPageCommand>(pdfDoc, index, width, height);
    if (m_doc->GetCommandStack().ExecuteCommand(std::move(cmd))) {
        m_activePages.clear();
        m_generation++;
        core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
        UpdateVisibleTiles();
        ReloadInteractableObjects();
        InvalidateView();
    }
}

void PdfViewer::DeletePage(int index) {
    if (!m_doc) return;
    auto* pdfDoc = dynamic_cast<PdfDocument*>(m_doc.get());
    if (!pdfDoc) return;
    if (pdfDoc->PageCount() <= 1) return;
    auto cmd = std::make_unique<pdf_engine::commands::DeletePageCommand>(pdfDoc, index);
    if (m_doc->GetCommandStack().ExecuteCommand(std::move(cmd))) {
        m_activePages.clear();
        m_generation++;
        core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
        UpdateVisibleTiles();
        ReloadInteractableObjects();
        InvalidateView();
    }
}

void PdfViewer::DuplicatePage(int index) {
    if (!m_doc) return;
    auto* pdfDoc = dynamic_cast<PdfDocument*>(m_doc.get());
    if (!pdfDoc) return;
    if (pdfDoc->DuplicatePage(index)) {
        m_activePages.clear();
        m_generation++;
        core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
        UpdateVisibleTiles();
        ReloadInteractableObjects();
        InvalidateView();
    }
}

void PdfViewer::MovePage(int sourceIndex, int destIndex) {
    if (!m_doc) return;
    auto* pdfDoc = dynamic_cast<PdfDocument*>(m_doc.get());
    if (!pdfDoc) return;
    auto cmd = std::make_unique<pdf_engine::commands::MovePageCommand>(pdfDoc, sourceIndex, destIndex);
    if (m_doc->GetCommandStack().ExecuteCommand(std::move(cmd))) {
        m_activePages.clear();
        m_generation++;
        core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
        UpdateVisibleTiles();
        ReloadInteractableObjects();
        InvalidateView();
    }
}

void PdfViewer::RotatePage(int index, int rotationDegrees) {
    if (!m_doc) return;
    auto* pdfDoc = dynamic_cast<PdfDocument*>(m_doc.get());
    if (!pdfDoc) return;
    auto cmd = std::make_unique<pdf_engine::commands::RotatePageCommand>(pdfDoc, index, rotationDegrees);
    if (m_doc->GetCommandStack().ExecuteCommand(std::move(cmd))) {
        m_activePages.clear();
        m_generation++;
        core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
        CachePageSizes();
        UpdateVisibleTiles();
        ReloadInteractableObjects();
        InvalidateView();
    }
}

void PdfViewer::ExecuteMacroStructureChange(std::unique_ptr<core::interfaces::dom::ICommand> cmd) {
    if (!m_doc || !cmd) return;
    if (m_doc->GetCommandStack().ExecuteCommand(std::move(cmd))) {
        m_activePages.clear();
        m_generation++;
        core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
        UpdateVisibleTiles();
        ReloadInteractableObjects();
        InvalidateView();
    }
}

#include <commdlg.h>

void PdfViewer::TriggerInsertImage() {
    wchar_t filename[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"Images (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        // Read file into m_pendingImageData
        HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD size = GetFileSize(hFile, NULL);
            m_pendingImageData.resize(size);
            DWORD read = 0;
            ReadFile(hFile, m_pendingImageData.data(), size, &read, NULL);
            CloseHandle(hFile);
            
            // Just placeholder dimensions for now until placed
            m_pendingImageWidth = 100;
            m_pendingImageHeight = 100;
            m_isInsertingImage = true;
            m_currentTool = ToolMode::Select; // Change cursor state
        }
    }
}
void PdfViewer::PasteImage() {}

void PdfViewer::SetLayoutMode(LayoutMode mode) {
    if (m_layoutMode != mode) {
        m_layoutMode = mode;
        if (mode == LayoutMode::SinglePage) {
            m_scrollY = 0.0f; // Reset scroll when switching to single page
        }
        UpdateVisibleTiles();
        InvalidateView();
    }
}

void PdfViewer::OnMouseWheel(float delta) {
    if (m_inputRouter) {
        ui::input::ScrollEvent se;
        se.deltaY = delta;
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) se.modifiers |= ui::input::KeyModifier::Shift;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) se.modifiers |= ui::input::KeyModifier::Control;
        if (GetAsyncKeyState(VK_MENU) & 0x8000) se.modifiers |= ui::input::KeyModifier::Alt;
        se.timestampMs = ::GetTickCount64();

        POINT pt;
        GetCursorPos(&pt);
        if (m_hwnd) ScreenToClient(m_hwnd, &pt);
        float scale = m_hwnd ? (GetDpiForWindow(m_hwnd) / 96.0f) : 1.0f;
        if (scale <= 0.0f) scale = 1.0f;
        se.anchorDip = { static_cast<float>(pt.x) / scale, static_cast<float>(pt.y) / scale };

        auto res = m_inputRouter->RouteMouseWheel(se);
        if (res == ui::input::EventResult::Handled || res == ui::input::EventResult::Consumed) {
            InvalidateView();
            return;
        }
    }

    if (m_layoutMode == LayoutMode::SinglePage) {
        float viewHeight = m_bounds.bottom - m_bounds.top;
        float pageH = m_layout.empty() ? 0 : m_layout[0].height;
        
        bool canScroll = false;
        if (pageH > viewHeight) {
            if (delta < 0 && m_scrollY < pageH - viewHeight) canScroll = true;
            if (delta > 0 && m_scrollY > 0.0f) canScroll = true;
        }
        
        if (canScroll) {
            float scaledDelta = -((delta / 120.0f) * 100.0f);
            m_kineticFilter.AddWheelDelta(0.0f, scaledDelta * 10.0f);
            OnScroll(scaledDelta);
            m_accumulatedWheelDelta = 0.0f;
            return;
        }
        
        m_accumulatedWheelDelta += delta;
        const float FLIP_THRESHOLD = 120.0f; // one standard notch
        if (m_accumulatedWheelDelta <= -FLIP_THRESHOLD) {
            GoToPage(m_currentPage + 1);
            m_accumulatedWheelDelta = 0.0f;
        } else if (m_accumulatedWheelDelta >= FLIP_THRESHOLD) {
            GoToPage(m_currentPage - 1);
            m_accumulatedWheelDelta = 0.0f;
        }
    } else {
        float scaledDelta = -((delta / 120.0f) * 100.0f);
        m_kineticFilter.AddWheelDelta(0.0f, scaledDelta * 10.0f);
        OnScroll(scaledDelta);
    }
}

bool PdfViewer::UpdatePhysics() {
    if (!m_kineticFilter.IsActive()) return false;
    float dx = 0.0f, dy = 0.0f;
    if (m_kineticFilter.Update(0.016, dx, dy)) {
        if (dx != 0.0f) OnScrollX(dx);
        if (dy != 0.0f) OnScroll(dy);
        return true;
    }
    return false;
}

void PdfViewer::OnScroll(float deltaY) {
    // Clamp to [0, maxScrollY]. Without the upper bound the user can scroll past
    // the last page, at which point every page fails the visibility test in
    // Render and the whole canvas paints blank -- which looks identical to the
    // "content disappeared" bug. maxScrollY keeps the last page's bottom reachable
    // but never lets the view scroll into empty space below the document.
    float viewHeight = m_bounds.bottom - m_bounds.top;
    float totalHeight = 0.0f;
    if (!m_layout.empty()) {
        totalHeight = m_layout.back().yOffset + m_layout.back().height;
    }
    float maxScrollY = std::max(0.0f, totalHeight - viewHeight);
    m_scrollY = std::max(0.0f, std::min(m_scrollY + deltaY, maxScrollY));
    UpdateVisibleTiles();
    InvalidateView();
}

void PdfViewer::OnScrollX(float deltaX) {
    float maxScroll = 0;
    float width = m_bounds.right - m_bounds.left;
    for (const auto& layout : m_layout) {
        if (layout.width > width) {
            float overflow = layout.width - width;
            if (overflow > maxScroll) maxScroll = overflow;
        }
    }
    m_scrollX = static_cast<int>(std::max(0.0f, std::min(static_cast<float>(m_scrollX + deltaX), maxScroll)));
    UpdateVisibleTiles();
    InvalidateView();
}

void PdfViewer::OnThumbnailScroll(int deltaY) {
    m_thumbnailScrollY = std::max(0.0f, m_thumbnailScrollY + deltaY);
}

void PdfViewer::OnZoom(double factor, double pivotX, double pivotY) {
    m_fitWidthMode = false; // user is driving zoom manually; stop auto fit-to-width
    
    if (!m_isZooming) {
        m_prevLayout = m_layout;
        m_prevZoom = m_zoom;
        m_prevScrollX = m_scrollX;
        m_prevScrollY = m_scrollY;
        m_isZooming = true;
    }

    double px = 0, py = 0;
    int hitPage = -1;
    if (m_interactionManager.viewToPage) {
        m_interactionManager.viewToPage(pivotX, pivotY, px, py, hitPage);
    }
    bool exactHit = (hitPage >= 0);

    // If we missed the page, fallback to a "best effort" using the layout's global Y.
    float oldYOffset = 0.0f;
    if (!exactHit) {
        for (const auto& layout : m_layout) {
            float pageY = m_bounds.top + layout.yOffset - m_scrollY;
            if (pivotY >= pageY && pivotY <= pageY + layout.height) {
                hitPage = layout.index;
                oldYOffset = layout.yOffset;
                break;
            }
        }
    }

    m_zoom = std::max(0.1, std::min(10.0, m_zoom * factor)); m_layoutDirty = true;
    float width = m_bounds.right - m_bounds.left;

    if (hitPage >= 0 && m_doc && hitPage < m_cachedPageSizes.size()) {
        float unscaledW = m_cachedPageSizes[hitPage].first;
        float unscaledH = m_cachedPageSizes[hitPage].second;
        
        float newPageWidth = unscaledW * static_cast<float>(m_zoom);
        
        float newYOffset = 0.0f;
        for (int i = 0; i < hitPage; ++i) {
            newYOffset += m_cachedPageSizes[i].second * static_cast<float>(m_zoom);
        }

        if (exactHit) {
            float expectedPageX_NoScroll = m_bounds.left + std::max(0.0f, (width - newPageWidth) / 2.0f);
            float expectedPageY_NoScroll = m_bounds.top + newYOffset;

            int rot = m_doc->GetPage(hitPage)->GetRotation();
            CoordinateConverter::PageContext pageCtx{ unscaledW, unscaledH, rot };
            CoordinateConverter::ViewContext viewCtx{ m_zoom, 0.0, 0.0, expectedPageX_NoScroll, expectedPageY_NoScroll };
            
            PointF screenPt_NoScroll = CoordinateConverter::PdfToScreen(pageCtx, viewCtx, px, py);

            m_scrollX = static_cast<int>(std::max(0.0, screenPt_NoScroll.x - pivotX));
            m_scrollY = static_cast<float>(std::max(0.0, screenPt_NoScroll.y - pivotY));
        } else {
            // Clicked in the horizontal margin next to a page.
            float relativeY = static_cast<float>((pivotY + m_prevScrollY - m_bounds.top - oldYOffset) / m_prevZoom);
            float newExpectedY = m_bounds.top + newYOffset + relativeY * static_cast<float>(m_zoom);
            m_scrollY = static_cast<float>(std::max(0.0, newExpectedY - pivotY));
            
            double oldUncenteredDocX = (pivotX + m_prevScrollX) / m_prevZoom;
            m_scrollX = static_cast<int>(std::max(0.0, oldUncenteredDocX * m_zoom - pivotX));
        }
    } else {
        double oldUncenteredDocX = (pivotX + m_prevScrollX) / m_prevZoom;
        double oldDocY = (pivotY + m_prevScrollY) / m_prevZoom;
        m_scrollX = static_cast<int>(std::max(0.0, oldUncenteredDocX * m_zoom - pivotX));
        m_scrollY = static_cast<float>(std::max(0.0, oldDocY * m_zoom - pivotY));
    }

    UpdateVisibleTiles();
    InvalidateView();
}

void PdfViewer::ZoomToFitWidth() {
    m_fitWidthMode = true;
    if (!m_doc || m_cachedPageSizes.empty()) return;
    float maxWidth = 0;
    for (const auto& size : m_cachedPageSizes) {
        if (size.first > maxWidth) maxWidth = size.first;
    }
    m_maxPageWidth = maxWidth;
    if (maxWidth > 0) {
        float viewWidth = m_bounds.right - m_bounds.left - 40.0f; // 40px margin
        if (viewWidth > 0) {
            m_zoom = viewWidth / maxWidth; m_layoutDirty = true;
            UpdateVisibleTiles();
            InvalidateView();
        }
    }
}

void PdfViewer::ZoomToFitPage() {
    m_fitWidthMode = false;
    if (m_cachedPageSizes.empty() || m_currentPage < 0 || m_currentPage >= (int)m_cachedPageSizes.size()) return;
    float pw = m_cachedPageSizes[m_currentPage].first;
    float ph = m_cachedPageSizes[m_currentPage].second;
    if (pw > 0 && ph > 0) {
        float viewWidth = m_bounds.right - m_bounds.left - 40.0f;
        float viewHeight = m_bounds.bottom - m_bounds.top - 40.0f;
        if (viewWidth > 0 && viewHeight > 0) {
            m_zoom = std::min(static_cast<double>(viewWidth) / pw, static_cast<double>(viewHeight) / ph); m_layoutDirty = true;
            UpdateVisibleTiles();
            InvalidateView();
        }
    }
}
void PdfViewer::OnResize(const D2D1_RECT_F& bounds) {
    m_bounds = bounds; m_layoutDirty = true;
    if (m_doc && m_fitWidthMode) {
        // Re-fit on every resize while in fit-width mode so the page tracks the canvas
        // width and stays centered. A one-time fit would leave the page glued to the
        // left after the window or side panels change size.
        ZoomToFitWidth();
    } else {
        UpdateVisibleTiles();
    }
}

void PdfViewer::GoToPage(int pageIndex) {
    if (!m_doc || pageIndex < 0 || pageIndex >= m_doc->PageCount()) return;
    m_currentPage = pageIndex;
    if (m_layoutMode == LayoutMode::Continuous) {
        for (const auto& layout : m_layout) {
            if (layout.index == pageIndex) {
                m_scrollY = layout.yOffset;
                break;
            }
        }
    } else {
        m_scrollY = 0.0f;
    }
    UpdateVisibleTiles();
    InvalidateView();
}

void PdfViewer::NavigateTo(const NavigationTarget& target) {
    GoToPage(target.pageIndex);
}

void PdfViewer::Render(ComPtr<ID2D1RenderTarget> target, const D2D1_RECT_F& bounds) {
    utils::Logger::Log("PDFVIEWER_RENDER_START");
    
    utils::Logger::Log("PDFVIEWER_RENDER_1");
    target->PushAxisAlignedClip(bounds, D2D1_ANTIALIAS_MODE_ALIASED);
    
    ComPtr<ID2D1SolidColorBrush> bgBrush;
    target->CreateSolidColorBrush(design::Colors::Workspace, &bgBrush);
    target->FillRectangle(bounds, bgBrush.Get());

    if (!m_doc) {
        utils::Logger::Log("PDFVIEWER_RENDER_END_NO_DOC");
        target->PopAxisAlignedClip();
        return;
    }

    utils::Logger::Log("PDFVIEWER_RENDER_2");
    float width = bounds.right - bounds.left;
    float height = bounds.bottom - bounds.top;

    // Update m_bounds so hit-testing is always against the live canvas rect.
    // DO NOT call UpdateVisibleTiles() or InvalidateView() from here --
    // that would create an infinite repaint loop (Render -> UpdateVisibleTiles
    // -> ReloadInteractableObjects -> InvalidateView -> WM_PAINT -> Render).
    // Fit-width zoom is applied in ZoomToFitWidth() which is called from OnResize()
    // and SetDocument(). Those are the correct, non-recursive paths.
    if (width > 0.0f && height > 0.0f) {
        m_bounds = bounds; m_layoutDirty = true;
    }

    if (width == 0 || height == 0) {
        width = 1200;
        height = 800;
    }

    D2D1_RECT_F visibleRect = D2D1::RectF(0, static_cast<float>(m_scrollY), width, static_cast<float>(m_scrollY) + height);

    utils::Logger::Log("PDFVIEWER_RENDER_3: layout loop size=" + std::to_string(m_layout.size()));
    // Per-frame tile accounting. If the page shows white after briefly appearing,
    // this line tells us instantly whether tiles are cache-HITTING (drawn) or
    // perpetually MISSING+re-REQUESTing (the cache-key-mismatch / zoom-oscillation
    // failure mode). A healthy steady state is hit>0, miss==0, req==0.
    int tileHit = 0, tileMiss = 0, tileReq = 0;

    float dpiScale = 1.0f;
    if (m_hwnd) {
        dpiScale = GetDpiForWindow(m_hwnd) / 96.0f;
    }

    if (m_isZooming && m_prevZoom > 0.0) {
        float scaleRatio = static_cast<float>(m_zoom / m_prevZoom);
        for (const auto& prevPage : m_prevLayout) {
            float docY = static_cast<float>(prevPage.yOffset / m_prevZoom);
            float newYOffset = static_cast<float>(docY * m_zoom);
            
            float pageX = bounds.left - m_scrollX + std::max(0.0f, (width - (prevPage.width * scaleRatio)) / 2.0f);
            float pageY = bounds.top + newYOffset - m_scrollY;
            
            float newHeight = prevPage.height * scaleRatio;
            if (pageY + newHeight < bounds.top || pageY > bounds.bottom) continue;
            
            int tw = 256;
            int th = 256;
            
            float visLeft = std::max(0.0f, bounds.left - pageX);
            float visTop = std::max(0.0f, bounds.top - pageY);
            float visRight = std::min(prevPage.width * scaleRatio, bounds.right - pageX);
            float visBottom = std::min(newHeight, bounds.bottom - pageY);

            if (visLeft < visRight && visTop < visBottom) {
                int startC = std::max(0, static_cast<int>((visLeft / scaleRatio) / tw));
                int endC = static_cast<int>((visRight / scaleRatio) / tw);
                int startR = std::max(0, static_cast<int>((visTop / scaleRatio) / th));
                int endR = static_cast<int>((visBottom / scaleRatio) / th);

                int rCount = static_cast<int>(std::ceil(prevPage.height / th));
                int cCount = static_cast<int>(std::ceil(prevPage.width / tw));
                
                for (int r = startR; r <= endR && r < rCount; ++r) {
                    for (int c = startC; c <= endC && c < cCount; ++c) {
                    TileKey prevKey{m_generation, prevPage.index, static_cast<float>(m_prevZoom), dpiScale, c * tw, r * th, tw, th};
                    auto bitmap = m_tileCache->Get(prevKey);
                    if (bitmap) {
                        float dx = snapPx(pageX + c * tw * scaleRatio, dpiScale);
                        float dy = snapPx(pageY + r * th * scaleRatio, dpiScale);
                        float dw = snapPx(tw * scaleRatio, dpiScale);
                        float dh = snapPx(th * scaleRatio, dpiScale);
                        D2D1_RECT_F destRect = D2D1::RectF(dx, dy, dx + dw, dy + dh);
                        target->DrawBitmap(bitmap.Get(), destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
                    }
                }
            }
            }
        }
    }

    for (const auto& page : m_layout) {
        if (page.yOffset + page.height < visibleRect.top || page.yOffset > visibleRect.bottom) {
            continue;
        }

        float pageX = bounds.left - m_scrollX + std::max(0.0f, (width - page.width) / 2.0f);
        float pageY = bounds.top + page.yOffset - m_scrollY;

        // DPI note: tiles are rendered using the actual monitor DPI scaling factor.
        // This ensures the bitmap is rendered crisp instead of scaling up logically.

        D2D1_RECT_F pageScreenRect = D2D1::RectF(pageX, pageY, pageX + page.width, pageY + page.height);

        float pgLeft = std::round(pageScreenRect.left);
        float pgTop = std::round(pageScreenRect.top);
        float pgRight = std::round(pageScreenRect.right);
        float pgBottom = std::round(pageScreenRect.bottom);

        for (int i = 1; i <= 4; ++i) { 
            float alpha = 0.08f / i; 
            ComPtr<ID2D1SolidColorBrush> shadowBrush; 
            if (SUCCEEDED(target->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, alpha), &shadowBrush)) && shadowBrush) {
                D2D1_RECT_F shadowRect = D2D1::RectF(pgLeft + i, pgTop + i, pgRight + i, pgBottom + i); 
                target->FillRectangle(shadowRect, shadowBrush.Get()); 
            }
        }

        ComPtr<ID2D1SolidColorBrush> whiteBrush;
        target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &whiteBrush);
        target->FillRectangle(pageScreenRect, whiteBrush.Get());

        utils::Logger::Log("PDFVIEWER_RENDER_4: get page " + std::to_string(page.index));
        {
            char pgbuf[256];
            snprintf(pgbuf, sizeof(pgbuf),
                "PDFVIEWER_PAGE_POS: page=%d pageX=%.1f pageY=%.1f pw=%.1f ph=%.1f "
                "boundsL=%.1f boundsT=%.1f boundsR=%.1f boundsB=%.1f scrollY=%.1f",
                page.index, pageX, pageY, page.width, page.height,
                bounds.left, bounds.top, bounds.right, bounds.bottom, (float)m_scrollY);
            utils::Logger::Log(pgbuf);
        }
        // Use find() not operator[] -- operator[] on std::map silently inserts
        // a null shared_ptr entry for missing keys, corrupting m_activePages.
        auto apIt = m_activePages.find(page.index);
        auto pdfPage = (apIt != m_activePages.end()) ? apIt->second : nullptr;
        
        utils::Logger::Log("PDFVIEWER_PAGE " + std::to_string(page.index) + " pdfPage=" + (pdfPage ? "VALID" : "NULL"));
        
        if (pdfPage) {
            int cols = static_cast<int>(std::ceil(page.width / (TILE_SIZE - OVERLAP_PX)));
            int rows = static_cast<int>(std::ceil(page.height / (TILE_SIZE - OVERLAP_PX)));

            float visLeft = std::max(0.0f, bounds.left - pageX);
            float visTop = std::max(0.0f, bounds.top - pageY);
            float visRight = std::min(page.width, bounds.right - pageX);
            float visBottom = std::min(page.height, bounds.bottom - pageY);

            if (visLeft < visRight && visTop < visBottom) {
                int startC = std::max(0, static_cast<int>(visLeft / (TILE_SIZE - OVERLAP_PX)));
                int endC = static_cast<int>(visRight / (TILE_SIZE - OVERLAP_PX));
                int startR = std::max(0, static_cast<int>(visTop / (TILE_SIZE - OVERLAP_PX)));
                int endR = static_cast<int>(visBottom / (TILE_SIZE - OVERLAP_PX));

                for (int r = startR; r <= endR && r < rows; ++r) {
                    for (int c = startC; c <= endC && c < cols; ++c) {
                        int srcX = c * (TILE_SIZE - OVERLAP_PX);
                        int srcY = r * (TILE_SIZE - OVERLAP_PX);
                    int tw = std::min(TILE_SIZE, static_cast<int>(std::round(page.width)) - srcX);
                    int th = std::min(TILE_SIZE, static_cast<int>(std::round(page.height)) - srcY);

                    TileKey key{m_generation, page.index, static_cast<float>(m_zoom), dpiScale, srcX, srcY, tw, th};
                    auto bitmap = m_tileCache->Get(key);
                    bool isFallback = false;
                    
                    if (!bitmap && m_generation > 0) {
                        TileKey prevKey = key;
                        prevKey.documentGeneration = m_generation - 1;
                        bitmap = m_tileCache->Get(prevKey);
                        if (bitmap) isFallback = true;
                    }
                    
                    if (bitmap) {
                        if (isFallback) {
                            // It's a hit for drawing to avoid flicker, but it's a MISS for the current generation!
                            // So we do not increment tileHit (we treat it as a miss for accounting purposes).
                            ++tileMiss;
                        } else {
                            ++tileHit;
                        }
                        
                        float dx = pageX + static_cast<float>(srcX);
                        float dy = pageY + static_cast<float>(srcY);
                        D2D1_RECT_F destRect = D2D1::RectF(
                            snapPx(dx, dpiScale), snapPx(dy, dpiScale),
                            snapPx(dx + static_cast<float>(tw), dpiScale),
                            snapPx(dy + static_cast<float>(th), dpiScale));
                            
                        if (r == 0 && c == 0) {
                            char tbuf[256];
                            snprintf(tbuf, sizeof(tbuf),
                                "TILE_DRAW: page=%d destRect={%.1f,%.1f,%.1f,%.1f} bmpSize=%dx%d%s",
                                page.index, destRect.left, destRect.top, destRect.right, destRect.bottom,
                                bitmap->GetPixelSize().width, bitmap->GetPixelSize().height,
                                isFallback ? " (FALLBACK)" : "");
                            utils::Logger::Log(tbuf);
                        }
                        target->DrawBitmap(bitmap.Get(), destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
                    } else {
                        ++tileMiss;
                    }
                    
                    // If we didn't have the exact current generation bitmap, we MUST request it!
                    if (isFallback || !bitmap) {
                        if (m_requestedTiles.find(key) == m_requestedTiles.end()) {
                            ++tileReq;
                            m_requestedTiles.insert(key);
                            core::models::RenderRequest req;
                            req.documentId = m_documentId;
                            req.generation = static_cast<int>(m_generation);
                            req.pageIndex = page.index;
                            req.renderScale = static_cast<float>(m_zoom);
                            req.dpi = dpiScale; // request crisp pixels
                            req.viewport = {0,0,0,0};
                            req.tileRect = {static_cast<float>(srcX), static_cast<float>(srcY), static_cast<float>(srcX + tw), static_cast<float>(srcY + th)};
                            
                            float screenCy = visibleRect.top + (visibleRect.bottom - visibleRect.top)/2.0f;
                            float tileCy = page.yOffset + srcY + th/2.0f;
                            req.priority = -static_cast<int>(std::abs(screenCy - tileCy));
                            req.tileCy = tileCy;
                            req.category = core::models::RenderPriority::Visible;
                            req.darkMode = m_isDarkMode;
                            
                            core::RenderController::Instance().EnqueueRequest(req);
                        }
                    }
                }
            }
            }
        }

        utils::Logger::Log("PDFVIEWER_RENDER_5: search/selection");
        
        if (m_selection.startPage >= 0 && m_selection.endPage == m_selection.startPage && m_selection.startChar >= 0 && m_selection.endChar >= 0) {
            int start = std::min(m_selection.startChar, m_selection.endChar);
            int end = std::max(m_selection.startChar, m_selection.endChar);
            int count = end - start + 1;
            
            auto* tp = GetTextPage(m_selection.startPage);
            if (tp && count > 0) {
                auto rects = tp->GetRects(start, count);
                if (!rects.empty()) {
                    ComPtr<ID2D1SolidColorBrush> selBrush;
                    if (SUCCEEDED(target->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.4f, 1.0f, 0.4f), &selBrush)) && selBrush) {
                        for (const auto& r : rects) {
                            double vx1, vy1, vx2, vy2;
                            m_interactionManager.pageToView(r.left, r.bottom, m_selection.startPage, vx1, vy1);
                            m_interactionManager.pageToView(r.right, r.top, m_selection.startPage, vx2, vy2);
                            
                            float left = static_cast<float>(std::min(vx1, vx2));
                            float right = static_cast<float>(std::max(vx1, vx2));
                            float top = static_cast<float>(std::min(vy1, vy2));
                            float bottom = static_cast<float>(std::max(vy1, vy2));
                            
                            target->FillRectangle(D2D1::RectF(left, top, right, bottom), selBrush.Get());
                        }
                    }
                }
            }
        }
        
        // Render search results via SearchHighlightOverlay
        if (!m_searchResults.empty()) {
            ui::search::SearchHighlightOverlay overlay;
            overlay.SetResults(m_searchResults, m_activeSearchIndex);
            overlay.Render(
                target.Get(),
                bounds,
                [this](int pageIndex) -> core::interfaces::dom::ITextPage* {
                    return GetTextPage(pageIndex);
                },
                [this](double px, double py, int pageIdx, double& vx, double& vy) {
                    m_interactionManager.pageToView(px, py, pageIdx, vx, vy);
                }
            );
        }

        if (m_doc) {
            if (auto docPage = m_doc->GetPage(page.index)) {
                ui::components::AnnotationOverlay::Render(
                    target.Get(),
                    docPage.get(),
                    [this, pageIdx = page.index](PointF pt) -> PointF {
                        double vx, vy;
                        m_interactionManager.pageToView(pt.x, pt.y, pageIdx, vx, vy);
                        return PointF{static_cast<float>(vx), static_cast<float>(vy)};
                    }
                );
            }
        }
    }
    
    // Single authoritative per-frame diagnostic. Everything needed to explain
    // "blank / not centered / won't scroll" is here: fit state + zoom, the widest
    // page width, the live canvas width, the scroll offset, and the tile tallies.
    {
        char buf[256];
        float w = bounds.right - bounds.left;
        snprintf(buf, sizeof(buf),
            "PDFVIEWER_RENDER_8: end fit=%d zoom=%.4f maxW=%.1f width=%.1f scrollY=%.1f pages=%zu hit=%d miss=%d req=%d",
            m_fitWidthMode ? 1 : 0, m_zoom, m_maxPageWidth, w, m_scrollY,
            m_layout.size(), tileHit, tileMiss, tileReq);
        utils::Logger::Log(buf);
    }
    
    if (m_isZooming && tileMiss == 0) {
        m_isZooming = false;
    }
    
    if (m_activeHandler) {
        m_activeHandler->RenderPreview(target.Get(), static_cast<float>(m_zoom), PointF{static_cast<float>(m_scrollX), m_scrollY});
    }
    
    // Modern Tool Overlay Render (8-way transform handles, rotation handle, selection marquee)
    if (m_toolStateMachine) {
        m_toolStateMachine->RenderOverlay(target.Get());
    }

    // Draw scrollbar
    float totalHeight = 0;
    if (!m_layout.empty()) {
        totalHeight = m_layout.back().yOffset + m_layout.back().height;
    }
    if (totalHeight > height && height > 0) {
        float scrollbarHeight = std::max(40.0f, (height / totalHeight) * height);
        float scrollbarY = (m_scrollY / totalHeight) * height;
        
        D2D1_RECT_F scrollbarRect = D2D1::RectF(
            bounds.right - 12.0f,
            bounds.top + scrollbarY,
            bounds.right - 4.0f,
            bounds.top + scrollbarY + scrollbarHeight
        );
        
        ComPtr<ID2D1SolidColorBrush> scrollBrush;
        if (SUCCEEDED(target->CreateSolidColorBrush(D2D1::ColorF(0.3f, 0.3f, 0.3f, 0.6f), &scrollBrush)) && scrollBrush) {
            target->FillRectangle(scrollbarRect, scrollBrush.Get());
        }
    }

        // Draw horizontal scrollbar
    float maxLayoutWidth = 0.0f;
    for (const auto& layout : m_layout) {
        if (layout.width > maxLayoutWidth) maxLayoutWidth = layout.width;
    }
    float viewWidth = bounds.right - bounds.left;
    if (maxLayoutWidth > viewWidth && viewWidth > 0) {
        float scrollbarWidth = std::max(40.0f, (viewWidth / maxLayoutWidth) * viewWidth);
        float scrollbarX = (m_scrollX / maxLayoutWidth) * viewWidth;
        
        D2D1_RECT_F hScrollbarRect = D2D1::RectF(
            bounds.left + scrollbarX,
            bounds.bottom - 12.0f,
            bounds.left + scrollbarX + scrollbarWidth,
            bounds.bottom - 4.0f
        );
        
        ComPtr<ID2D1SolidColorBrush> scrollBrush;
        if (SUCCEEDED(target->CreateSolidColorBrush(D2D1::ColorF(0.3f, 0.3f, 0.3f, 0.6f), &scrollBrush)) && scrollBrush) {
            target->FillRectangle(hScrollbarRect, scrollBrush.Get());
        }
    }

    target->PopAxisAlignedClip();
}

void PdfViewer::OnUndo() {
    if (m_doc && m_doc->GetCommandStack().Undo()) {
        m_generation++;
        core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
        CachePageSizes();
        UpdateVisibleTiles();
        ReloadInteractableObjects();
        InvalidateView();
    }
}

void PdfViewer::OnRedo() {
    if (m_doc && m_doc->GetCommandStack().Redo()) {
        m_generation++;
        core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
        CachePageSizes();
        UpdateVisibleTiles();
        ReloadInteractableObjects();
        InvalidateView();
    }
}

void PdfViewer::OnPaste() {
    if (!m_doc) return;
    
    if (OpenClipboard(m_hwnd)) {
        HBITMAP hbm = (HBITMAP)GetClipboardData(CF_BITMAP);
        if (hbm) {
            HDC hdc = GetDC(NULL);
            BITMAPINFO bmi = { 0 };
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            if (GetDIBits(hdc, hbm, 0, 0, NULL, &bmi, DIB_RGB_COLORS)) {
                int width = bmi.bmiHeader.biWidth;
                int height = abs(bmi.bmiHeader.biHeight);
                bmi.bmiHeader.biBitCount = 32;
                bmi.bmiHeader.biCompression = BI_RGB;
                bmi.bmiHeader.biHeight = -height;
                
                std::vector<uint8_t> pixels(width * height * 4);
                if (GetDIBits(hdc, hbm, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS)) {
                    int pageIndex = GetCurrentPage();
                    
                    // Paste near center of viewport
                    float vx = m_scrollX + (m_hwnd ? 400.0f : 200.0f); // Fallback heuristics
                    float vy = m_scrollY + (m_hwnd ? 300.0f : 200.0f);
                    double px, py;
                    m_interactionManager.viewToPage(vx, vy, px, py, pageIndex);
                    
                    float pw = 100.0f;
                    float ph = (float)height * (100.0f / (float)width);
                    RectF bounds = { (float)px, (float)py, (float)px + pw, (float)py - ph };
                    
                    auto cmd = std::make_unique<pdf_engine::commands::InsertImageCommand>(
                        m_doc.get(), pageIndex, pixels, width, height, bounds
                    );
                    if (m_toolStateMachine) {
                        m_toolStateMachine->GetContext().executeCommand(std::move(cmd));
                    }
                }
            }
            ReleaseDC(NULL, hdc);
        }
        CloseClipboard();
    }
}


void PdfViewer::UpdateVisibleTiles() {
    printf("[TRACE_UVT 1: start]\n"); fflush(stdout);
    // Stale tile requests from a previous zoom or scroll state must be purged.
    // Otherwise the Render loop sees them in m_requestedTiles and skips re-requesting,
    // leaving tiles that were never fulfilled under the new parameters.
    m_requestedTiles.clear();

    std::map<int, std::shared_ptr<core::interfaces::dom::IPage>> newActivePages;
    bool changed = false;
    if (!m_doc || m_cachedPageSizes.empty()) {
        printf("[TRACE_UVT 2: doc null or empty]\n"); fflush(stdout);
        m_activePages.clear();
        return;
    }
    
    float height = m_bounds.bottom - m_bounds.top;
    if (height == 0) height = 800; // fallback

    printf("[TRACE_UVT 3: set viewport]\n"); fflush(stdout);
    core::RenderController::Instance().SetViewport(static_cast<float>(m_scrollY) + height / 2.0f, height);

    m_maxPageWidth = 0.0f;
    float y = 0.0f;
    int pageCount = m_doc->PageCount();
    printf("[TRACE_UVT 4: loop pageCount = %d]\n", pageCount); fflush(stdout);
    
    if (m_layoutDirty || m_layout.size() != (m_layoutMode == LayoutMode::SinglePage ? 1 : pageCount)) {
        m_layout.resize(m_layoutMode == LayoutMode::SinglePage ? 1 : pageCount);
        int layoutIdx = 0;
        for (int i = 0; i < pageCount; ++i) {
            if (i >= m_cachedPageSizes.size()) break;
            if (m_layoutMode == LayoutMode::SinglePage && i != m_currentPage) continue;
            
            float w = m_cachedPageSizes[i].first;
            float h = m_cachedPageSizes[i].second;
            if (w > m_maxPageWidth) m_maxPageWidth = w;
            
            m_layout[layoutIdx].index = i;
            m_layout[layoutIdx].width = static_cast<float>(w * m_zoom);
            m_layout[layoutIdx].height = static_cast<float>(h * m_zoom);
            m_layout[layoutIdx].yOffset = y;
            
            y += m_layout[layoutIdx].height + 20.0f;
            layoutIdx++;
        }
        m_layoutDirty = false;
    } else {
        // Just calculate max page width (it might be cached but doing it is fast for < 1000 pages)
        // Actually, just loop to find visible tiles using existing m_layout
        for (const auto& layout : m_layout) {
            if (layout.width / m_zoom > m_maxPageWidth) m_maxPageWidth = static_cast<float>(layout.width / m_zoom);
        }
    }

    float viewCenterY = static_cast<float>(m_scrollY) + height / 2.0f;
    int visiblePage = m_currentPage;
    for (const auto& layout : m_layout) {
        int i = layout.index;
        if (layout.yOffset <= viewCenterY && layout.yOffset + layout.height >= viewCenterY) {
            visiblePage = i;
        }
        // Check if page is visible
        if (layout.yOffset + layout.height >= m_scrollY && layout.yOffset <= m_scrollY + height) {
            // Keep existing or load new
            if (m_activePages.find(i) != m_activePages.end()) {
                newActivePages[i] = m_activePages[i];
                utils::Logger::Log("UPDATE_VISIBLE_TILES: kept page " + std::to_string(i));
            } else {
                printf("[TRACE_UVT 5: calling GetPage(%d)]\n", i); fflush(stdout);
                newActivePages[i] = m_doc->GetPage(i);
                printf("[TRACE_UVT 6: GetPage(%d) done]\n", i); fflush(stdout);
                changed = true;
                utils::Logger::Log("UPDATE_VISIBLE_TILES: loaded page " + std::to_string(i) + " ptr=" + (newActivePages[i] ? "VALID" : "NULL"));
            }
        }
        
        y += layout.height + 20.0f; // 20px gap
    }
    
    if (m_activePages.size() != newActivePages.size()) {
        changed = true;
    }
    
    if (m_layoutMode == LayoutMode::Continuous && !m_layout.empty()) {
        float viewCenter = m_scrollY + height / 2.0f;
        int bestPage = 0;
        for (const auto& l : m_layout) {
            if (viewCenter >= l.yOffset && viewCenter <= l.yOffset + l.height) {
                bestPage = l.index;
                break;
            } else if (l.yOffset > viewCenter) {
                break;
            }
            bestPage = l.index;
        }
        if (m_currentPage != bestPage) {
            m_currentPage = bestPage;
            if (onPageChanged) {
                onPageChanged(m_currentPage, m_doc ? m_doc->PageCount() : 1);
            }
        }
    }
    m_activePages = newActivePages;
    if (changed) {
        bool needsInteractables = (m_currentTool == ToolMode::Select ||
                                   m_currentTool == ToolMode::EditText ||
                                   m_currentTool == ToolMode::AddText);
        if (needsInteractables) {
            // Post a custom timer to the main window to debounce text extraction.
            // When this fires, the UI thread will fetch the interaction objects without stuttering during the scroll.
            if (m_hwnd) {
                SetTimer(m_hwnd, 1001, 150, nullptr);
            }
        }
    }
    printf("[TRACE_UVT 7: finished]\n"); fflush(stdout);
}



void PdfViewer::OnTileReady(core::models::RenderResult* result, Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) {
    if (!result || result->documentId != m_documentId || result->generation != static_cast<int>(m_generation)) return;

    // The physical bitmap size (result->width/height) may differ from the tile dims
    // when req.dpi != 1, but D2D draws it into the correct logical rect regardless.
    const float dpiScale = result->dpi;
    int srcX = static_cast<int>(result->tileRect.left);
    int srcY = static_cast<int>(result->tileRect.top);
    int tw = static_cast<int>(result->tileRect.right - result->tileRect.left);
    int th = static_cast<int>(result->tileRect.bottom - result->tileRect.top);
    TileKey key{m_generation, result->pageIndex, result->renderScale, dpiScale, srcX, srcY, tw, th};

    if (!target || result->pixelBuffer.empty()) {
        m_requestedTiles.erase(key);
        InvalidateView();
        return;
    }

    D2D1_SIZE_U size = D2D1::SizeU(result->width, result->height);
    D2D1_BITMAP_PROPERTIES props;
    props.pixelFormat.format = (result->pixelFormat == core::models::PixelFormat::RGBA8) ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_B8G8R8A8_UNORM;
    props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    props.dpiX = 96.0f; // ID2D1Bitmap requires DPI to be consistent with target, or just 96 for unscaled bits
    props.dpiY = 96.0f;

    Microsoft::WRL::ComPtr<ID2D1Bitmap> d2dBitmap;
    HRESULT hr = target->CreateBitmap(
        size,
        result->pixelBuffer.data(),
        result->stride,
        &props,
        &d2dBitmap
    );

    if (SUCCEEDED(hr) && d2dBitmap) {
        char bmpbuf[128];
        snprintf(bmpbuf, sizeof(bmpbuf), "TILE_READY_SUCCESS: page=%d hr=0x%08X size=%dx%d", 
                 result->pageIndex, (unsigned)hr, result->width, result->height);
        utils::Logger::Log(bmpbuf);

        m_tileCache->Put(key, d2dBitmap, result->pixelBuffer.size());
        m_requestedTiles.erase(key);
        InvalidateView();
    } else {
        char bmpbuf[128];
        snprintf(bmpbuf, sizeof(bmpbuf), "TILE_READY_FAILED: hr=0x%08X", (unsigned)hr);
        utils::Logger::Log(bmpbuf);
        m_requestedTiles.erase(key); // clear it so we can retry if needed
    }
}

core::interfaces::dom::ITextPage* PdfViewer::GetTextPage(int pageIndex) {
    // Return cached text page if available
    auto it = m_textPages.find(pageIndex);
    if (it != m_textPages.end()) return it->second.get();

    // Load from active page
    auto pageIt = m_activePages.find(pageIndex);
    if (pageIt == m_activePages.end() || !pageIt->second) return nullptr;

    auto tp = pageIt->second->LoadTextPage();
    if (!tp) return nullptr;

    auto* raw = tp.get();
    m_textPages[pageIndex] = std::move(tp);
    return raw;
}

void PdfViewer::SetSearchResults(const std::vector<core::models::SearchResult>& results, int activeIndex) {
    m_searchResults = results;
    m_activeSearchIndex = activeIndex;
    
    // Auto-scroll to active search result if valid
    if (m_activeSearchIndex >= 0 && m_activeSearchIndex < static_cast<int>(m_searchResults.size()) && m_doc) {
        ui::search::SearchHighlightOverlay overlay;
        overlay.SetResults(m_searchResults, m_activeSearchIndex);
        float viewHeight = m_bounds.bottom - m_bounds.top;
        auto scrollRes = overlay.CalculateAutoScroll(
            viewHeight,
            m_scrollY,
            [this](int pageIndex) -> core::interfaces::dom::ITextPage* {
                return GetTextPage(pageIndex);
            },
            [this](double px, double py, int pageIdx, double& vx, double& vy) {
                m_interactionManager.pageToView(px, py, pageIdx, vx, vy);
            }
        );
        if (scrollRes.shouldScroll) {
            m_scrollY = scrollRes.newScrollY;
            UpdateVisibleTiles();
        }
    }
    InvalidateView();
}

void PdfViewer::OnCommand(unsigned __int64 wParam, __int64 /*lParam*/) {
    int wmId = LOWORD(wParam);
    
    // Image Context Menu handling
    if (wmId >= IDM_IMAGE_REPLACE && wmId <= IDM_IMAGE_DELETE) {
        auto selection = m_interactionManager.GetSelection();
        if (selection.size() == 1) {
            auto imgObj = std::dynamic_pointer_cast<ui::interaction::ImageSelectableObject>(selection[0]);
            if (!imgObj) return;
            
            auto pageImages = m_doc->GetPage(imgObj->GetPageIndex())->GetImages();
            std::shared_ptr<core::interfaces::dom::IImage> actualPdfImage = nullptr;
            for (auto& pi : pageImages) {
                if (pi->GetId() == imgObj->GetId()) {
                    actualPdfImage = pi;
                    break;
                }
            }
            if (!actualPdfImage) return;

            if (wmId == IDM_IMAGE_EXTRACT) {
                OPENFILENAMEW ofn = {0};
                WCHAR szFile[260] = {0};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = m_hwnd;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);
                ofn.lpstrFilter = L"JPEG Image\0*.jpg;*.jpeg\0Bitmap\0*.bmp\0All Files\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
                
                if (GetSaveFileNameW(&ofn)) {
                    auto data = actualPdfImage->GetBitmapData();
                    if (!data.empty()) {
                        std::ofstream out(szFile, std::ios::binary);
                        out.write(reinterpret_cast<const char*>(data.data()), data.size());
                        ::ui::dialogs::MessageDialog::Show(m_hwnd, L"PDF Elite", L"Image extracted successfully.", ::ui::dialogs::MessageDialogType::Ok);
                    } else {
                        ::ui::dialogs::MessageDialog::Show(m_hwnd, L"Error", L"Failed to extract image data.", ::ui::dialogs::MessageDialogType::Ok);
                    }
                }
            } else if (wmId == IDM_IMAGE_DELETE) {
                auto cmd = std::make_unique<pdf_engine::commands::DeleteImageCommand>(
                    m_doc.get(), imgObj->GetPageIndex(), actualPdfImage
                );
                m_doc->GetCommandStack().ExecuteCommand(std::move(cmd));
                m_interactionManager.RemoveObject(imgObj->GetId());
                m_generation++;
                core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                ReloadInteractableObjects();
                InvalidateView();
            } else if (wmId == IDM_IMAGE_REPLACE) {
                OPENFILENAMEW ofn = {0};
                WCHAR szFile[260] = {0};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = m_hwnd;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);
                ofn.lpstrFilter = L"JPEG Image\0*.jpg;*.jpeg\0All Files\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                
                if (GetOpenFileNameW(&ofn)) {
                    std::ifstream in(szFile, std::ios::binary | std::ios::ate);
                    if (in.is_open()) {
                        std::streamsize size = in.tellg();
                        in.seekg(0, std::ios::beg);
                        std::vector<uint8_t> buffer(size);
                        if (in.read(reinterpret_cast<char*>(buffer.data()), size)) {
                            auto macro = std::make_unique<pdf_engine::commands::MacroCommand>("Replace Image");
                            
                            auto delCmd = std::make_unique<pdf_engine::commands::DeleteImageCommand>(
                                m_doc.get(), imgObj->GetPageIndex(), actualPdfImage
                            );
                            
                            auto insCmd = std::make_unique<pdf_engine::commands::InsertImageCommand>(
                                m_doc.get(), imgObj->GetPageIndex(), buffer, actualPdfImage->GetWidth(), actualPdfImage->GetHeight(), actualPdfImage->GetBounds()
                            );
                            
                            macro->AddCommand(std::move(delCmd));
                            macro->AddCommand(std::move(insCmd));
                            
                            m_doc->GetCommandStack().ExecuteCommand(std::move(macro));
                            m_generation++;
                            core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                            ReloadInteractableObjects();
                            InvalidateView();
                        }
                    }
                }
            }
        }
    } else if (wmId >= IDM_TEXT_COPY && wmId <= 3017) {
        if (wmId == IDM_TEXT_COPY && m_selection.startPage >= 0 && m_selection.startChar >= 0 && m_selection.endChar >= 0) {
            int start = std::min(m_selection.startChar, m_selection.endChar);
            int end = std::max(m_selection.startChar, m_selection.endChar);
            int count = end - start + 1;
            auto* tp = GetTextPage(m_selection.startPage);
            if (tp && count > 0) {
                std::wstring text = tp->GetText(start, count);
                if (!text.empty()) {
                    core::Clipboard::SetText(m_hwnd, text);
                }
            }
            return;
        }

        auto selection = m_interactionManager.GetSelection();
        if (selection.size() == 1) {
            auto txtObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(selection[0]);
            if (!txtObj) return;
            auto textDomObj = txtObj->GetTextObject();
            if (!textDomObj) return;

            if (wmId == IDM_TEXT_COPY) {
                core::Clipboard::SetText(m_hwnd, textDomObj->GetText());
            } else if (wmId == IDM_TEXT_EDIT) {
                SetToolMode(ToolMode::EditText);
                m_interactionManager.EnterTextEditMode(txtObj);
                InvalidateView();
            } else if (wmId == IDM_TEXT_DELETE) {
                if (m_doc) {
                    auto cmd = std::make_unique<pdf_engine::commands::DeleteTextCommand>(
                        m_doc.get(), txtObj->GetPageIndex(), textDomObj
                    );
                    m_doc->GetCommandStack().ExecuteCommand(std::move(cmd));
                    m_interactionManager.RemoveObject(txtObj->GetId());
                    m_generation++;
                    core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                    ReloadInteractableObjects();
                    InvalidateView();
                }
            }
        }
        
        if ((wmId == 3014 || wmId == 3015 || wmId == 3016) && m_selection.startPage >= 0 && m_selection.startChar >= 0 && m_selection.endChar >= 0) {
            auto type = (wmId == 3014) ? core::interfaces::dom::AnnotationType::Highlight :
                        (wmId == 3015) ? core::interfaces::dom::AnnotationType::Underline :
                                         core::interfaces::dom::AnnotationType::StrikeOut;
                                         
            int start = std::min(m_selection.startChar, m_selection.endChar);
            int end = std::max(m_selection.startChar, m_selection.endChar);
            int count = end - start + 1;
            auto* tp = GetTextPage(m_selection.startPage);
            if (tp && count > 0) {
                auto rects = tp->GetRects(start, count);
                if (!rects.empty()) {
                    float minX = 999999, minY = 999999, maxX = -999999, maxY = -999999;
                    std::vector<QuadF> quads;
                    for (auto& r : rects) {
                        float l = std::min(r.left, r.right);
                        float right = std::max(r.left, r.right);
                        float b = std::min(r.bottom, r.top);
                        float t = std::max(r.bottom, r.top);
                        minX = std::min(minX, l);
                        minY = std::min(minY, b);
                        maxX = std::max(maxX, right);
                        maxY = std::max(maxY, t);
                        quads.push_back({ PointF{l, t}, PointF{right, t}, PointF{l, b}, PointF{right, b} });
                    }
                    auto cmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
                        m_doc.get(), m_selection.startPage, type, RectF{minX, minY, maxX, maxY}
                    );
                    cmd->SetQuads(quads);
                    if (type == core::interfaces::dom::AnnotationType::Highlight) {
                        cmd->SetColor(0, 255, 0, 100); // Default green highlight
                    }
                    if (m_toolStateMachine) {
                        m_toolStateMachine->GetContext().executeCommand(std::move(cmd));
                    }
                    m_selection.startChar = m_selection.endChar = m_selection.startPage = m_selection.endPage = -1;
                    InvalidateView();
                }
            }
        }
    }
    else if (wmId >= 3020 && wmId <= 3023) { // Annotation commands
        if (wmId == 3020) { // AnnotProperties
            auto target = m_contextMenuTarget;
            if (target) {
                m_interactionManager.GetSelectionModel().Clear();
                m_interactionManager.GetSelectionModel().AddSelect(target);
                InvalidateView();
            }
        }
        else if (wmId == 3022) { // AnnotDelete
            auto target = m_contextMenuTarget;
            if (!target) {
                auto selection = m_interactionManager.GetSelection();
                if (selection.size() == 1) target = selection[0];
            }
            if (target) {
                auto annotObj = std::dynamic_pointer_cast<ui::interaction::AnnotationSelectableObject>(target);
                if (annotObj && m_doc) {
                    auto cmd = std::make_unique<pdf_engine::commands::DeleteAnnotationCommand>(
                        m_doc.get(), annotObj->GetPageIndex(), annotObj->GetAnnotation()
                    );
                    m_doc->GetCommandStack().ExecuteCommand(std::move(cmd));
                    m_interactionManager.RemoveObject(annotObj->GetId());
                    m_generation++;
                    core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                    ReloadInteractableObjects();
                    InvalidateView();
                }
            }
        }
    } else if (wmId >= 3030 && wmId <= 3036) { // Page commands
        if (wmId == 3034) { // PageZoomIn
            OnZoom(1.2, m_bounds.left + (m_bounds.right - m_bounds.left) / 2.0f, m_bounds.top + (m_bounds.bottom - m_bounds.top) / 2.0f);
        } else if (wmId == 3035) { // PageZoomOut
            OnZoom(0.8, m_bounds.left + (m_bounds.right - m_bounds.left) / 2.0f, m_bounds.top + (m_bounds.bottom - m_bounds.top) / 2.0f);
        } else if (wmId == 3032 || wmId == 3033) { // Rotate Cw or Ccw
            int pageIndex = GetCurrentPage();
            auto* pdfDoc = dynamic_cast<PdfDocument*>(m_doc.get());
            if (pageIndex >= 0 && pdfDoc) {
                int delta = (wmId == 3032) ? 90 : -90;
                auto cmd = std::make_unique<pdf_engine::commands::RotatePageCommand>(pdfDoc, pageIndex, delta);
                pdfDoc->GetCommandStack().ExecuteCommand(std::move(cmd));
                m_generation++;
                core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                CachePageSizes();
                UpdateVisibleTiles();
                ReloadInteractableObjects();
                InvalidateView();
            }
        } else if (wmId == 3030) { // Insert Blank Page
            int pageIndex = GetCurrentPage();
            auto* pdfDoc = dynamic_cast<PdfDocument*>(m_doc.get());
            if (pageIndex >= 0 && pdfDoc) {
                auto cmd = std::make_unique<pdf_engine::commands::InsertBlankPageCommand>(pdfDoc, pageIndex + 1, 612.0, 792.0);
                pdfDoc->GetCommandStack().ExecuteCommand(std::move(cmd));
                m_generation++;
                core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                CachePageSizes();
                UpdateVisibleTiles();
                ReloadInteractableObjects();
                InvalidateView();
            }
        } else if (wmId == 3031) { // Delete Page
            int pageIndex = GetCurrentPage();
            auto* pdfDoc = dynamic_cast<PdfDocument*>(m_doc.get());
            if (pageIndex >= 0 && pdfDoc && pdfDoc->PageCount() > 1) {
                auto cmd = std::make_unique<pdf_engine::commands::DeletePageCommand>(pdfDoc, pageIndex);
                pdfDoc->GetCommandStack().ExecuteCommand(std::move(cmd));
                m_generation++;
                core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                if (m_currentPage >= pdfDoc->PageCount()) {
                    m_currentPage = pdfDoc->PageCount() - 1;
                }
                CachePageSizes();
                UpdateVisibleTiles();
                ReloadInteractableObjects();
                InvalidateView();
            }
        } else if (wmId == 3036) { // Select All
            int pageIndex = GetCurrentPage();
            if (pageIndex >= 0) {
                auto* tp = GetTextPage(pageIndex);
                if (tp) {
                    m_selection.startPage = pageIndex;
                    m_selection.endPage = pageIndex;
                    m_selection.startChar = 0;
                    m_selection.endChar = tp->CountChars() - 1;
                    InvalidateView();
                }
            }
        }
    }
}
void PdfViewer::OnChar(unsigned __int64 wParam) {
    if (m_inputRouter) {
        ui::input::KeyEvent ke;
        ke.charCode = static_cast<wchar_t>(wParam);
        ke.isDown = true;
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) ke.modifiers |= ui::input::KeyModifier::Shift;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) ke.modifiers |= ui::input::KeyModifier::Control;
        if (GetAsyncKeyState(VK_MENU) & 0x8000) ke.modifiers |= ui::input::KeyModifier::Alt;
        ke.timestampMs = ::GetTickCount64();

        auto res = m_inputRouter->RouteChar(ke);
        if (res == ui::input::EventResult::Handled || res == ui::input::EventResult::Consumed) {
            InvalidateView();
            return;
        }
    }

    if (m_currentTool == ToolMode::EditText) {
        if (m_interactionManager.OnChar(wParam)) {
            InvalidateView();
        }
    }
}
void PdfViewer::OnKeyDown(unsigned __int64 wParam) {
    if (m_inputRouter) {
        ui::input::KeyEvent ke;
        ke.virtualKey = static_cast<uint32_t>(wParam);
        ke.isDown = true;
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) ke.modifiers |= ui::input::KeyModifier::Shift;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) ke.modifiers |= ui::input::KeyModifier::Control;
        if (GetAsyncKeyState(VK_MENU) & 0x8000) ke.modifiers |= ui::input::KeyModifier::Alt;
        ke.timestampMs = ::GetTickCount64();

        auto res = m_inputRouter->RouteKeyDown(ke);
        if (res == ui::input::EventResult::Handled || res == ui::input::EventResult::Consumed) {
            InvalidateView();
            return;
        }
    }

    if (m_activeHandler) {
        bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;        if (m_activeHandler->OnKeyDown(static_cast<int>(wParam), shiftPressed, ctrlPressed, altPressed)) {
            InvalidateView();
            return;
        }
    }

    if (wParam == VK_ESCAPE) {
        if (m_isInsertingImage) {
            m_isInsertingImage = false;
            m_pendingImageData.clear();
        }
        SetToolMode(ToolMode::Select);
        ui::commands::CommandManager::Instance().ExecuteAction(L"Select");
        InvalidateView();
        return;
    }

    if (m_currentTool == ToolMode::EditText) {
        bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        if (m_interactionManager.OnKeyDown(wParam, shiftPressed, ctrlPressed)) {
            InvalidateView();
        }
    }
}

void PdfViewer::CopySelection() {
    if (m_selection.startPage >= 0 && m_selection.startChar >= 0 && m_selection.endChar >= 0) {
        int start = std::min(m_selection.startChar, m_selection.endChar);
        int end = std::max(m_selection.startChar, m_selection.endChar);
        int count = end - start + 1;
        auto* tp = GetTextPage(m_selection.startPage);
        if (tp && count > 0) {
            std::wstring text = tp->GetText(start, count);
            if (!text.empty() && OpenClipboard(m_hwnd)) {
                EmptyClipboard();
                size_t byteSize = (text.length() + 1) * sizeof(wchar_t);
                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, byteSize);
                if (hMem) {
                    memcpy(GlobalLock(hMem), text.c_str(), byteSize);
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_UNICODETEXT, hMem);
                }
                CloseClipboard();
            }
        }
        return;
    }
    
    // Check InteractionManager selection (Text objects, Images, etc)
    auto sel = m_interactionManager.GetSelection();
    std::wstring textToCopy;
    bool copiedImage = false;
    for (auto& obj : sel) {
        if (auto textObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(obj)) {
            if (auto t = textObj->GetTextObject()) {
                std::wstring text = t->GetText();
                if (!text.empty()) {
                    if (!textToCopy.empty()) textToCopy += L"\n";
                    textToCopy += text;
                }
            }
        } else if (!copiedImage) {
            if (auto imgObj = std::dynamic_pointer_cast<ui::interaction::ImageSelectableObject>(obj)) {
                if (auto img = imgObj->GetImage()) {
                    auto bitmapData = img->GetBitmapData();
                    int width = img->GetWidth();
                    int height = img->GetHeight();
                    if (width > 0 && height > 0 && !bitmapData.empty()) {
                        if (OpenClipboard(m_hwnd)) {
                            EmptyClipboard();
                            size_t headerSize = sizeof(BITMAPINFOHEADER);
                            size_t dataSize = width * height * 4;
                            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, headerSize + dataSize);
                            if (hMem) {
                                uint8_t* mem = (uint8_t*)GlobalLock(hMem);
                                BITMAPINFOHEADER* bmi = (BITMAPINFOHEADER*)mem;
                                bmi->biSize = sizeof(BITMAPINFOHEADER);
                                bmi->biWidth = width;
                                bmi->biHeight = -height;
                                bmi->biPlanes = 1;
                                bmi->biBitCount = 32;
                                bmi->biCompression = BI_RGB;
                                bmi->biSizeImage = static_cast<DWORD>(dataSize);
                                memcpy(mem + headerSize, bitmapData.data(), dataSize);
                                GlobalUnlock(hMem);
                                SetClipboardData(CF_DIB, hMem);
                                copiedImage = true;
                            }
                            CloseClipboard();
                        }
                    }
                }
            }
        }
    }
    
    if (!textToCopy.empty() && OpenClipboard(m_hwnd)) {
        EmptyClipboard();
        size_t byteSize = (textToCopy.length() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, byteSize);
        if (hMem) {
            memcpy(GlobalLock(hMem), textToCopy.c_str(), byteSize);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        CloseClipboard();
    }
}

void PdfViewer::OnRButtonUp(float x, float y) {
    if (m_currentTool != ToolMode::Select && m_currentTool != ToolMode::EditText) return;

    struct ScopedRightClick { 
        bool& flag; 
        ScopedRightClick(bool& f) : flag(f) { flag = true; } 
        ~ScopedRightClick() { flag = false; } 
    } scope(m_isRightClickProcessing);

    POINT pt{static_cast<LONG>(x), static_cast<LONG>(y)};
    if (m_hwnd) {
        ClientToScreen(m_hwnd, &pt);
    }

    ui::menu::ContextMenuInfo info;
    info.targetType = ui::menu::TargetType::PageCanvas;
    
    std::shared_ptr<ui::interaction::ISelectableObject> targetObj = nullptr;

    // 1. Check if we right-clicked a specific object directly
    if (m_interactionManager.viewToPage) {
        double px, py;
        int pageIndex = -1;
        m_interactionManager.viewToPage(x, y, px, py, pageIndex);
        if (pageIndex >= 0) {
            targetObj = m_interactionManager.HitTestObjects(px, py, pageIndex);
        }
    }

    // 2. If we didn't hit a specific object, but we have exactly 1 object selected, use the selection
    if (!targetObj) {
        auto selection = m_interactionManager.GetSelection();
        if (selection.size() == 1) {
            targetObj = selection[0];
        }
    }

    // 3. Populate info based on targetObj
    if (targetObj) {
        if (auto imgObj = std::dynamic_pointer_cast<ui::interaction::ImageSelectableObject>(targetObj)) {
            info.targetType = ui::menu::TargetType::ImageObject;
            info.hasSelection = true;
            info.pageIndex = imgObj->GetPageIndex();
        } else if (auto txtObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(targetObj)) {
            info.targetType = ui::menu::TargetType::TextObject;
            info.hasSelection = true;
            info.pageIndex = txtObj->GetPageIndex();
            if (auto domText = txtObj->GetTextObject()) {
                info.selectedText = domText->GetText();
            }
        } else if (auto annotObj = std::dynamic_pointer_cast<ui::interaction::AnnotationSelectableObject>(targetObj)) {
            info.targetType = ui::menu::TargetType::Annotation;
            info.hasSelection = true;
            info.pageIndex = annotObj->GetPageIndex();
        }
    } else if (m_selection.startPage >= 0 && m_selection.startChar >= 0 && m_selection.endChar >= 0) {
        // 4. If no object, check if we have text selected
        info.targetType = ui::menu::TargetType::TextSelection;
        info.hasSelection = true;
        info.pageIndex = m_selection.startPage;
        int start = std::min(m_selection.startChar, m_selection.endChar);
        int end = std::max(m_selection.startChar, m_selection.endChar);
        int count = end - start + 1;
        auto* tp = GetTextPage(m_selection.startPage);
        if (tp && count > 0) {
            info.selectedText = tp->GetText(start, count);
        }
    }

    if (m_doc) {
        info.canUndo = m_doc->GetCommandStack().CanUndo();
        info.canRedo = m_doc->GetCommandStack().CanRedo();
    }

    m_contextMenuTarget = targetObj;
    ui::menu::ContextMenuManager::Instance().ShowContextMenu(m_hwnd, pt, info);
}



void PdfViewer::CancelActiveInteractions() {
    if (m_inputRouter) {
        m_inputRouter->OnCaptureLost();
    }
    if (m_activeHandler) {
        m_activeHandler->Cancel();
    }
    m_isDrawingInk = false;
    m_inkStroke.clear();
    m_inkPageIndex = -1;
    m_isCreatingAnnotation = false;
    m_createAnnotationPage = -1;
    m_isInsertingImage = false;
    m_pendingImageData.clear();
    m_isPlacingStickyNote = false;
    m_isPanning = false;
    m_isMidPanning = false;
    if (m_hwnd && GetCapture() == m_hwnd) ReleaseCapture();
    m_interactionManager.CommitTextEdit();
}

bool PdfViewer::OnSetCursor() {
    HCURSOR hCursor = nullptr;

    // Highest priority: middle-mouse panning always shows hand cursor
    if (m_isMidPanning) {
        if ((GetAsyncKeyState(VK_MBUTTON) & 0x8000) == 0) {
            m_isMidPanning = false;
        } else {
            SetCursor(LoadCursor(nullptr, IDC_HAND));
            return true;
        }
    }

    // 0. Active tool state machine query (unconditional for all tools)
    if (m_toolStateMachine && m_toolStateMachine->GetActiveTool()) {
        POINT pt;
        GetCursorPos(&pt);
        if (m_hwnd) ScreenToClient(m_hwnd, &pt);
        float scale = m_hwnd ? (GetDpiForWindow(m_hwnd) / 96.0f) : 1.0f;
        if (scale <= 0.0f) scale = 1.0f;
        PointF dipPt = { static_cast<float>(pt.x) / scale, static_cast<float>(pt.y) / scale };
        hCursor = m_toolStateMachine->GetCursor(dipPt);
        if (hCursor) {
            SetCursor(hCursor);
            return true;
        }
    }

    // 1. Active annotation handler has priority
    if (m_activeHandler) {
        POINT pt;
        GetCursorPos(&pt);
        if (m_hwnd) ScreenToClient(m_hwnd, &pt);
        hCursor = m_activeHandler->OnSetCursor(PointF{static_cast<float>(pt.x), static_cast<float>(pt.y)});
        if (hCursor) {
            SetCursor(hCursor);
            return true;
        }
    }

    // 2. Active edit or drag/resize handle has highest priority (managed by InteractionManager)
    if (m_currentTool == ToolMode::Select || m_currentTool == ToolMode::EditText || m_currentTool == ToolMode::AddText) {
        hCursor = m_interactionManager.GetCursor();
    }
    
    // 3. Active panning overrides hover
    if (!hCursor && m_currentTool == ToolMode::Pan) {
        hCursor = LoadCursor(nullptr, m_isPanning ? IDC_SIZEALL : IDC_HAND);
    }

    // 4. Active ink drawing overrides hover
    if (!hCursor && m_currentTool == ToolMode::Ink && m_isDrawingInk) {
        hCursor = LoadCursor(nullptr, IDC_CROSS);
    }
    
    // 5. Active shape creation overrides hover
    if (!hCursor && m_isCreatingAnnotation) {
        hCursor = LoadCursor(nullptr, IDC_CROSS);
    }

    // 6. Text Hover (Content Aware)
    if (!hCursor && m_interactionManager.viewToPage) {
        POINT pt;
        GetCursorPos(&pt);
        if (m_hwnd) ScreenToClient(m_hwnd, &pt);
        
        double px = 0.0, py = 0.0;
        int pageIndex = -1;
        float scale = m_hwnd ? (GetDpiForWindow(m_hwnd) / 96.0f) : 1.0f;
        if (scale <= 0.0f) scale = 1.0f;
        m_interactionManager.viewToPage(pt.x / scale, pt.y / scale, px, py, pageIndex);
        
        if (pageIndex >= 0) {
            auto* tp = GetTextPage(pageIndex);
            if (tp) {
                int charIndex = tp->GetCharIndexAtPos(px, py, 5.0, 5.0);
                if (charIndex >= 0) {
                    hCursor = LoadCursor(nullptr, IDC_IBEAM);
                }
            }
        }
    }
    
    // 6. Object Hover (Content Aware for Select Mode)
    if (!hCursor && (m_currentTool == ToolMode::Select || m_currentTool == ToolMode::EditText)) {
        if (m_interactionManager.IsHoveringObject()) {
            hCursor = LoadCursor(nullptr, IDC_SIZEALL);
        }
    }
    
    // 7. Fallback to standard tool cursor
    if (!hCursor) {
        switch (m_currentTool) {
            case ToolMode::Ink:
            case ToolMode::Rectangle:
            case ToolMode::Ellipse:
            case ToolMode::Line:
            case ToolMode::Arrow:
            case ToolMode::Stamp:
                hCursor = LoadCursor(nullptr, IDC_CROSS);
                break;
            case ToolMode::Highlight:
            case ToolMode::Underline:
            case ToolMode::Strikeout:
            case ToolMode::AddText:
            case ToolMode::EditText:
            case ToolMode::StickyNote:
                // Note: I-Beam is shown over text, but crosshair/I-beam usually everywhere for these tools
                hCursor = LoadCursor(nullptr, IDC_ARROW);
                break;
            case ToolMode::Eraser:
                hCursor = LoadCursor(nullptr, IDC_NO);
                break;
            case ToolMode::Select:
            default:
                hCursor = LoadCursor(nullptr, IDC_ARROW);
                break;
        }
    }
    
    if (hCursor) {
        SetCursor(hCursor);
        return true;
    }
    return false;
}

void PdfViewer::OnMouseMove(float x, float y) {
    if (m_isDraggingHScrollbar) {
        float maxLayoutWidth = 0.0f;
        for (const auto& layout : m_layout) {
            if (layout.width > maxLayoutWidth) maxLayoutWidth = layout.width;
        }
        float width = m_bounds.right - m_bounds.left;
        if (maxLayoutWidth > width && width > 0) {
            float scrollbarWidth = std::max(40.0f, (width / maxLayoutWidth) * width);
            float scrollTrackWidth = width - scrollbarWidth;
            if (scrollTrackWidth > 0) {
                float targetX = (x - m_bounds.left) - m_hScrollbarDragOffsetX;
                targetX = std::max(0.0f, std::min(targetX, scrollTrackWidth));
                m_scrollX = static_cast<int>((targetX / scrollTrackWidth) * (maxLayoutWidth - width));
                m_scrollX = std::max(0, static_cast<int>(std::min(static_cast<float>(m_scrollX), maxLayoutWidth - width)));
                UpdateVisibleTiles();
                InvalidateView();
            }
        }
        return;
    }
    if (m_isDraggingScrollbar) {
        float totalHeight = 0;
        if (!m_layout.empty()) {
            totalHeight = m_layout.back().yOffset + m_layout.back().height;
        }
        float height = m_bounds.bottom - m_bounds.top;
        if (totalHeight > height && height > 0) {
            float scrollbarHeight = std::max(40.0f, (height / totalHeight) * height);
            float scrollTrackHeight = height - scrollbarHeight;
            if (scrollTrackHeight > 0) {
                float targetY = (y - m_bounds.top) - m_scrollbarDragOffsetY;
                targetY = std::max(0.0f, std::min(targetY, scrollTrackHeight));
                m_scrollY = (targetY / scrollTrackHeight) * (totalHeight - height);
                m_scrollY = std::max(0.0f, std::min(m_scrollY, totalHeight - height));
                UpdateVisibleTiles();
                InvalidateView();
            }
        }
        return;
    }

    // Middle-mouse panning takes highest priority â€” always handle it first
    if (m_isMidPanning) {
        float dx = x - m_midPanStartPt.x;
        float dy = y - m_midPanStartPt.y;

        float viewHeight = m_bounds.bottom - m_bounds.top;
        float totalHeight = 0.0f;
        if (!m_layout.empty()) {
            totalHeight = m_layout.back().yOffset + m_layout.back().height;
        }
        float maxScrollY = std::max(0.0f, totalHeight - viewHeight);

        m_scrollY = std::max(0.0f, std::min(m_midPanStartScroll.y - dy, maxScrollY));
        m_scrollX = std::max(0, static_cast<int>(m_midPanStartScroll.x - dx));

        UpdateVisibleTiles();
        InvalidateView();
        return;
    }

    if (m_inputRouter) {
        auto pe = CreatePointerEvent(ui::input::PointerEventType::Move, x, y, ui::input::PointerButton::None);
        auto res = m_inputRouter->RoutePointerMove(pe);
        if (res == ui::input::EventResult::Handled || res == ui::input::EventResult::Consumed) {
            return;
        }
    }

    if (m_currentTool == ToolMode::Pan && m_isPanning) {
        float dx = x - m_panStartPt.x;
        float dy = y - m_panStartPt.y;
        
        float viewHeight = m_bounds.bottom - m_bounds.top;
        float totalHeight = 0.0f;
        if (!m_layout.empty()) {
            totalHeight = m_layout.back().yOffset + m_layout.back().height;
        }
        float maxScrollY = std::max(0.0f, totalHeight - viewHeight);
        
        m_scrollY = std::max(0.0f, std::min(m_panStartScroll.y - dy, maxScrollY));
        m_scrollX = std::max(0, static_cast<int>(m_panStartScroll.x - dx));
        
        UpdateVisibleTiles();
        InvalidateView();
        return;
    }

    if (m_activeHandler) {
        ui::annotation::MouseEvent me;
        me.viewX = x;
        me.viewY = y;
        me.shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        me.ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        me.alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        if (m_interactionManager.viewToPage) {
            m_interactionManager.viewToPage(x, y, me.pdfX, me.pdfY, me.pageIndex);
        }
        if (m_activeHandler->OnMouseMove(me)) {
            return;
        }
    }

    // Ink: accumulate stroke points (fallback)
    if (m_currentTool == ToolMode::Ink && m_isDrawingInk && m_inkPageIndex >= 0) {
        double px = 0.0, py = 0.0; int pageIndex = -1;
        if (m_interactionManager.viewToPage) {
            m_interactionManager.viewToPage(x, y, px, py, pageIndex);
        }
        if (pageIndex == m_inkPageIndex) {
            m_inkStroke.push_back({static_cast<float>(px), static_cast<float>(py)});
            InvalidateView();
        }
        return;
    }

    // Shape creation preview (fallback)
    if (m_isCreatingAnnotation && m_createAnnotationPage >= 0) {
        double px = 0.0, py = 0.0; int pageIndex = -1;
        if (m_interactionManager.viewToPage) {
            m_interactionManager.viewToPage(x, y, px, py, pageIndex);
        }
        m_createAnnotationCurrentPdf = {static_cast<float>(px), static_cast<float>(py)};
        InvalidateView();
        return;
    }

    if (m_selection.isSelecting) {
        double px = 0.0, py = 0.0; int pageIndex = -1;
        if (m_interactionManager.viewToPage) {
            m_interactionManager.viewToPage(x, y, px, py, pageIndex);
        }
        if (pageIndex >= 0) {
            auto* tp = GetTextPage(pageIndex);
            if (tp) {
                int charIndex = tp->GetCharIndexAtPos(px, py, 5.0, 5.0);
                if (charIndex >= 0) {
                    if (m_selection.endPage != pageIndex || m_selection.endChar != charIndex) {
                        m_selection.endPage = pageIndex;
                        m_selection.endChar = charIndex;
                        InvalidateView();
                    }
                }
            }
        }
    }

    if (m_currentTool == ToolMode::EditText || m_currentTool == ToolMode::Select) {
        if (m_interactionManager.OnMouseMove(x, y)) {
            InvalidateView();
        }
    }
}

void PdfViewer::OnLButtonUp(float x, float y) {
    if (m_isDraggingHScrollbar) {
        m_isDraggingHScrollbar = false;
        if (m_hwnd) ReleaseCapture();
        return;
    }
    if (m_isDraggingScrollbar) {
        m_isDraggingScrollbar = false;
        if (m_hwnd) ReleaseCapture();
        return;
    }

    if (m_inputRouter) {
        auto pe = CreatePointerEvent(ui::input::PointerEventType::Up, x, y, ui::input::PointerButton::Left);
        auto res = m_inputRouter->RoutePointerUp(pe);
        if (res == ui::input::EventResult::Handled || res == ui::input::EventResult::Consumed) {
            return;
        }
    }

    if (m_currentTool == ToolMode::Pan && m_isPanning) {
        m_isPanning = false;
        if (m_hwnd && GetCapture() == m_hwnd) ReleaseCapture();
        return;
    }

    if (m_activeHandler) {
        ui::annotation::MouseEvent me;
        me.viewX = x;
        me.viewY = y;
        me.shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        me.ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        me.alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        if (m_interactionManager.viewToPage) {
            m_interactionManager.viewToPage(x, y, me.pdfX, me.pdfY, me.pageIndex);
        }
        if (m_activeHandler->OnMouseUp(me)) {
            return;
        }
    }

    // Commit ink stroke (fallback)
    if (m_currentTool == ToolMode::Ink && m_isDrawingInk) {
        m_isDrawingInk = false;
        if (m_hwnd && GetCapture() == m_hwnd) ReleaseCapture();
        if (m_doc && m_inkPageIndex >= 0 && m_inkStroke.size() >= 2) {
            auto addCmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
                m_doc.get(), m_inkPageIndex,
                core::interfaces::dom::AnnotationType::Ink,
                RectF{0, 0, 0, 0}
            );
            if (m_doc->GetCommandStack().ExecuteCommand(std::move(addCmd))) {
                m_generation++;
                core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                ReloadInteractableObjects();
                InvalidateView();
            }
        }
        m_inkStroke.clear();
        m_inkPageIndex = -1;
        return;
    }

    // Commit shape annotation (fallback)
    if (m_isCreatingAnnotation && m_createAnnotationPage >= 0) {
        m_isCreatingAnnotation = false;
        if (m_hwnd && GetCapture() == m_hwnd) ReleaseCapture();

        float x0 = std::min(m_createAnnotationStartPdf.x, m_createAnnotationCurrentPdf.x);
        float y0 = std::min(m_createAnnotationStartPdf.y, m_createAnnotationCurrentPdf.y);
        float x1 = std::max(m_createAnnotationStartPdf.x, m_createAnnotationCurrentPdf.x);
        float y1 = std::max(m_createAnnotationStartPdf.y, m_createAnnotationCurrentPdf.y);

        if (x1 - x0 < 4.0f) x1 = x0 + 4.0f;
        if (y1 - y0 < 4.0f) y1 = y0 + 4.0f;

        core::interfaces::dom::AnnotationType annType = core::interfaces::dom::AnnotationType::Square;
        if (m_currentTool == ToolMode::Ellipse)   annType = core::interfaces::dom::AnnotationType::Circle;
        else if (m_currentTool == ToolMode::Line)  annType = core::interfaces::dom::AnnotationType::Line;
        else if (m_currentTool == ToolMode::Arrow) annType = core::interfaces::dom::AnnotationType::Line;

        if (m_doc) {
            auto cmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
                m_doc.get(), m_createAnnotationPage, annType, RectF{x0, y0, x1, y1}
            );
            bool ok = m_doc->GetCommandStack().ExecuteCommand(std::move(cmd));
            if (ok) {
                m_generation++;
                core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                ReloadInteractableObjects();
                InvalidateView();
            }
        }
        m_createAnnotationPage = -1;
        return;
    }

    if (m_selection.isSelecting) {
        m_selection.isSelecting = false;
        if (m_hwnd && GetCapture() == m_hwnd) ReleaseCapture();
        InvalidateView();
    }

    if (m_currentTool == ToolMode::EditText || m_currentTool == ToolMode::Select) {
        if (m_interactionManager.OnLButtonUp(x, y)) {
            InvalidateView();
        }
        if (m_hwnd && GetCapture() == m_hwnd) {
            ReleaseCapture();
        }
    }
}

void PdfViewer::OnLButtonDoubleClick(float x, float y) {
    if (m_inputRouter) {
        auto pe = CreatePointerEvent(ui::input::PointerEventType::DoubleClick, x, y, ui::input::PointerButton::Left);
        auto res = m_inputRouter->RoutePointerDoubleClick(pe);
        if (res == ui::input::EventResult::Handled || res == ui::input::EventResult::Consumed) {
            return;
        }
    }

    if (m_currentTool == ToolMode::EditText) {
        auto hit = m_interactionManager.OnLButtonDown(x, y, false);
        if (hit != ui::interaction::HitResult::None) {
            auto sel = m_interactionManager.GetSelection();
            if (sel.size() == 1) {
                if (auto textSel = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(sel[0])) {
                    SetToolMode(ToolMode::EditText);
                    m_interactionManager.EnterTextEditMode(textSel);
                    InvalidateView();
                }
            }
        }
    }
}

void PdfViewer::OnMButtonDown(float x, float y) {
    m_isMidPanning = true;
    m_midPanStartPt = { x, y };
    m_midPanStartScroll = { static_cast<float>(m_scrollX), m_scrollY };
    if (m_hwnd) SetCapture(m_hwnd);
    // Force cursor update immediately
    InvalidateView();
}

void PdfViewer::OnMButtonUp(float x, float y) {
    (void)x; (void)y;
    if (m_isMidPanning) {
        m_isMidPanning = false;
        if (m_hwnd && GetCapture() == m_hwnd) ReleaseCapture();
        // Force WM_SETCURSOR to re-evaluate with normal tool cursor
        InvalidateView();
        // Post a fake mouse move to let WM_SETCURSOR re-fire
    if (m_hwnd) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(m_hwnd, &pt);
            PostMessage(m_hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(pt.x, pt.y));
        }
    }
}

void PdfViewer::OnLButtonDown(float x, float y) {
    if (m_hwnd) {
        ::SetFocus(m_hwnd);
    }

    float maxLayoutWidth = 0.0f;
    for (const auto& layout : m_layout) {
        if (layout.width > maxLayoutWidth) maxLayoutWidth = layout.width;
    }
    float viewWidth = m_bounds.right - m_bounds.left;
    if (maxLayoutWidth > viewWidth && viewWidth > 0) {
        float scrollbarWidth = std::max(40.0f, (viewWidth / maxLayoutWidth) * viewWidth);
        float scrollbarX = (m_scrollX / maxLayoutWidth) * viewWidth;
        D2D1_RECT_F hScrollbarRect = D2D1::RectF(
            m_bounds.left + scrollbarX,
            m_bounds.bottom - 12.0f,
            m_bounds.left + scrollbarX + scrollbarWidth,
            m_bounds.bottom - 4.0f
        );
        if (y >= hScrollbarRect.top - 4.0f && y <= m_bounds.bottom && x >= m_bounds.left && x <= m_bounds.right) {
            if (x >= hScrollbarRect.left && x <= hScrollbarRect.right) {
                m_isDraggingHScrollbar = true;
                m_hScrollbarDragOffsetX = x - hScrollbarRect.left;
            } else {
                if (x < hScrollbarRect.left) m_scrollX = static_cast<int>(m_scrollX - viewWidth * 0.8f);
                else m_scrollX = static_cast<int>(m_scrollX + viewWidth * 0.8f);
                m_scrollX = std::max(0, static_cast<int>(std::min(static_cast<float>(m_scrollX), maxLayoutWidth - viewWidth)));
                UpdateVisibleTiles();
                InvalidateView();
            }
            if (m_hwnd) SetCapture(m_hwnd);
            return;
        }
    }

    float totalHeight = 0;
    if (!m_layout.empty()) {
        totalHeight = m_layout.back().yOffset + m_layout.back().height;
    }
    float height = m_bounds.bottom - m_bounds.top;
    if (totalHeight > height && height > 0) {
        float scrollbarHeight = std::max(40.0f, (height / totalHeight) * height);
        float scrollbarY = (m_scrollY / totalHeight) * height;
        D2D1_RECT_F scrollbarRect = D2D1::RectF(
            m_bounds.right - 12.0f,
            m_bounds.top + scrollbarY,
            m_bounds.right - 4.0f,
            m_bounds.top + scrollbarY + scrollbarHeight
        );
        if (x >= scrollbarRect.left - 4.0f && x <= m_bounds.right && y >= m_bounds.top && y <= m_bounds.bottom) {
            if (y >= scrollbarRect.top && y <= scrollbarRect.bottom) {
                m_isDraggingScrollbar = true;
                m_scrollbarDragOffsetY = y - scrollbarRect.top;
            } else {
                if (y < scrollbarRect.top) m_scrollY -= height * 0.8f;
                else m_scrollY += height * 0.8f;
                m_scrollY = std::max(0.0f, std::min(m_scrollY, totalHeight - height));
                InvalidateView();
            }
            if (m_hwnd) SetCapture(m_hwnd);
            return;
        }
    }

    if (m_currentTool == ToolMode::EditText || m_currentTool == ToolMode::Select) {
        if (m_interactionManager.HandleContextualToolbarHit(x, y)) {
            InvalidateView();
            return;
        }
    }

    if (m_inputRouter) {
        auto pe = CreatePointerEvent(ui::input::PointerEventType::Down, x, y, ui::input::PointerButton::Left);
        auto res = m_inputRouter->RoutePointerDown(pe);
        if (res == ui::input::EventResult::Handled || res == ui::input::EventResult::Consumed) {
            return;
        }
    }

    if (m_currentTool == ToolMode::Pan) {
        m_isPanning = true;
        m_panStartPt = { x, y };
        m_panStartScroll = { static_cast<float>(m_scrollX), m_scrollY };
        if (m_hwnd) SetCapture(m_hwnd);
        return;
    }

    if (m_isInsertingImage && !m_pendingImageData.empty()) {
        double px = 0.0, py = 0.0;
        int pageIndex = -1;
        if (m_interactionManager.viewToPage) {
            m_interactionManager.viewToPage(x, y, px, py, pageIndex);
        }
        if (pageIndex >= 0) {
            auto cmd = std::make_unique<pdf_engine::commands::InsertImageCommand>(
                m_doc.get(), pageIndex, m_pendingImageData, m_pendingImageWidth, m_pendingImageHeight,
                RectF{(float)px, (float)py, (float)px + 100.f, (float)py + 100.f}
            );
            m_doc->GetCommandStack().ExecuteCommand(std::move(cmd));
            m_generation++;
            core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
            ReloadInteractableObjects();
            InvalidateView();
        }
        m_isInsertingImage = false;
        m_pendingImageData.clear();
        return;
    }

    if (m_currentTool == ToolMode::AddText) {
        m_interactionManager.EnterNewTextMode(x, y);
        InvalidateView();
        return;
    }

    if (m_activeHandler) {
        ui::annotation::MouseEvent me;
        me.viewX = x;
        me.viewY = y;
        me.shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        me.ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        me.alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        me.leftButton = true;
        if (m_interactionManager.viewToPage) {
            m_interactionManager.viewToPage(x, y, me.pdfX, me.pdfY, me.pageIndex);
        }
        if (m_activeHandler->OnMouseDown(me)) {
            return;
        }
    }

    // Stamp â€” place a Stamp annotation with the selected label in its Contents
    if (m_currentTool == ToolMode::Stamp) {
        double px = 0.0, py = 0.0;
        int pageIndex = -1;
        if (m_interactionManager.viewToPage) {
            m_interactionManager.viewToPage(x, y, px, py, pageIndex);
        }
        if (pageIndex >= 0 && m_doc) {
            float stampW = 80.f, stampH = 24.f;
            RectF stampBounds{static_cast<float>(px) - stampW / 2.f,
                              static_cast<float>(py) - stampH / 2.f,
                              static_cast<float>(px) + stampW / 2.f,
                              static_cast<float>(py) + stampH / 2.f};
            auto cmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
                m_doc.get(), pageIndex,
                core::interfaces::dom::AnnotationType::Stamp,
                stampBounds
            );
            auto* cmdRaw = cmd.get();
            if (m_doc->GetCommandStack().ExecuteCommand(std::move(cmd))) {
                if (!m_pendingStampLabel.empty()) {
                    auto addedAnnot = cmdRaw->GetAnnotation();
                    if (addedAnnot) {
                        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, m_pendingStampLabel.c_str(), -1, nullptr, 0, nullptr, nullptr);
                        std::string label(utf8Len > 0 ? utf8Len - 1 : 0, '\0');
                        if (utf8Len > 0) WideCharToMultiByte(CP_UTF8, 0, m_pendingStampLabel.c_str(), -1, label.data(), utf8Len, nullptr, nullptr);
                        addedAnnot->SetContents(label);
                    }
                }
                m_generation++;
                core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                ReloadInteractableObjects();
                InvalidateView();
            }
        }
        return;
    }

    // Eraser â€” delete the annotation or ink stroke under the click
    if (m_currentTool == ToolMode::Eraser) {
        double px = 0.0, py = 0.0;
        int pageIndex = -1;
        if (m_interactionManager.viewToPage) {
            m_interactionManager.viewToPage(x, y, px, py, pageIndex);
        }
        if (pageIndex >= 0 && m_doc) {
            const auto& objects = m_interactionManager.GetObjects();
            for (auto& obj : objects) {
                if (obj->GetPageIndex() != pageIndex) continue;
                auto annotObj = std::dynamic_pointer_cast<ui::interaction::AnnotationSelectableObject>(obj);
                if (annotObj) {
                    auto annot = annotObj->GetAnnotation();
                    if (annot && annotObj->HitTest(px, py)) {
                        auto cmd = std::make_unique<pdf_engine::commands::DeleteAnnotationCommand>(
                            m_doc.get(), pageIndex, annot
                        );
                        if (m_doc->GetCommandStack().ExecuteCommand(std::move(cmd))) {
                            m_interactionManager.RemoveObject(obj->GetId());
                            m_generation++;
                            core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
                            InvalidateView();
                        }
                        return;
                    }
                }
            }
        }
        return;
    }

    if (m_currentTool == ToolMode::EditText || m_currentTool == ToolMode::Select) {
        auto hit = m_interactionManager.OnLButtonDown(x, y, (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
        if (hit != ui::interaction::HitResult::None) {
            m_selection.isSelecting = false;
            if (m_hwnd) SetCapture(m_hwnd);
            InvalidateView();
            return;
        } else {
            // Check for text hit before falling back to marquee
            double px = 0, py = 0; int pageIndex = -1;
            if (m_interactionManager.viewToPage) {
                m_interactionManager.viewToPage(x, y, px, py, pageIndex);
            }
            int charIndex = -1;
            if (pageIndex >= 0) {
                auto* tp = GetTextPage(pageIndex);
                if (tp) charIndex = tp->GetCharIndexAtPos(px, py, 5.0, 5.0);
            }
            if (charIndex >= 0 && m_currentTool == ToolMode::Select) {
                m_selection.isSelecting = true;
                m_selection.startPage = pageIndex;
                m_selection.startChar = charIndex;
                m_selection.endPage = pageIndex;
                m_selection.endChar = charIndex;
                m_interactionManager.GetSelectionModel().Clear();
                if (m_hwnd) SetCapture(m_hwnd);
                InvalidateView();
            } else {
                m_selection.startChar = -1;
                m_selection.endChar = -1;
                m_selection.isSelecting = false;
                m_interactionManager.StartMarquee(x, y, (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
                if (m_hwnd) SetCapture(m_hwnd);
                InvalidateView();
            }
        }        // If clicking on text, allow editing
        if (m_currentTool == ToolMode::EditText) {
            auto sel = m_interactionManager.GetSelection();
            if (sel.size() == 1) {
                if (auto textSel = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(sel[0])) {
                    m_interactionManager.EnterTextEditMode(textSel);
                    InvalidateView();
                }
            }
        }
    }
}

void PdfViewer::SelectAllText() {
    auto tool = GetToolStateMachine()->GetTool(ui::tools::ToolType::TextSelect);
    if (!tool) return;
    auto textTool = dynamic_cast<ui::tools::TextSelectTool*>(tool);
    if (!textTool) return;
    
    textTool->GetSelectionModel().ClearTextSelection();
    
    if (!m_doc) return;
    int pageIndex = GetActivePageIndex();
    if (pageIndex < 0 || pageIndex >= m_doc->PageCount()) return;
    
    auto page = m_doc->GetPage(pageIndex);
    if (!page) return;
    auto textPage = page->LoadTextPage();
    if (!textPage) return;
    
    int charCount = textPage->GetCharCount();
    if (charCount > 0) {
        ui::selection::TextSelectionRange range;
        range.pageIndex = pageIndex;
        range.startCharIndex = 0;
        range.endCharIndex = charCount - 1;
        range.text = textPage->GetText(0, charCount);
        range.rects = textPage->GetRects(0, charCount);
        textTool->GetSelectionModel().SetTextSelection(range);
    }
}

void PdfViewer::SetDarkMode(bool dark) {
    char buf[128]; snprintf(buf, sizeof(buf), "PDFVIEWER: SetDarkMode called with %d", dark);
    utils::Logger::Log(buf);
    if (m_isDarkMode != dark) {
        m_isDarkMode = dark;
        m_generation++;
        core::RenderController::Instance().SetCurrentGeneration(static_cast<int>(m_generation));
        InvalidateView();
    }
}




















