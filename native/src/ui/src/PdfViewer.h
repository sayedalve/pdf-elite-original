#include "../../core/models/RenderResult.h"
#pragma once
#include <windows.h>
#include <memory>
#include <vector>
#include <utility>
#include <unordered_set>
#include "GraphicsDevice.h"
#include "TileCache.h"

#include "core/interfaces/dom/IDocument.h"
#include "core/models/SearchResult.h"
#include "../../pdf_engine/src/SearchEngine.h"
#include "interaction/InteractionManager.h"
#include "input/InputRouter.h"
#include "input/PointerCaptureService.h"
#include "tools/ToolStateMachine.h"
#include "tools/PanTool.h"
#include "viewport/KineticScrollFilter.h"
#include <map>

struct PageLayout {
    int index;
    float yOffset; // Scaled offset from the top
    float width;   // Scaled width
    float height;  // Scaled height
};

namespace ui::annotation {
class IAnnotationHandler;
}

#include "core/interfaces/dom/IAnnotation.h"

enum class ToolMode {
    Select,
    Pan,
    Hand = Pan,
    Highlight,
    Underline,
    Strikeout,
    Squiggly,
    Caret,
    Rectangle,
    Ellipse,
    Line,
    Arrow,
    Ink,
    FreeText,
    TypeWriter,
    TextBox,
    TextCallout,
    StickyNote,
    InsertImage,
    AddText,
    AddImage,
    EditText,
    Stamp,
    Eraser,
    AreaHighlight
};

enum class LayoutMode {
    Continuous,
    SinglePage
};

// Context Menu IDs for Images
#define IDM_IMAGE_REPLACE 3001
#define IDM_IMAGE_EXTRACT 3002
#define IDM_IMAGE_CROP    3003
#define IDM_IMAGE_DELETE  3004

// Context Menu IDs for Text
#define IDM_TEXT_COPY     3010
#define IDM_TEXT_EDIT     3011
#define IDM_TEXT_DELETE   3012
#define IDM_TEXT_ADD_LINK 3013

namespace components { class AppShell; }

class PdfViewer {
public:
    PdfViewer();
    ~PdfViewer();

    bool Initialize(HWND parentHwnd);
    void Render(ComPtr<ID2D1RenderTarget> target, const D2D1_RECT_F& bounds);
    
    // Core document API
    void SetDocumentId(const std::wstring& id) { m_documentId = id; }
    void SetDocument(std::shared_ptr<core::interfaces::dom::IDocument> doc);
    void SetCachedPageSizes(const std::vector<std::pair<float,float>>& sizes) { m_cachedPageSizes = sizes; }
    std::shared_ptr<core::interfaces::dom::IDocument> GetDocument() const { return m_doc; }
    ToolMode GetToolMode() const { return m_currentTool; }
    void SetToolMode(ToolMode mode);

    // Interaction access
    ui::interaction::InteractionManager& GetInteractionManager();
    TileCache* GetTileCache() { return m_tileCache.get(); }
    ui::input::IInputRouter* GetInputRouter() const { return m_inputRouter.get(); }
    ui::tools::ToolStateMachine* GetToolStateMachine() const { return m_toolStateMachine.get(); }
    ui::input::IPointerCaptureService* GetCaptureService() const { return m_captureService.get(); }
    void ReloadInteractableObjects();
    void InvalidateView();
    std::shared_ptr<ui::annotation::IAnnotationHandler> GetActiveAnnotationHandler() const { return m_activeHandler; }
    int GetActivePageIndex() const { return m_activePageIndex; }
    void SelectAllText();

    // Text Search
    void FindText(const std::wstring& query, bool matchCase = false, bool wholeWord = false);
    
    // Page Operations UI Hooks
    void InsertBlankPage(int index, double width, double height);
    void DeletePage(int index);
    void DuplicatePage(int index);
    void MovePage(int sourceIndex, int destIndex);
    void RotatePage(int index, int rotationDegrees);
    void ExecuteMacroStructureChange(std::unique_ptr<core::interfaces::dom::ICommand> cmd);
    
    void TriggerInsertImage();
    void PasteImage();
    void SetPendingStampLabel(const std::wstring& label) { m_pendingStampLabel = label; }
    
    void OnMouseWheel(float delta);
    void OnScroll(float deltaY);
    void OnScrollX(float deltaX);
    void OnThumbnailScroll(int deltaY);
    void OnZoom(double deltaZoom, double mouseX = -1, double mouseY = -1);
    void ZoomToFitWidth();
    void ZoomToFitPage();
    double GetZoom() const { return m_zoom; }
    void OnResize(const D2D1_RECT_F& bounds);
    
    void GoToPage(int pageIndex);
    void NavigateTo(const core::interfaces::dom::NavigationTarget& target);
    
    int GetCurrentPage() const {
        if (m_layoutMode == LayoutMode::SinglePage) return m_currentPage;
        for (const auto& page : m_layout) {
            if (page.yOffset + page.height / 2 > m_scrollY) {
                return page.index;
            }
        }
        if (!m_layout.empty()) return m_layout.back().index;
        return 0;
    }
    
    LayoutMode GetLayoutMode() const { return m_layoutMode; }
    void InvalidateCaches() { m_activePages.clear(); m_interactionManager.SetObjects({}); m_tileCache->InvalidateAll(); }
    void SetLayoutMode(LayoutMode mode);
    void RenderThumbnails(ComPtr<ID2D1RenderTarget> target, const D2D1_RECT_F& bounds);

    bool UpdatePhysics();
    void OnLButtonDown(float x, float y);
    void OnLButtonUp(float x, float y);
    void OnLButtonDoubleClick(float x, float y);
    void OnMButtonDown(float x, float y);
    void OnMButtonUp(float x, float y);
    void OnRButtonUp(float x, float y);
    void OnMouseMove(float x, float y);
    bool OnSetCursor();
    void CancelActiveInteractions();
    void OnKeyDown(WPARAM wParam);
    void OnChar(WPARAM wParam);
    void OnResize(int width, int height);
    void OnPointerDown(float x, float y, int pointerId, bool isRightClick = false);
    void OnPointerUpdate(float x, float y, int pointerId);
    void OnPointerUp(float x, float y, int pointerId, bool isRightClick = false);
    void OnPointerLeave(int pointerId);
    void OnPaste();
    void OnUndo();
    void OnRedo();
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void CopySelection();
    
    // Pointer Event Factory (populates 4-tier spatial coordinates and modifier state)
    ui::input::PointerEvent CreatePointerEvent(
        ui::input::PointerEventType type,
        float x,
        float y,
        ui::input::PointerButton btn = ui::input::PointerButton::None) const;
    
    void SetSearchResults(const std::vector<core::models::SearchResult>& results, int activeIndex);
    
    void OnTileReady(core::models::RenderResult* result, ComPtr<ID2D1RenderTarget> target);

    bool IsRightClickProcessing() const { return m_isRightClickProcessing; }
    
    void SetDarkMode(bool dark);
    bool IsDarkMode() const { return m_isDarkMode; }

    void SetAppShell(std::weak_ptr<components::AppShell> shell) { m_appShell = shell; }
    std::function<void(int, int)> onPageChanged;

private:
    void UpdateVisibleTiles();
    void RecalculateLayout();
    void CachePageSizes();

    bool m_isRightClickProcessing = false;
    std::shared_ptr<ui::interaction::ISelectableObject> m_contextMenuTarget;
    bool m_isDarkMode = false;
    int m_activePageIndex = -1;
    std::weak_ptr<components::AppShell> m_appShell;

    // Rebuilds view state after a structural change to the document (page
    // added / removed / moved / rotated) WITHOUT recreating the render worker
    // or re-initialising forms, so the existing worker (and any sidebar holding
    // a pointer to it) stays valid. Clamps the current page, rebuilds the page
    // layout, invalidates cached tiles and interaction objects, and repaints.
    void RefreshAfterStructureChange();

    // Tears down the render worker and every object derived from the current
    // document (interaction wrappers, text pages, active pages, links) in an
    // order that never lets a PdfPage / PdfTextPage / annotation outlive its
    // parent PdfDocument. Safe to call with no document loaded.
    void ReleaseDocumentState();

    core::interfaces::dom::ITextPage* GetTextPage(int pageIndex);
    void RenderSelection(ComPtr<ID2D1RenderTarget> target, const PageLayout& page, float pageScreenX, float pageScreenY);
    void RenderSearchResults(ComPtr<ID2D1RenderTarget> target, const PageLayout& page, float pageScreenX, float pageScreenY);

    HWND m_hwnd = nullptr;
    std::shared_ptr<core::interfaces::dom::IDocument> m_doc;
    std::vector<std::pair<float, float>> m_cachedPageSizes;
    std::wstring m_documentId;
    
    std::unique_ptr<TileCache> m_tileCache;
    

    std::vector<PageLayout> m_layout;
    bool m_layoutDirty = true;
    std::vector<PageLayout> m_prevLayout;
    double m_prevZoom = 1.0;
    int m_prevScrollX = 0;
    float m_prevScrollY = 0.0f;
    bool m_isZooming = false;
    std::unordered_set<TileKey, TileKeyHash> m_requestedTiles;
    float m_totalHeight = 0;
    
    std::vector<PageLayout> m_thumbnailLayout;
    float m_totalThumbnailHeight = 0;
    float m_thumbnailScrollY = 0;
    D2D1_RECT_F m_thumbnailBounds = {0,0,0,0};

    int m_scrollX = 0;
    float m_scrollY = 0;
    bool m_isDraggingScrollbar = false;
    bool m_isDraggingHScrollbar = false;
    float m_hScrollbarDragOffsetX = 0.0f;
    float m_scrollbarDragOffsetY = 0.0f;
    float m_scrollVelocity = 0;
    float m_scrollAccumulator = 0.0f;
    double m_zoom = 1.0;
    // When true, the view auto-fits page width to the canvas and re-fits on resize;
    // cleared as soon as the user zooms manually. This is the single source of truth
    // for "should I recompute fit-width", so fitting no longer depends on
    // OnResize/SetDocument happening to run when the canvas already has valid bounds.
    bool m_fitWidthMode = true;
    // Largest page width (PDF points) across the document; refreshed in
    // UpdateVisibleTiles and used to compute the fit-width zoom during paint.
    float m_maxPageWidth = 0.0f;
    uint64_t m_generation = 0;
    D2D1_RECT_F m_bounds = {0, 0, 0, 0};
    
    const int TILE_SIZE = 768;
    const int OVERLAP_PX = 5;

    struct SelectionState {
        bool isSelecting = false;
        int startPage = -1;
        int startChar = -1;
        int endPage = -1;
        int endChar = -1;
    } m_selection;
    
    std::map<int, std::unique_ptr<core::interfaces::dom::ITextPage>> m_textPages;
    std::map<int, std::shared_ptr<core::interfaces::dom::IPage>> m_textPages_pages;
    std::map<int, std::shared_ptr<core::interfaces::dom::IPage>> m_activePages;
    
    std::vector<core::models::SearchResult> m_searchResults;
    int m_activeSearchIndex = -1;
    
    ui::interaction::InteractionManager m_interactionManager;
    
    // Text Annotation state
    HWND m_editControl = nullptr;
    WNDPROC m_oldEditProc = nullptr;
    ToolMode m_editTool = ToolMode::Select;
    int m_editPageIndex = -1;
    PointF m_editStartPt = {0,0};
    
    // Image insertion state
    bool m_isInsertingImage = false;
    int m_insertImagePage = -1;
    PointF m_insertImageStartPt;
    std::vector<uint8_t> m_pendingImageData;
    UINT m_pendingImageWidth = 0;
    UINT m_pendingImageHeight = 0;

    // Links
    std::map<int, std::vector<core::interfaces::dom::PdfLink>> m_pageLinks;
    std::optional<core::interfaces::dom::PdfLink> m_hoveredLink;
    
    // Annotation creation state (for Line, Arrow, Rectangle etc)
    bool m_isCreatingAnnotation = false;
    int m_createAnnotationPage = -1;
    PointF m_createAnnotationStartPdf;
    PointF m_createAnnotationCurrentPdf;

    // Tool state
    ToolMode m_currentTool = ToolMode::Select;
    
    // Panning state for Hand tool
    bool m_isPanning = false;
    PointF m_panStartPt = {0.0f, 0.0f};
    PointF m_panStartScroll = {0.0f, 0.0f};
    
    // Middle-mouse button panning (independent of active tool)
    bool m_isMidPanning = false;
    PointF m_midPanStartPt = {0.0f, 0.0f};
    PointF m_midPanStartScroll = {0.0f, 0.0f};
    
    // Ink stroke state (freehand drawing)
    bool m_isDrawingInk = false;
    int m_inkPageIndex = -1;
    std::vector<PointF> m_inkStroke; // accumulated points in PDF coords

    // Sticky note pending placement
    bool m_isPlacingStickyNote = false;

    // Stamp label selected by the stamp picker (set from MainWindow via SetPendingStampLabel)
    std::wstring m_pendingStampLabel;
    
    // Modern Annotation Handlers
    std::map<ToolMode, std::shared_ptr<ui::annotation::IAnnotationHandler>> m_handlers;
    std::shared_ptr<ui::annotation::IAnnotationHandler> m_activeHandler;
    void InitializeAnnotationHandlers();

    // Modern Input and Tool Infrastructure
    std::shared_ptr<ui::input::PointerCaptureService> m_captureService;
    std::shared_ptr<ui::tools::ToolStateMachine> m_toolStateMachine;
    std::unique_ptr<ui::input::InputRouter> m_inputRouter;
    void SetupInputRouting();

    LayoutMode m_layoutMode = LayoutMode::Continuous;
    int m_currentPage = 0;
    float m_accumulatedWheelDelta = 0.0f;
    ui::viewport::KineticScrollFilter m_kineticFilter;
};



