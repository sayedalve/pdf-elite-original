#include "ui/dialogs/MessageDialog.h"
#include "ui/MainWindow.h"
#include <dwmapi.h>
#include "components/AppShell.h"
#include "views/DocumentView.h"
#include "components/BookmarkPanel.h"
#include "components/SearchBar.h"
#include "components/ThumbnailViewer.h"
#include "components/ModeRail.h"
#include "components/StatusBar.h"
#include "components/PropertiesPanel.h"
#include "menu/ContextMenuManager.h"
#include "NativeDesignSystem.h"
#include "views/HomeView.h"
#include "ThemeManager.h"
#include "PdfViewer.h"
#include "AppMode.h"
#include "CommandManager.h"
#include "interaction/TextSelectableObject.h"
#include "interaction/ImageSelectableObject.h"
#include "../../core/TabManager.h"
#include "../../core/DocumentController.h"
#include "../../core/SearchController.h"
#include "../../core/RecentFilesManager.h"
#include "../../core/Clipboard.h"
#include "../../app/PrintManager.h"
#include "tools/SelectTool.h"

#include "../../pdf_engine/src/PdfDocument.h"
#include "../../pdf_engine/src/commands/PageCommands.h"
#include "../../pdf_engine/src/commands/MacroCommand.h"
#include "../../pdf_engine/src/commands/AnnotationCommands.h"

#include "ui/dialogs/LinkDialog.h"
#include "ui/dialogs/BackgroundDialog.h"
#include "ui/dialogs/WatermarkDialog.h"
#include "ui/dialogs/HeaderFooterDialog.h"
#include "ui/dialogs/CreateBlankDialog.h"
#include "ui/dialogs/ExtractImagesDialog.h"
#include "ui/dialogs/CombinePdfDialog.h"

#include "pdf_engine/commands/AddLinkCommand.h"
#include "pdf_engine/commands/AddBackgroundCommand.h"
#include "pdf_engine/commands/AddWatermarkCommand.h"
#include "pdf_engine/commands/AddHeaderFooterCommand.h"
#include "pdf_engine/operations/CreateBlankPdf.h"
#include "pdf_engine/operations/ExtractImagesFromPdf.h"
#include "pdf_engine/operations/CombinePdfs.h"
#include "../../pdf_engine/src/RenderWorker.h"
#include "../../pdf_engine/src/EngineAdapter.h"

#include "utils/Logger.h"
#include "GraphicsDevice.h"

#define SEARCH_BAR_HEIGHT 40
#define TOOLTIP_TIMER_ID 1002

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <set>

#include <fstream>
#include <chrono>

static void LogPipeline(const std::string& msg) {
    std::ofstream out("C:\\Users\\sayed\\Downloads\\PDF-Elite\\pipeline.log", std::ios_base::app);
    out << "[" << std::chrono::system_clock::now().time_since_epoch().count() << "] " << msg << "\n";
}



// Custom window messages
#define WM_APP_SEARCH_COMPLETE (WM_APP + 1)
#define WM_APP_OPEN_FILE       (WM_APP + 2)
#define WM_APP_TILE_READY      (WM_APP + 3)

#define WM_APP_FILE_LOADED (WM_APP + 5)

struct AsyncLoadResult {
    std::wstring path;
    std::shared_ptr<core::interfaces::dom::IDocument> doc;
    std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> bookmarks;
    std::vector<std::pair<float,float>> pageSizes;
    bool success = false;
    explicit AsyncLoadResult(std::wstring p) : path(std::move(p)) {}
};


#define TIMER_RELOAD_OBJECTS   1001
#define TIMER_PHYSICS          1002

using namespace ui;

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// MainWindowImpl
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
struct DocumentTab {
    std::wstring filePath;
    std::wstring title;
    std::shared_ptr<core::interfaces::dom::IDocument> document;
    std::unique_ptr<PdfViewer> viewer;
    bool dirty = false;
};

class MainWindowImpl {
public:
    HWND hwnd = nullptr;
    HWND pageEditHwnd = nullptr;

    std::vector<std::unique_ptr<DocumentTab>> tabs;
    int activeTabIndex = -1;

    std::shared_ptr<::components::AppShell>       appShell;
    std::shared_ptr<views::DocumentView>        documentView;
    std::shared_ptr<views::HomeView>            homeView;
    std::shared_ptr<void>                       currentView;

    std::unique_ptr<SearchBar>                  searchBar;
    std::shared_ptr<::components::BookmarkPanel>  bookmarkPanel;

    bool showBookmarks = false;
    ComPtr<ID2D1DeviceContext5> renderTarget;

    std::wstring currentTooltipText;
    bool tooltipVisible = false;
    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;

    DocumentTab* GetActiveTab() {
        if (activeTabIndex < 0 || activeTabIndex >= static_cast<int>(tabs.size()))
            return nullptr;
        return tabs[activeTabIndex].get();
    }

    PdfViewer* GetViewer() {
        auto* tab = GetActiveTab();
        return tab ? tab->viewer.get() : nullptr;
    }

    void UpdateTabs() {
        if (appShell) {
            if (auto tabBar = appShell->GetDocumentTabs()) {
                std::vector<std::wstring> titles;
                titles.reserve(tabs.size());
                for (const auto& tab : tabs) {
                    if (tab && !tab->title.empty()) {
                        titles.push_back(tab->title);
                    } else {
                        titles.push_back(L"Untitled");
                    }
                }
                tabBar->SetTabs(titles, activeTabIndex);
            }
        }
    }
};

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Constructor / Destructor
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
MainWindow::MainWindow() : m_impl(new MainWindowImpl()) {}
MainWindow::~MainWindow() {
    delete m_impl;
}

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Create
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
bool MainWindow::Create(const std::wstring& title, int width, int height) {
    if (!GraphicsDevice::Instance().Initialize()) {
        ::ui::dialogs::MessageDialog::Show(nullptr, L"Error", L"Failed to initialize GraphicsDevice (D2D/DWrite)!", ::ui::dialogs::MessageDialogType::Ok);
        return false;
    }
    
    try {
        HINSTANCE hInstance = GetModuleHandle(nullptr);
        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc   = MainWindow::WindowProc;
        wc.hInstance     = hInstance;
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        m_className      = L"PDFEliteMainWindow";
        wc.lpszClassName = m_className.c_str();
        wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(101));
        RegisterClassExW(&wc);

        m_hwnd = CreateWindowExW(
            0,
            m_className.c_str(),
            title.c_str(),
            WS_OVERLAPPEDWINDOW & ~WS_CAPTION,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height,
            nullptr, nullptr, hInstance, this
        );

        if (!m_hwnd) {
            DWORD err = GetLastError();
            ::ui::dialogs::MessageDialog::Show(nullptr, L"PDFElite Error", (L"CreateWindowEx failed with error: " + std::to_wstring(err)).c_str(), ::ui::dialogs::MessageDialogType::Ok);
            return false;
        }

        m_impl->hwnd = m_hwnd;
        MARGINS margins = { 1, 1, 1, 1 };
        DwmExtendFrameIntoClientArea(m_hwnd, &margins);
        
        core::RenderController::Instance().SetWorker(std::make_unique<pdf_engine::RenderWorker>(m_hwnd, 4));

        // â”€â”€ Build UI hierarchy â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        m_impl->homeView     = std::make_shared<views::HomeView>();
        m_impl->documentView = std::make_shared<views::DocumentView>();

        m_impl->bookmarkPanel = std::make_shared<::components::BookmarkPanel>();
        m_impl->bookmarkPanel->Create(m_hwnd);
        m_impl->bookmarkPanel->Hide();

        m_impl->homeView->onOpenRequest = [this]() {
            OpenFile();
        };
        m_impl->homeView->onOpenFileRequest = [this](const std::wstring& path) {
            OpenFileDirect(path);
        };
        m_impl->homeView->onOpenFolderRequest = [this](const std::wstring& folderPath) {
            ShellExecuteW(nullptr, L"open", folderPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        };
        m_impl->homeView->onCreateRequest = [this]() {
            DoCreatePdf();
        };
        m_impl->homeView->onNavRequest = [this](int navIdx) {
            (void)navIdx;
            InvalidateRect(m_hwnd, nullptr, FALSE);
        };
        m_impl->homeView->onToolRequest = [this](const std::wstring& toolName) {
            auto openAndSetMode = [this](app::AppMode mode) {
                size_t prevCount = m_impl->tabs.size();
                OpenFile();
                if (m_impl->tabs.size() > prevCount && m_impl->activeTabIndex >= 0) {
                    if (auto rail = m_impl->documentView->GetModeRail()) rail->SetActiveMode(mode);
                    if (auto tb = m_impl->appShell->GetReadingToolbar()) tb->SetMode(mode);
                    RECT rc;
                    GetClientRect(m_hwnd, &rc);
                    SendMessageW(m_hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                }
            };

            if (toolName == L"Combine Files") {
                DoCombineFiles();
            } else if (toolName == L"Create PDF") {
                DoCreatePdf();
            } else if (toolName == L"Edit PDF") {
                openAndSetMode(app::AppMode::Edit);
            } else if (toolName == L"Add Comments") {
                openAndSetMode(app::AppMode::Comment);
            } else if (toolName == L"Convert PDF") {
                openAndSetMode(app::AppMode::Convert);
            } else if (toolName == L"OCR PDF") {
                openAndSetMode(app::AppMode::Tools);
            } else if (toolName == L"Translate PDF") {
                openAndSetMode(app::AppMode::Tools);
            } else if (toolName == L"Compress PDF") {
                openAndSetMode(app::AppMode::Tools);
            } else if (toolName == L"Batch PDFs") {
                openAndSetMode(app::AppMode::Tools);
            } else if (toolName == L"All Tools") {
                if (m_impl->tabs.empty()) {
                    openAndSetMode(app::AppMode::Tools);
                } else {
                    if (auto rail = m_impl->documentView->GetModeRail()) rail->SetActiveMode(app::AppMode::Tools);
                    if (auto tb = m_impl->appShell->GetReadingToolbar()) tb->SetMode(app::AppMode::Tools);
                    m_impl->appShell->SetMode(::components::AppShellMode::Document);
                    RECT rc;
                    GetClientRect(m_hwnd, &rc);
                    SendMessageW(m_hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                }
            }
        };

        m_impl->tabs.clear();
        m_impl->activeTabIndex = -1;
        m_impl->UpdateTabs();

        core::SearchController::Instance().onSearchResultsUpdated = [this](const std::wstring& /*docId*/) {
            PostMessage(m_hwnd, WM_APP_SEARCH_COMPLETE, 0, 0);
        };
        core::SearchController::Instance().onSearchIndexChanged = [this](const std::wstring& /*docId*/, int /*idx*/) {
            PostMessage(m_hwnd, WM_APP_SEARCH_COMPLETE, 0, 0);
        };
        m_impl->currentView = m_impl->homeView;
        m_impl->appShell = std::make_shared<::components::AppShell>();
        m_impl->appShell->SetHomeContent(m_impl->homeView);
        m_impl->appShell->SetDocumentWorkspace(m_impl->documentView);
        m_impl->appShell->SetMode(::components::AppShellMode::Home);

        m_impl->appShell->onMinimize = [this]() {
            ShowWindow(m_hwnd, SW_MINIMIZE);
        };
        m_impl->appShell->onMaximize = [this]() {
            if (IsZoomed(m_hwnd)) {
                ShowWindow(m_hwnd, SW_RESTORE);
            } else {
                ShowWindow(m_hwnd, SW_MAXIMIZE);
            }
        };
        m_impl->appShell->onClose = [this]() {
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
        };

        if (auto tabBar = m_impl->appShell->GetDocumentTabs()) {
            tabBar->SetOnTabSelected([this](int index) {
                SwitchToTab(index);
            });
            tabBar->SetOnTabClosed([this](int index) {
                CloseTab(index);
            });
        }

        if (auto tb = m_impl->appShell->GetReadingToolbar()) {
            // Register a few tools to demonstrate the new Action architecture
            auto cmdMgr = &ui::commands::CommandManager::Instance();
            cmdMgr->RegisterAction(std::make_shared<ui::commands::Action>(L"Hand", [this, tb]() {
                tb->SetActiveTool(L"Select");
                if (m_impl->GetViewer()) m_impl->GetViewer()->SetToolMode(ToolMode::Pan);
            }));
            cmdMgr->RegisterAction(std::make_shared<ui::commands::Action>(L"Select", [this, tb]() {
                tb->SetActiveTool(L"Select");
                if (m_impl->GetViewer()) m_impl->GetViewer()->SetToolMode(ToolMode::Select);
            }));
            cmdMgr->RegisterAction(std::make_shared<ui::commands::Action>(L"Edit All", [this, tb]() {
                tb->SetActiveTool(L"Edit All");
                if (m_impl->GetViewer()) m_impl->GetViewer()->SetToolMode(ToolMode::EditText);
            }));

            tb->onAction = [this, tb, cmdMgr](const std::wstring& action) {
                // Execute via new Command Architecture (which updates observers)
                cmdMgr->ExecuteAction(action);
                
                // Fallback for legacy actions
                if (action == L"Save") {
                    DoSave(m_impl->activeTabIndex);
                }
                else if (action == L"Print") {
                    if (auto tab = m_impl->GetActiveTab()) {
                        if (tab->document) {
                            app::PrintManager::PrintDocument(m_hwnd, tab->document);
                        }
                    }
                }
                else if (action == L"Save As") {
                    DoSaveAs(m_impl->activeTabIndex);
                }
                else if (action == L"Undo") {
                    if (auto tab = m_impl->GetActiveTab()) {
                        if (tab->document) {
                            tab->document->GetCommandStack().Undo();
                            if (tab->viewer) tab->viewer->InvalidateView();
                            RefreshAfterPageOp();
                        }
                    }
                }
                else if (action == L"Redo") {
                    if (auto tab = m_impl->GetActiveTab()) {
                        if (tab->viewer) {
                            tab->viewer->OnRedo();
                            RefreshAfterPageOp();
                        }
                    }
                }
                else if (action == L"Zoom Out") { if (m_impl->GetViewer()) m_impl->GetViewer()->OnZoom(-0.2f); }
                else if (action == L"ZoomIn")   { if (m_impl->GetViewer()) m_impl->GetViewer()->OnZoom(0.2f); }
                else if (action == L"Fit Width") { if (m_impl->GetViewer()) m_impl->GetViewer()->ZoomToFitWidth(); }
                else if (action == L"Fit Page")  { if (m_impl->GetViewer()) m_impl->GetViewer()->ZoomToFitPage(); }
                else if (action == L"First") { if (m_impl->GetViewer()) m_impl->GetViewer()->GoToPage(0); }
                else if (action == L"Prev") {
                    if (m_impl->GetViewer() && m_impl->GetActiveTab() && m_impl->GetActiveTab()->document) {
                        int current = m_impl->GetViewer()->GetCurrentPage();
                        if (current > 0) m_impl->GetViewer()->GoToPage(current - 1);
                    }
                }
                else if (action == L"Next") {
                    if (m_impl->GetViewer() && m_impl->GetActiveTab() && m_impl->GetActiveTab()->document) {
                        int current = m_impl->GetViewer()->GetCurrentPage();
                        if (current < m_impl->GetActiveTab()->document->PageCount() - 1) m_impl->GetViewer()->GoToPage(current + 1);
                    }
                }
                else if (action == L"Last") {
                    if (m_impl->GetViewer() && m_impl->GetActiveTab() && m_impl->GetActiveTab()->document) {
                        m_impl->GetViewer()->GoToPage(m_impl->GetActiveTab()->document->PageCount() - 1);
                    }
                }
                else if (action == L"Hand")   { tb->SetActiveTool(L"Select");   if (m_impl->GetViewer()) m_impl->GetViewer()->SetToolMode(ToolMode::Pan); }
                else if (action == L"Select") { tb->SetActiveTool(L"Select"); if (m_impl->GetViewer()) m_impl->GetViewer()->SetToolMode(ToolMode::Select); }
                else if (action == L"Highlight" || action == L"Area Highlight" || action == L"Pencil" || action == L"Draw" || action == L"Ink" || action == L"Eraser" || action == L"Type Writer" || action == L"Text Box" || action == L"Text Callout" || action == L"Note" || action == L"Comment" || action == L"Rectangle" || action == L"Ellipse" || action == L"Line" || action == L"Arrow") {
                    auto toggleTool = [&](const std::wstring& btnAction, ToolMode mode) {
                        if (auto viewer = m_impl->GetViewer()) {
                            if (viewer->GetToolMode() == mode) {
                                viewer->SetToolMode(ToolMode::Select);
                                tb->SetActiveTool(L"Select");
                            } else {
                                viewer->SetToolMode(mode);
                                tb->SetActiveTool(btnAction);
                            }
                        }
                    };

                    if (action == L"Highlight") toggleTool(action, ToolMode::Highlight);
                    else if (action == L"Area Highlight") toggleTool(action, ToolMode::AreaHighlight);
                    else if (action == L"Pencil" || action == L"Draw" || action == L"Ink") toggleTool(action, ToolMode::Ink);
                    else if (action == L"Eraser") toggleTool(action, ToolMode::Eraser);
                    else if (action == L"Type Writer") toggleTool(action, ToolMode::TypeWriter);
                    else if (action == L"Text Box") toggleTool(action, ToolMode::TextBox);
                    else if (action == L"Text Callout") toggleTool(action, ToolMode::TextCallout);
                    else if (action == L"Note") toggleTool(action, ToolMode::StickyNote);
                    else if (action == L"Comment") toggleTool(action, ToolMode::AddText); // Or whatever Comment mode is
                    else if (action == L"Rectangle") toggleTool(action, ToolMode::Rectangle);
                    else if (action == L"Ellipse") toggleTool(action, ToolMode::Ellipse);
                    else if (action == L"Line") toggleTool(action, ToolMode::Line);
                    else if (action == L"Arrow") toggleTool(action, ToolMode::Arrow);
                }
                else if (action == L"Edit All")   { tb->SetActiveTool(L"Edit All");   if (m_impl->GetViewer()) m_impl->GetViewer()->SetToolMode(ToolMode::EditText); }
                else if (action == L"Add Text")   { tb->SetActiveTool(L"Add Text");   if (m_impl->GetViewer()) m_impl->GetViewer()->SetToolMode(ToolMode::AddText); }
                else if (action == L"Image") { if (m_impl->GetViewer()) m_impl->GetViewer()->TriggerInsertImage(); }
                else if (action == L"Stamps") {
                    // Simple stamp picker: offer common stamps via a menu
                    if (auto viewer = m_impl->GetViewer(); viewer && viewer->GetDocument()) {
                        HMENU hMenu = CreatePopupMenu();
                        AppendMenuW(hMenu, MF_STRING, 4001, L"APPROVED");
                        AppendMenuW(hMenu, MF_STRING, 4002, L"REJECTED");
                        AppendMenuW(hMenu, MF_STRING, 4003, L"DRAFT");
                        AppendMenuW(hMenu, MF_STRING, 4004, L"CONFIDENTIAL");
                        AppendMenuW(hMenu, MF_STRING, 4005, L"FOR REVIEW");
                        AppendMenuW(hMenu, MF_STRING, 4006, L"VOID");
                        POINT pt; GetCursorPos(&pt);
                        int choice = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, NULL);
                        DestroyMenu(hMenu);
                        static const wchar_t* stampLabels[] = {
                            nullptr, L"APPROVED", L"REJECTED", L"DRAFT",
                            L"CONFIDENTIAL", L"FOR REVIEW", L"VOID"
                        };
                        if (choice >= 4001 && choice <= 4006) {
                            viewer->SetPendingStampLabel(stampLabels[choice - 4000]);
                            tb->SetActiveTool(L"Stamps");
                            viewer->SetToolMode(ToolMode::Stamp);
                        }
                    }
                }
                else if (action == L"Signature") {
                    // Signature uses freehand ink mode — user draws their signature
                    tb->SetActiveTool(L"Signature");
                    if (m_impl->GetViewer()) m_impl->GetViewer()->SetToolMode(ToolMode::Ink);
                }
                else if (action == L"Rotate CW") {
                    if (auto tab = m_impl->GetActiveTab()) {
                        if (tab->document && tab->viewer) {
                            auto selected = m_impl->documentView->GetLeftSidebar()->GetSelectedPages();
                            if (selected.empty()) selected.insert(tab->viewer->GetCurrentPage());
                            auto macro = std::make_unique<pdf_engine::commands::MacroCommand>("Rotate Pages CW");
                            for (int page : selected) {
                                macro->AddCommand(std::make_unique<pdf_engine::commands::RotatePageCommand>(static_cast<PdfDocument*>(tab->document.get()), page, 90));
                            }
                            if (tab->document->GetCommandStack().ExecuteCommand(std::move(macro))) {
                                RefreshAfterPageOp();
                            }
                        }
                    }
                }
                else if (action == L"Rotate CCW") {
                    if (auto tab = m_impl->GetActiveTab()) {
                        if (tab->document && tab->viewer) {
                            auto selected = m_impl->documentView->GetLeftSidebar()->GetSelectedPages();
                            if (selected.empty()) selected.insert(tab->viewer->GetCurrentPage());
                            auto macro = std::make_unique<pdf_engine::commands::MacroCommand>("Rotate Pages CCW");
                            for (int page : selected) {
                                macro->AddCommand(std::make_unique<pdf_engine::commands::RotatePageCommand>(static_cast<PdfDocument*>(tab->document.get()), page, 270));
                            }
                            if (tab->document->GetCommandStack().ExecuteCommand(std::move(macro))) {
                                RefreshAfterPageOp();
                            }
                        }
                    }
                }
                else if (action == L"Delete") {
                    if (auto tab = m_impl->GetActiveTab()) {
                        if (tab->document && tab->viewer) {
                            auto selected = m_impl->documentView->GetLeftSidebar()->GetSelectedPages();
                            if (selected.empty()) selected.insert(tab->viewer->GetCurrentPage());
                            if (selected.size() >= static_cast<size_t>(tab->document->PageCount())) {
                                ::ui::dialogs::MessageDialog::Show(m_hwnd, L"Delete Page", L"A document must keep at least one page.", ::ui::dialogs::MessageDialogType::Ok);
                                return;
                            }
                            auto macro = std::make_unique<pdf_engine::commands::MacroCommand>("Delete Pages");
                            std::vector<int> sortedPages(selected.begin(), selected.end());
                            for (auto it = sortedPages.rbegin(); it != sortedPages.rend(); ++it) {
                                macro->AddCommand(std::make_unique<pdf_engine::commands::DeletePageCommand>(static_cast<PdfDocument*>(tab->document.get()), *it));
                            }
                            tab->viewer->ExecuteMacroStructureChange(std::move(macro));
                            int current = tab->viewer->GetCurrentPage();
                            if (current >= tab->document->PageCount()) current = tab->document->PageCount() - 1;
                            tab->viewer->GoToPage(current);
                            RefreshAfterPageOp();
                        }
                    }
                }
                else if (action == L"Insert") {
                    if (auto tab = m_impl->GetActiveTab()) {
                        if (tab->document && tab->viewer) {
                            int current = tab->viewer->GetCurrentPage();
                            auto size = tab->document->GetPageSize(current);
                            tab->viewer->InsertBlankPage(current + 1, size.width, size.height);
                            RefreshAfterPageOp();
                        }
                    }
                }
                else if (action == L"Extract Page") {
                    if (auto tab = m_impl->GetActiveTab()) {
                        if (tab->document && tab->viewer) {
                            auto selected = m_impl->documentView->GetLeftSidebar()->GetSelectedPages();
                            if (selected.empty()) selected.insert(tab->viewer->GetCurrentPage());
                            std::vector<int> indices(selected.begin(), selected.end());
                            auto extracted = tab->document->ExtractPages(indices);
                            if (extracted) {
                                OPENFILENAMEW ofn = {};
                                ofn.lStructSize  = sizeof(ofn);
                                ofn.hwndOwner    = m_hwnd;
                                wchar_t szFile[MAX_PATH] = {0};
                                ofn.lpstrFile    = szFile;
                                ofn.nMaxFile     = MAX_PATH;
                                ofn.lpstrFilter  = L"PDF Documents (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0";
                                ofn.nFilterIndex = 1;
                                ofn.Flags        = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                                ofn.lpstrDefExt  = L"pdf";
                                if (GetSaveFileNameW(&ofn)) {
                                    extracted->SaveAs(ofn.lpstrFile);
                                }
                            }
                        }
                    }
                }
                else if (action == L"Search") {
                    if (m_impl->searchBar) m_impl->searchBar->Show();
                }
                else if (action == L"Thumbnails") {
                    if (m_impl->documentView) {
                        auto sidebar = m_impl->documentView->GetLeftSidebar();
                        if (sidebar) {
                            if (sidebar->IsVisible()) sidebar->SetVisible(false);
                            else sidebar->SetVisible(true);
                            RECT rc; GetClientRect(m_hwnd, &rc);
                            SendMessageW(m_hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
                        }
                    }
                }
                else                   if (action == L"More") {
                      if (m_impl->GetViewer()) {
                          ui::menu::ContextMenuInfo info;
                          info.targetType = ui::menu::TargetType::PageCanvas;
                          if (m_impl->GetViewer()->GetDocument()) {
                              info.canUndo = m_impl->GetViewer()->GetDocument()->GetCommandStack().CanUndo();
                              info.canRedo = m_impl->GetViewer()->GetDocument()->GetCommandStack().CanRedo();
                          }
                          POINT pt;
                          GetCursorPos(&pt);
                          ui::menu::ContextMenuManager::Instance().ShowContextMenu(m_hwnd, pt, info);
                      }
                      return;
                  }
                  if (action == L"Dark Mode") {
                    if (auto viewer = m_impl->GetViewer()) {
                        viewer->SetDarkMode(!viewer->IsDarkMode());
                        if (auto sidebar = m_impl->documentView->GetLeftSidebar()) {
                            sidebar->SetDarkMode(viewer->IsDarkMode());
                        }
                        InvalidateRect(m_hwnd, nullptr, FALSE);
                    }
                    return;
                }

                else if (action == L"Add Link") { DoAddLink(); }
                else if (action == L"Background") { DoAddBackground(); }
                else if (action == L"Watermark") { DoAddWatermark(); }
                else if (action == L"Header & Footer" || action == L"Header/Footer") { DoAddHeaderFooter(); }
                else if (action == L"Create PDF" || action == L"New PDF") { DoCreatePdf(); }
                else if (action == L"Extract Images") { DoExtractImages(); }
                else if (action == L"Combine Files" || action == L"Combine PDF" || action == L"Merge PDF") { DoCombineFiles(); }

                else if (action == L"To Word" || action == L"To Excel" || action == L"To Image" ||
                         action == L"OCR PDF" || action == L"Compress" || action == L"Batch PDFs") {
                    ::ui::dialogs::MessageDialog::Show(m_hwnd, L"Feature Unavailable", (std::wstring(L"The '") + action + L"' feature is not available in the current edition.").c_str(), ::ui::dialogs::MessageDialogType::Ok);
                }

                InvalidateRect(m_hwnd, nullptr, FALSE);
            };
        }

        if (auto rail = m_impl->documentView->GetModeRail()) {
            rail->SetOnModeSelected([this](app::AppMode mode) {
                if (mode == app::AppMode::Home) {
                    if (auto r = m_impl->documentView->GetModeRail()) r->SetActiveMode(app::AppMode::View);
                    if (auto tb = m_impl->appShell->GetReadingToolbar()) tb->SetMode(app::AppMode::View);
                    if (m_impl->appShell->onHomeRequest) m_impl->appShell->onHomeRequest();
                    return;
                }
                if (m_impl->documentView) m_impl->documentView->SetAppMode(mode);
                if (auto tb = m_impl->appShell->GetReadingToolbar()) tb->SetMode(mode);
                RECT rc;
                GetClientRect(m_hwnd, &rc);
                SendMessageW(m_hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
                InvalidateRect(m_hwnd, nullptr, FALSE);
            });
            rail->SetActiveMode(app::AppMode::View);
        }

        if (auto status = m_impl->documentView->GetStatusBar()) {
            status->onAction = [this](const std::wstring& action) {
                if (action == L"Thumbnails") {
                    if (m_impl->documentView) {
                        auto sidebar = m_impl->documentView->GetLeftSidebar();
                        if (sidebar) {
                            sidebar->SetVisible(!sidebar->IsVisible());
                            RECT rc;
                            GetClientRect(m_hwnd, &rc);
                            SendMessageW(m_hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
                        }
                    }
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                    return;
                }
                                  if (action == L"More") {
                      if (m_impl->GetViewer()) {
                          ui::menu::ContextMenuInfo info;
                          info.targetType = ui::menu::TargetType::PageCanvas;
                          if (m_impl->GetViewer()->GetDocument()) {
                              info.canUndo = m_impl->GetViewer()->GetDocument()->GetCommandStack().CanUndo();
                              info.canRedo = m_impl->GetViewer()->GetDocument()->GetCommandStack().CanRedo();
                          }
                          POINT pt;
                          GetCursorPos(&pt);
                          ui::menu::ContextMenuManager::Instance().ShowContextMenu(m_hwnd, pt, info);
                      }
                      return;
                  }
                  if (action == L"Dark Mode") {
                    if (auto viewer = m_impl->GetViewer()) {
                        viewer->SetDarkMode(!viewer->IsDarkMode());
                        if (auto sidebar = m_impl->documentView->GetLeftSidebar()) {
                            sidebar->SetDarkMode(viewer->IsDarkMode());
                        }
                        InvalidateRect(m_hwnd, nullptr, FALSE);
                    }
                    return;
                }
                if (action == L"Bookmarks" || action == L"Comments") {
                    m_impl->showBookmarks = !m_impl->showBookmarks;
                    if (m_impl->showBookmarks) {
                        if (m_impl->bookmarkPanel) m_impl->bookmarkPanel->Show();
                    } else {
                        if (m_impl->bookmarkPanel) m_impl->bookmarkPanel->Hide();
                    }
                    RECT rc;
                    GetClientRect(m_hwnd, &rc);
                    SendMessageW(m_hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                    return;
                }

                if (auto viewer = m_impl->GetViewer()) {
                    if (action == L"Zoom In") {
                        viewer->OnZoom(1.2f);
                    } else if (action == L"Zoom Out") {
                        viewer->OnZoom(1.0f / 1.2f);
                    } else if (action == L"Fit") {
                        viewer->ZoomToFitWidth();
                    } else if (action == L"Page Up") {
                        viewer->GoToPage(std::max(0, viewer->GetCurrentPage() - 1));
                                        } else if (action == L"Page Down") {
                        auto tab = m_impl->GetActiveTab();
                        if (tab && tab->document) {
                            viewer->GoToPage(std::min(tab->document->PageCount() - 1, viewer->GetCurrentPage() + 1));
                        }
                    } else if (action == L"GoToPageDialog") {
                        auto tab = m_impl->GetActiveTab();
                        if (tab && tab->document && tab->viewer) {
                            if (!m_impl->pageEditHwnd) {
                                m_impl->pageEditHwnd = CreateWindowExW(
                                    0, L"EDIT", L"",
                                    WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER,
                                    0, 0, 0, 0, m_hwnd, (HMENU)105, GetModuleHandle(nullptr), nullptr
                                );
                                SetWindowSubclass(m_impl->pageEditHwnd, [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) -> LRESULT {
                                    MainWindow* pThis = (MainWindow*)dwRefData;
                                    if (uMsg == WM_KEYDOWN) {
                                        if (wParam == VK_RETURN) {
                                            wchar_t buf[256] = {0};
                                            GetWindowTextW(hWnd, buf, 256);
                                            int page = _wtoi(buf) - 1;
                                            auto t = pThis->m_impl->GetActiveTab();
                                            if (t && t->document && t->viewer) {
                                                t->viewer->GoToPage(page);
                                            }
                                            ShowWindow(hWnd, SW_HIDE);
                                            ::SetFocus(pThis->GetHwnd());
                                            return 0;
                                        } else if (wParam == VK_ESCAPE) {
                                            ShowWindow(hWnd, SW_HIDE);
                                            ::SetFocus(pThis->GetHwnd());
                                            return 0;
                                        }
                                    } else if (uMsg == WM_KILLFOCUS) {
                                        ShowWindow(hWnd, SW_HIDE);
                                    }
                                    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
                                }, 0, (DWORD_PTR)this);
                                
                                HFONT hFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                                SendMessage(m_impl->pageEditHwnd, WM_SETFONT, (WPARAM)hFont, FALSE);
                            }
                            
                            RECT pr; GetClientRect(m_hwnd, &pr);
                            float btnSize = 40.0f;
                            float sidebarW = 48.0f;
                            float x1 = (pr.right - sidebarW) + (sidebarW - btnSize) / 2.0f;
                            float by = pr.bottom - 8.0f;
                            by -= btnSize + 8.0f;
                            by -= btnSize + 8.0f;
                            by -= 32.0f;
                            by -= 8.0f;
                            by -= btnSize + 8.0f;
                            by -= btnSize + 8.0f;
                            by -= 24.0f;
                            by -= 4.0f;
                            by -= 32.0f;
                            
                            SetWindowPos(m_impl->pageEditHwnd, HWND_TOP, (int)x1, (int)by, (int)btnSize, 32, SWP_SHOWWINDOW);
                            std::wstring curStr = std::to_wstring(tab->viewer->GetCurrentPage() + 1);
                            SetWindowTextW(m_impl->pageEditHwnd, curStr.c_str());
                            SendMessage(m_impl->pageEditHwnd, EM_SETSEL, 0, -1);
                            ::SetFocus(m_impl->pageEditHwnd);
                        }
                    }
                    if (auto st = m_impl->documentView->GetStatusBar()) {
                        st->SetZoom(static_cast<float>(viewer->GetZoom()));
                    }
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                }
            };
        }

        m_impl->searchBar = std::make_unique<SearchBar>();
        m_impl->searchBar->Create(m_hwnd);
        m_impl->searchBar->SetOnSearchCallback([this](const std::wstring& query) {
            auto tab = m_impl->GetActiveTab();
            if (!tab) return;
            core::SearchController::Instance().SearchAsync(tab->filePath, query, m_impl->searchBar->GetCaseSensitive(), m_impl->searchBar->GetWholeWord());
        });
        m_impl->searchBar->SetOnNextCallback([this]() {
            auto tab = m_impl->GetActiveTab();
            if (!tab) return;
            core::SearchController::Instance().NextMatch(tab->filePath);
        });
        m_impl->searchBar->SetOnPrevCallback([this]() {
            auto tab = m_impl->GetActiveTab();
            if (!tab) return;
            core::SearchController::Instance().PrevMatch(tab->filePath);
        });
        m_impl->searchBar->SetOnCloseCallback([this]() {
            if (m_impl->searchBar) {
                m_impl->searchBar->Hide();
            }
            SetFocus(m_hwnd);
        });

        m_impl->appShell->onHomeRequest = [this]() {
            bool canGoHomeNow = true;
            for (auto& tab : m_impl->tabs) {
                if (tab->dirty) {
                    canGoHomeNow = false;
                    break;
                }
            }
            if (!canGoHomeNow) return;

            if (m_impl->searchBar) m_impl->searchBar->Hide();
            if (m_impl->bookmarkPanel) m_impl->bookmarkPanel->Hide();
            m_impl->showBookmarks = false;
            if (m_impl->documentView && m_impl->documentView->GetPropertiesPanel()) {
                m_impl->documentView->GetPropertiesPanel()->SetVisible(false);
                m_impl->documentView->GetPropertiesPanel()->SetSelectedObject(nullptr);
            }

            for (auto& tab : m_impl->tabs) {
                if (tab && tab->viewer) {
                    tab->viewer->GetInteractionManager().onSelectionChanged = nullptr;
                }
            }

            m_impl->tabs.clear();
            m_impl->activeTabIndex = -1;
            m_impl->UpdateTabs();
            m_impl->currentView = m_impl->homeView;
            m_impl->appShell->SetMode(::components::AppShellMode::Home);
            RECT rc;
            if (m_hwnd && GetClientRect(m_hwnd, &rc)) {
                UINT dpi = GetDpiForWindow(m_hwnd);
                float scale = dpi > 0 ? (dpi / 96.0f) : 1.0f;
                if (m_impl->appShell) {
                    m_impl->appShell->Layout(D2D1::RectF(0, 0, static_cast<float>(rc.right) / scale, static_cast<float>(rc.bottom) / scale));
                }
            }
            InvalidateRect(m_hwnd, nullptr, FALSE);
        };

        ThemeManager::Instance().OnThemeChanged.AddListener([this]() {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        });

        DragAcceptFiles(m_hwnd, TRUE);
        SetTimer(m_hwnd, TIMER_PHYSICS, 16, nullptr);
        return true;
    }
    catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Exception in Create", MB_OK);
        return false;
    }
}

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Show
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
void MainWindow::Show(int nCmdShow) {
    if (!m_hwnd) return;
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    if (m_impl->appShell) {
        m_impl->appShell->Layout(D2D1::RectF(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
    }
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// WindowProc (static)
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;
    if (uMsg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = static_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
        if (pThis->m_impl) pThis->m_impl->hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    if (pThis) return pThis->HandleMessage(uMsg, wParam, lParam);
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// HandleMessage
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        switch (uMsg) {
        case WM_NCCALCSIZE: {
            if (wParam == TRUE) {
                return 0; // Remove standard non-client area (title bar and borders)
            }
            return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
        }
        case WM_NCHITTEST: {
            LRESULT hit = DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
            if (hit == HTCLIENT) {
                POINT pt;
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
                ScreenToClient(m_hwnd, &pt);
                if (pt.y < 40) {
                    RECT rc;
                    GetClientRect(m_hwnd, &rc);
                    UINT dpi = GetDpiForWindow(m_hwnd);
                    float scale = dpi > 0 ? (dpi / 96.0f) : 1.0f;
                    if (pt.x >= rc.right - (46 * 3 * scale)) return HTCLIENT; // Min/Max/Close
                    if (pt.x >= 60 * scale && pt.x < rc.right - (200 * scale)) return HTCLIENT; // Tabs
                    return HTCAPTION;
                }
            }
            return hit;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(m_hwnd, &ps);
            if (!m_impl->renderTarget) {
                RECT rc;
                GetClientRect(m_hwnd, &rc);
                GraphicsDevice::Instance().CreateRenderTarget(m_hwnd, rc.right, rc.bottom, m_impl->renderTarget);
            }
            if (m_impl->renderTarget) {
                // COORDINATE SYSTEM EXPLANATION:
                // - WM_SIZE delivers physical pixels (w, h). Layout is done at w/scale, h/scale
                //   which are 96-DPI logical units (DIPs). All panel/canvas bounds are in 96-DPI DIPs.
                // - The DXGI swapchain is sized in physical pixels.
                // - SetDpi(dpi, dpi) tells D2D to scale: 1 logical unit (96-DPI DIP) → dpi/96 physical pixels.
                //   At 150% DPI: 1 logical → 1.5 physical, so layout coords 0..1280 fill 0..1920 physical. ✓
                // - WITHOUT SetDpi: 1 logical → 1 physical, layout 0..1280 fills only 0..1280 of 1920px window.
                // - Mouse coords from WM_LBUTTONDOWN are physical, divided by scale = 96-DPI logical. ✓
                UINT dpi = GetDpiForWindow(m_hwnd);
                float dpiVal = dpi > 0 ? static_cast<float>(dpi) : 96.0f;
                m_impl->renderTarget->SetDpi(dpiVal, dpiVal);
                m_impl->renderTarget->BeginDraw();
                m_impl->renderTarget->Clear(design::Colors::Background);
                
                if (m_impl->appShell) {
                    m_impl->appShell->Render(m_impl->renderTarget);
                }
                
                if (m_impl->appShell && m_impl->appShell->GetMode() == ::components::AppShellMode::Document) {
                    bool isOrg = m_impl->documentView && m_impl->documentView->GetOrganizeView()->IsVisible();
                    if (auto viewer = m_impl->GetViewer()) {
                        if (!isOrg) {
                            D2D1_RECT_F cb = m_impl->documentView->GetCanvasContainer()->GetBounds();
                            // BUG-08 fix: skip render if bounds are not yet valid (before first WM_SIZE)
                            if (cb.right > cb.left && cb.bottom > cb.top) {
                                viewer->Render(m_impl->renderTarget, cb);
                            }
                        }
                    }
                }

                if (m_impl->tooltipVisible && !m_impl->currentTooltipText.empty()) {
                    auto format = design::FontManager::Instance().GetCaption();
                    if (format) {
                        ComPtr<IDWriteTextLayout> layout;
                        GraphicsDevice::Instance().GetDWriteFactory()->CreateTextLayout(
                            m_impl->currentTooltipText.c_str(),
                            static_cast<UINT32>(m_impl->currentTooltipText.length()),
                            format,
                            400.0f, 100.0f,
                            &layout);
                        if (layout) {
                            DWRITE_TEXT_METRICS metrics;
                            layout->GetMetrics(&metrics);
                            float padding = 6.0f;
                            
                            // Get window size
                            RECT rcClient;
                            GetClientRect(m_hwnd, &rcClient);
                            UINT winDpi = GetDpiForWindow(m_hwnd);
                            float winScale = winDpi > 0 ? (winDpi / 96.0f) : 1.0f;
                            float winW = static_cast<float>(rcClient.right) / winScale;
                            float winH = static_cast<float>(rcClient.bottom) / winScale;
                            
                            float tipW = metrics.width + padding * 2;
                            float tipH = metrics.height + padding * 2;
                            
                            float tx = m_impl->lastMouseX + 10.0f;
                            float ty = m_impl->lastMouseY + 20.0f;
                            
                            // If mouse is on the right sidebar, position tooltip to the left of the cursor
                            if (m_impl->lastMouseX > winW - 80.0f) {
                                tx = m_impl->lastMouseX - tipW - 10.0f;
                            }
                            
                            if (tx + tipW > winW) tx = winW - tipW - 4.0f;
                            if (tx < 0) tx = 4.0f;
                            if (ty + tipH > winH) ty = m_impl->lastMouseY - tipH - 4.0f;
                            
                            D2D1_RECT_F tipRect = D2D1::RectF(tx, ty, tx + tipW, ty + tipH);

                            ComPtr<ID2D1SolidColorBrush> bgBrush, borderBrush, textBrush;
                            m_impl->renderTarget->CreateSolidColorBrush(design::Colors::Surface, &bgBrush);
                            m_impl->renderTarget->CreateSolidColorBrush(design::Colors::BorderSubtle, &borderBrush);
                            m_impl->renderTarget->CreateSolidColorBrush(design::Colors::TextPrimary, &textBrush);
                            if (bgBrush && borderBrush && textBrush) {
                                D2D1_ROUNDED_RECT rrect = D2D1::RoundedRect(tipRect, 4.0f, 4.0f);
                                m_impl->renderTarget->FillRoundedRectangle(&rrect, bgBrush.Get());
                                m_impl->renderTarget->DrawRoundedRectangle(&rrect, borderBrush.Get(), 1.0f);
                                m_impl->renderTarget->DrawTextLayout(
                                    D2D1::Point2F(tipRect.left + padding, tipRect.top + padding),
                                    layout.Get(), textBrush.Get());
                            }
                        }
                    }
                }
                
                HRESULT hr = m_impl->renderTarget->EndDraw();
                if (hr == D2DERR_RECREATE_TARGET) m_impl->renderTarget.Reset();
                
                GraphicsDevice::Instance().Present();
            }
            EndPaint(m_hwnd, &ps);
            return 0;
        }
        case WM_SIZE: {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            UINT dpi = GetDpiForWindow(m_hwnd);
            float scale = dpi > 0 ? (dpi / 96.0f) : 1.0f;
            GraphicsDevice::Instance().Resize(w, h);
            
            if (m_impl->documentView) {
                m_impl->documentView->SetRightPanelVisible(m_impl->showBookmarks);
            }
            
            if (m_impl->appShell) {
                m_impl->appShell->Layout(D2D1::RectF(0, 0, static_cast<float>(w) / scale, static_cast<float>(h) / scale));
            }
            if (auto viewer = m_impl->GetViewer()) {
                viewer->OnResize(m_impl->documentView->GetCanvasContainer()->GetBounds());
            }
            
            if (m_impl->showBookmarks && m_impl->bookmarkPanel && m_impl->documentView) {
                D2D1_RECT_F rb = m_impl->documentView->GetRightPanelBounds();
                m_impl->bookmarkPanel->SetBounds(
                    (int)(rb.left * scale), 
                    (int)(rb.top * scale), 
                    (int)((rb.right - rb.left) * scale), 
                    (int)((rb.bottom - rb.top) * scale)
                );
            }
            
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_COMMAND: {
            if (auto viewer = m_impl->GetViewer()) {
                viewer->OnCommand(wParam, lParam);
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            UINT dpi = GetDpiForWindow(m_hwnd);
            float scale = dpi > 0 ? (dpi / 96.0f) : 1.0f;
            float dipX = GET_X_LPARAM(lParam) / scale;
            float dipY = GET_Y_LPARAM(lParam) / scale;
            auto viewer = m_impl->GetViewer();
            bool viewerHasCapture = viewer && viewer->GetCaptureService() && viewer->GetCaptureService()->IsAnyCaptured();
            
            std::wstring tooltipSource = L"";
            if (viewerHasCapture) {
                viewer->OnMouseMove(dipX, dipY);
            } else {
                if (m_impl->appShell) {
                    m_impl->appShell->OnMouseMove(dipX, dipY);
                    tooltipSource = m_impl->appShell->GetTooltipText();
                }
                if (m_impl->appShell && m_impl->appShell->GetMode() == ::components::AppShellMode::Document) {
                    bool isOrg = m_impl->documentView && m_impl->documentView->GetOrganizeView()->IsVisible();
                    if (viewer && !isOrg) {
                        viewer->OnMouseMove(dipX, dipY);
                        // For viewer tooltips if any
                        // std::wstring vTip = viewer->GetTooltipText();
                        // if (!vTip.empty()) tooltipSource = vTip;
                    }
                }
            }

            if (tooltipSource != m_impl->currentTooltipText) {
                m_impl->currentTooltipText = tooltipSource;
                m_impl->tooltipVisible = false;
                if (!m_impl->currentTooltipText.empty()) {
                    SetTimer(m_hwnd, TOOLTIP_TIMER_ID, 500, nullptr);
                } else {
                    KillTimer(m_hwnd, TOOLTIP_TIMER_ID);
                }
            }
            InvalidateRect(m_hwnd, nullptr, FALSE);
            m_impl->lastMouseX = dipX;
            m_impl->lastMouseY = dipY;

            return 0;
        }
        case WM_LBUTTONDOWN: {
            UINT dpi = GetDpiForWindow(m_hwnd);
            float scale = dpi > 0 ? (dpi / 96.0f) : 1.0f;
            float dipX = GET_X_LPARAM(lParam) / scale;
            float dipY = GET_Y_LPARAM(lParam) / scale;
            if (m_impl->appShell && m_impl->appShell->GetMode() == ::components::AppShellMode::Home) {
                m_impl->appShell->OnMouseDown(dipX, dipY);
            } else if (m_impl->appShell && m_impl->appShell->GetMode() == ::components::AppShellMode::Document) {
                auto canvas = m_impl->documentView ? m_impl->documentView->GetCanvasContainer() : nullptr;
                D2D1_RECT_F cb = canvas ? canvas->GetBounds() : D2D1_RECT_F{0, 0, 0, 0};
                bool insideCanvas = (dipX >= cb.left && dipX <= cb.right && dipY >= cb.top && dipY <= cb.bottom);
                bool isOrg = m_impl->documentView && m_impl->documentView->GetOrganizeView()->IsVisible();
                
                if (insideCanvas && !isOrg) {
                    if (auto viewer = m_impl->GetViewer()) {
                        viewer->OnLButtonDown(dipX, dipY);
                    }
                } else {
                    m_impl->appShell->OnMouseDown(dipX, dipY);
                }
            }
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_LBUTTONUP: {
            UINT dpi = GetDpiForWindow(m_hwnd);
            float scale = dpi > 0 ? (dpi / 96.0f) : 1.0f;
            float dipX = GET_X_LPARAM(lParam) / scale;
            float dipY = GET_Y_LPARAM(lParam) / scale;
            auto viewer = m_impl->GetViewer();
            bool viewerHasCapture = viewer && viewer->GetCaptureService() && viewer->GetCaptureService()->IsAnyCaptured();
            if (viewerHasCapture) {
                viewer->OnLButtonUp(dipX, dipY);
            } else if (m_impl->appShell && m_impl->appShell->GetMode() == ::components::AppShellMode::Home) {
                m_impl->appShell->OnMouseUp(dipX, dipY);
            } else if (m_impl->appShell && m_impl->appShell->GetMode() == ::components::AppShellMode::Document) {
                auto canvas = m_impl->documentView ? m_impl->documentView->GetCanvasContainer() : nullptr;
                D2D1_RECT_F cb = canvas ? canvas->GetBounds() : D2D1_RECT_F{0, 0, 0, 0};
                bool insideCanvas = (dipX >= cb.left && dipX <= cb.right && dipY >= cb.top && dipY <= cb.bottom);
                bool isOrg = m_impl->documentView && m_impl->documentView->GetOrganizeView()->IsVisible();
                if (insideCanvas && viewer && !isOrg) {
                    viewer->OnLButtonUp(dipX, dipY);
                } else {
                    m_impl->appShell->OnMouseUp(dipX, dipY);
                }
            }
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_LBUTTONDBLCLK: {
            UINT dpi = GetDpiForWindow(m_hwnd);
            float scale = dpi > 0 ? (dpi / 96.0f) : 1.0f;
            float dipX = GET_X_LPARAM(lParam) / scale;
            float dipY = GET_Y_LPARAM(lParam) / scale;
            if (m_impl->appShell && m_impl->appShell->GetMode() == ::components::AppShellMode::Home) {
                m_impl->appShell->OnMouseDown(dipX, dipY);
            } else if (m_impl->appShell && m_impl->appShell->GetMode() == ::components::AppShellMode::Document) {
                auto canvas = m_impl->documentView ? m_impl->documentView->GetCanvasContainer() : nullptr;
                D2D1_RECT_F cb = canvas ? canvas->GetBounds() : D2D1_RECT_F{0, 0, 0, 0};
                bool insideCanvas = (dipX >= cb.left && dipX <= cb.right && dipY >= cb.top && dipY <= cb.bottom);
                bool isOrg = m_impl->documentView && m_impl->documentView->GetOrganizeView()->IsVisible();
                
                if (insideCanvas && !isOrg) {
                    if (auto viewer = m_impl->GetViewer()) {
                        viewer->OnLButtonDoubleClick(dipX, dipY);
                    }
                } else {
                    m_impl->appShell->OnMouseDown(dipX, dipY);
                }
            }
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_RBUTTONUP: {
            UINT dpi = GetDpiForWindow(m_hwnd);
            float scale = dpi > 0 ? (dpi / 96.0f) : 1.0f;
            float dipX = GET_X_LPARAM(lParam) / scale;
            float dipY = GET_Y_LPARAM(lParam) / scale;
            if (m_impl->currentView == m_impl->documentView && m_impl->GetViewer()) {
                m_impl->GetViewer()->OnRButtonUp(dipX, dipY);
            }
            return 0;
        }
        case WM_MBUTTONDOWN: {
            UINT dpi = GetDpiForWindow(m_hwnd);
            float scale = dpi > 0 ? (dpi / 96.0f) : 1.0f;
            float dipX = GET_X_LPARAM(lParam) / scale;
            float dipY = GET_Y_LPARAM(lParam) / scale;
            if (m_impl->currentView == m_impl->documentView && m_impl->GetViewer()) {
                m_impl->GetViewer()->OnMButtonDown(dipX, dipY);
            }
            return 0;
        }
        case WM_MBUTTONUP: {
            UINT dpi = GetDpiForWindow(m_hwnd);
            float scale = dpi > 0 ? (dpi / 96.0f) : 1.0f;
            float dipX = GET_X_LPARAM(lParam) / scale;
            float dipY = GET_Y_LPARAM(lParam) / scale;
            if (m_impl->currentView == m_impl->documentView && m_impl->GetViewer()) {
                m_impl->GetViewer()->OnMButtonUp(dipX, dipY);
            }
            return 0;
        }
        case WM_MOUSEHWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (m_impl->currentView == m_impl->documentView && m_impl->GetViewer()) {
                float scaledDelta = ((static_cast<float>(delta) / 120.0f) * 100.0f);
                m_impl->GetViewer()->OnScrollX(scaledDelta);
            }
            return 0;
        }
        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ScreenToClient(m_hwnd, &pt);
            UINT dpi = GetDpiForWindow(m_hwnd);
            float scale = dpi / 96.0f;
            float x = pt.x / scale;
            float y = pt.y / scale;
            bool isOrg = m_impl->documentView && m_impl->documentView->GetOrganizeView()->IsVisible();
            if (m_impl->currentView == m_impl->documentView && m_impl->GetViewer() && !isOrg) {
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    float wheelDelta = static_cast<float>(delta) / 120.0f;
                    float factor = 1.0f + (wheelDelta * 0.1f);
                    m_impl->GetViewer()->OnZoom(factor, x, y);
                } else {
                    m_impl->GetViewer()->OnMouseWheel(static_cast<float>(delta));
                }
            } else if (m_impl->appShell) {
                m_impl->appShell->OnMouseWheel(static_cast<float>(delta));
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_KEYDOWN: {
            bool handled = false;
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                if (wParam == 'O') {
                    OpenFile(); handled = true;
                } else if (wParam == 'S') {
                    if (GetKeyState(VK_SHIFT) & 0x8000) DoSaveAs(m_impl->activeTabIndex);
                    else DoSave(m_impl->activeTabIndex);
                    handled = true;
                } else if (wParam == 'F') {
                    if (m_impl->searchBar) {
                        m_impl->searchBar->Show();
                        m_impl->searchBar->SetFocus();
                    }
                    handled = true;
                } else if (wParam == 'P') {
                    if (auto tab = m_impl->GetActiveTab()) {
                        if (tab->document) {
                            app::PrintManager::PrintDocument(m_hwnd, tab->document);
                        }
                    }
                    handled = true;
                } else if (wParam == 'Z') {
                    if (GetKeyState(VK_SHIFT) & 0x8000) {
                        // Redo (Ctrl+Shift+Z)
                        if (auto tab = m_impl->GetActiveTab()) {
                            if (tab->viewer) {
                                tab->viewer->OnRedo();
                                RefreshAfterPageOp();
                            }
                        }
                    } else {
                        // Undo (Ctrl+Z)
                        if (auto tab = m_impl->GetActiveTab()) {
                            if (tab->viewer) {
                                tab->viewer->OnUndo();
                                RefreshAfterPageOp();
                            }
                        }
                    }
                    handled = true;
                } else if (wParam == 'Y') {
                    // Redo (Ctrl+Y)
                    if (auto tab = m_impl->GetActiveTab()) {
                        if (tab->viewer) {
                            tab->viewer->OnRedo();
                            RefreshAfterPageOp();
                        }
                    }
                    handled = true;
                } else if (wParam == 'V') {
                    if (m_impl->currentView == m_impl->documentView && m_impl->GetViewer()) {
                        m_impl->GetViewer()->OnPaste();
                    }
                    handled = true;
                    if (auto tab = m_impl->GetActiveTab()) {
                        if (tab->viewer) {
                            auto& im = tab->viewer->GetInteractionManager();
                            if (im.IsEditingText()) {
                                tab->viewer->OnKeyDown(wParam);
                            } else {
                                auto sel = im.GetSelection();
                                if (sel.empty()) {
                                    tab->viewer->OnKeyDown(wParam);
                                } else {
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
                                                                bmi->biSizeImage = 0;
                                                                bmi->biXPelsPerMeter = 0;
                                                                bmi->biYPelsPerMeter = 0;
                                                                bmi->biClrUsed = 0;
                                                                bmi->biClrImportant = 0;
                                                                memcpy(mem + headerSize, bitmapData.data(), dataSize);
                                                                GlobalUnlock(hMem);
                                                                SetClipboardData(CF_DIB, hMem);
                                                            }
                                                            CloseClipboard();
                                                            copiedImage = true;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    if (!textToCopy.empty() && !copiedImage) {
                                        core::Clipboard::SetText(m_hwnd, textToCopy);
                                    }
                                }
                            }
                        }
                    }
                    handled = true;
                    if (auto tab = m_impl->GetActiveTab()) {
                        if (tab->viewer) {
                            auto& im = tab->viewer->GetInteractionManager();
                            if (im.IsEditingText()) {
                                tab->viewer->OnKeyDown(wParam);
                            } else {
                                tab->viewer->SelectAllText();
                                tab->viewer->InvalidateView();
                                InvalidateRect(m_hwnd, nullptr, FALSE);
                            }
                        }
                    }
                    handled = true;
                } else if (wParam == VK_OEM_PLUS || wParam == VK_ADD || wParam == '=' || wParam == '+') {
                    if (auto viewer = m_impl->GetViewer()) {
                        viewer->OnZoom(0.2f);
                        if (auto st = m_impl->documentView->GetStatusBar()) {
                            st->SetZoom(static_cast<float>(viewer->GetZoom()));
                        }
                        InvalidateRect(m_hwnd, nullptr, FALSE);
                    }
                    handled = true;
                } else if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT || wParam == '-') {
                    if (auto viewer = m_impl->GetViewer()) {
                        viewer->OnZoom(-0.2f);
                        if (auto st = m_impl->documentView->GetStatusBar()) {
                            st->SetZoom(static_cast<float>(viewer->GetZoom()));
                        }
                        InvalidateRect(m_hwnd, nullptr, FALSE);
                    }
                    handled = true;
                } else if (wParam == '0' || wParam == VK_NUMPAD0) {
                    if (auto viewer = m_impl->GetViewer()) {
                        viewer->ZoomToFitPage();
                        if (auto st = m_impl->documentView->GetStatusBar()) {
                            st->SetZoom(static_cast<float>(viewer->GetZoom()));
                        }
                        InvalidateRect(m_hwnd, nullptr, FALSE);
                    }
                    handled = true;
                } else if (wParam == 'F') {
                    if (m_impl->searchBar) m_impl->searchBar->Show();
                    handled = true;
                } else if (wParam == 'W') {
                    if (m_impl->activeTabIndex >= 0) CloseTab(m_impl->activeTabIndex);
                    handled = true;
                } else if (wParam == 'T') {
                    OpenFile(); handled = true;
                } else if (wParam == 'I') {
                    if (m_impl->GetViewer()) m_impl->GetViewer()->TriggerInsertImage();
                    handled = true;
                }
            } else if (wParam == VK_DELETE) {
                if (m_impl->GetViewer()) {
                    m_impl->GetViewer()->OnKeyDown(VK_DELETE);
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                }
                handled = true;
            }
            if (!handled) {
                if (wParam == VK_ESCAPE) {
                    if (m_impl->GetViewer()) {
                        m_impl->GetViewer()->SetToolMode(ToolMode::Pan);
                        if (auto tb = m_impl->appShell->GetReadingToolbar()) {
                            tb->SetActiveTool(L"Select");
                        }
                    }
                    handled = true;
                }
            }
            if (!handled && m_impl->GetViewer()) {
                m_impl->GetViewer()->OnKeyDown(static_cast<int>(wParam));
            }
            return 0;
        }
        case WM_CHAR:
            if (m_impl->currentView == m_impl->documentView && m_impl->GetViewer()) {
                m_impl->GetViewer()->OnChar(static_cast<wchar_t>(wParam));
            }
            return 0;
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                if (m_impl->currentView == m_impl->documentView && m_impl->GetViewer()) {
                    if (m_impl->GetViewer()->OnSetCursor()) {
                        return TRUE;
                    }
                }
            }
            return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
        case WM_CAPTURECHANGED: {
            if (auto viewer = m_impl->GetViewer()) {
                viewer->CancelActiveInteractions();
            }
            return 0;
        }
        case WM_DPICHANGED: {
            auto lprcNewScale = reinterpret_cast<RECT*>(lParam);
            SetWindowPos(m_hwnd, nullptr,
                lprcNewScale->left,
                lprcNewScale->top,
                lprcNewScale->right - lprcNewScale->left,
                lprcNewScale->bottom - lprcNewScale->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
            if (m_impl->renderTarget) {
                UINT newDpi = HIWORD(wParam);
                m_impl->renderTarget->SetDpi(static_cast<float>(newDpi), static_cast<float>(newDpi));
            }
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_APP_TILE_READY: {
            auto* result = reinterpret_cast<core::models::RenderResult*>(wParam);
            if (result) {
                if (auto viewer = m_impl->GetViewer()) {
                    viewer->OnTileReady(result, m_impl->renderTarget);
                }
                if (m_impl->documentView) {
                    if (auto sidebar = m_impl->documentView->GetLeftSidebar()) {
                        sidebar->OnTileReady(result, m_impl->renderTarget);
                    }
                    if (auto orgView = m_impl->documentView->GetOrganizeView()) {
                        if (auto grid = orgView->GetGrid()) {
                            grid->OnTileReady(result, m_impl->renderTarget);
                        }
                    }
                }
                delete result;
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;
        }
                case WM_APP_FILE_LOADED: {
            auto* result = reinterpret_cast<AsyncLoadResult*>(wParam);
            if (result->success) {
                this->FinishOpenFileDirect(result->path, result->doc, std::move(result->bookmarks), std::move(result->pageSizes));
            } else {
                ::ui::dialogs::MessageDialog::Show(m_hwnd, L"PDF Elite", (L"Failed to open: " + result->path).c_str(), ::ui::dialogs::MessageDialogType::Ok);
            }
            delete result;
            return 0;
        }
case WM_APP_SEARCH_COMPLETE: {
            auto tab = m_impl->GetActiveTab();
            if (tab) {
                auto session = core::TabManager::Instance().GetSession(tab->filePath);
                if (session) {
                    auto results = session->GetSearchResults();
                    int idx = session->GetActiveSearchIndex();
                    if (m_impl->searchBar) {
                        m_impl->searchBar->SetResultCount(static_cast<int>(results.size()), results.empty() ? 0 : idx + 1);
                    }
                }
            }
            return 0;
        }
        case WM_TIMER: {
            if (wParam == TOOLTIP_TIMER_ID) {
                KillTimer(m_hwnd, TOOLTIP_TIMER_ID);
                if (!m_impl->currentTooltipText.empty()) {
                    m_impl->tooltipVisible = true;
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                }
            } else if (wParam == TIMER_RELOAD_OBJECTS) {
                KillTimer(m_hwnd, TIMER_RELOAD_OBJECTS);
                if (auto activeTab = m_impl->GetActiveTab()) {
                    if (activeTab->viewer) {
                        activeTab->viewer->ReloadInteractableObjects();
                        activeTab->viewer->InvalidateView();
                    }
                }
            } else if (wParam == TIMER_PHYSICS) {
                if (auto activeTab = m_impl->GetActiveTab()) {
                    if (activeTab->viewer) {
                        if (activeTab->viewer->UpdatePhysics()) {
                            InvalidateRect(m_hwnd, nullptr, FALSE);
                        }
                    }
                }
            }
            return 0;
        }
        case WM_CLOSE: {
            bool canClose = true;
            for (int i = 0; i < static_cast<int>(m_impl->tabs.size()); ++i) {
                if (!PromptSaveChanges(i)) {
                    canClose = false;
                    break;
                }
            }
            if (canClose) {
                DestroyWindow(m_hwnd);
            }
            return 0;
        }
        case WM_DESTROY:
            KillTimer(m_hwnd, TIMER_PHYSICS);
            PostQuitMessage(0);
            return 0;
        case WM_DROPFILES: {
            HDROP hDrop = reinterpret_cast<HDROP>(wParam);
            UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
            for (UINT i = 0; i < count; ++i) {
                wchar_t path[MAX_PATH] = {0};
                if (DragQueryFileW(hDrop, i, path, MAX_PATH)) {
                    OpenFileDirect(path);
                }
            }
            DragFinish(hDrop);
            return 0;
        }
        default:
            return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
        }
    }
    catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Exception in WindowProc", MB_OK);
        return 0;
    }
}

void MainWindow::SwitchToTab(int index) {
    if (index < 0 || index >= static_cast<int>(m_impl->tabs.size())) return;
    m_impl->activeTabIndex = index;
    m_impl->UpdateTabs();
    
    // Sync OrganizeView
    if (auto orgView = m_impl->documentView ? m_impl->documentView->GetOrganizeView() : nullptr) {
        auto tab = m_impl->GetActiveTab();
        if (tab) {
            orgView->SetDocumentId(tab->filePath);
            orgView->SetDocument(tab->document);
        }
    }
    
    // Fix tab-leaking: Sync the shared UI toolbar state with the per-tab viewer's ToolStateMachine state
    if (auto tb = m_impl->appShell->GetReadingToolbar()) {
        auto tab = m_impl->GetActiveTab();
        if (tab && tab->viewer) {
            auto tsm = tab->viewer->GetToolStateMachine();
            if (tsm) {
                ui::tools::ToolType tt = tsm->GetActiveToolType();
                if (tt == ui::tools::ToolType::Pan) tb->SetActiveTool(L"Select");
                else if (tt == ui::tools::ToolType::Select) tb->SetActiveTool(L"Select");
                // Add more mappings as necessary
            }
        }
    }
    
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void MainWindow::FinishOpenFileDirect(const std::wstring& path, std::shared_ptr<core::interfaces::dom::IDocument> sharedDoc, std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> cachedBookmarks, std::vector<std::pair<float,float>> cachedPageSizes) {
    LogPipeline("MainWindow::FinishOpenFileDirect called");
    if (sharedDoc) {
        LogPipeline("Result has value, creating DocumentTab");
        auto tab = std::make_unique<DocumentTab>();
        tab->filePath = path;
        size_t slashPos = path.find_last_of(L"/\\");
        tab->title = (slashPos != std::wstring::npos) ? path.substr(slashPos + 1) : path;
        tab->document = sharedDoc;

        LogPipeline("Creating PdfViewer");
        tab->viewer = std::make_unique<PdfViewer>();
        LogPipeline("PdfViewer::Initialize");
        tab->viewer->Initialize(m_hwnd);
        tab->viewer->onPageChanged = [this](int current, int total) {
            if (auto status = m_impl->documentView->GetStatusBar()) {
                status->SetPageInfo(current, total);
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        };
        LogPipeline("PdfViewer::SetDocumentId");
        tab->viewer->SetDocumentId(path);
        LogPipeline("PdfViewer::SetDocument");
        tab->viewer->SetCachedPageSizes(cachedPageSizes);
        tab->viewer->SetDocument(sharedDoc);

        LogPipeline("Creating EngineAdapter");
        auto engine = std::make_shared<pdf_engine::EngineAdapter>(sharedDoc);
        LogPipeline("Registering with RenderController");
        core::RenderController::Instance().RegisterDocument(path, engine);

        LogPipeline("Setting up InteractionManager callbacks");
        tab->viewer->GetInteractionManager().onSelectionChanged = [this]() {
            auto t = m_impl->GetActiveTab();
            if (!t || !t->viewer) return;

            auto sel = t->viewer->GetInteractionManager().GetSelection();
            
            if (auto props = m_impl->documentView ? m_impl->documentView->GetPropertiesPanel() : nullptr) {
                if (sel.size() == 1 && std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(sel[0])) {
                    props->SetSelectedObject(sel[0]);
                    if (!t->viewer->IsRightClickProcessing()) props->SetVisible(true);
                } else {
                    props->SetSelectedObject(nullptr);
                    if (!t->viewer->IsRightClickProcessing()) props->SetVisible(false);
                }
                RECT rc;
                if (m_hwnd && GetClientRect(m_hwnd, &rc)) {
                    UINT dpi = GetDpiForWindow(m_hwnd);
                    float scale = dpi > 0 ? (dpi / 96.0f) : 1.0f;
                    if (m_impl->appShell) {
                        m_impl->appShell->Layout(D2D1::RectF(0, 0, static_cast<float>(rc.right) / scale, static_cast<float>(rc.bottom) / scale));
                    }
                    if (auto viewer = m_impl->GetViewer()) {
                        viewer->OnResize(m_impl->documentView->GetCanvasContainer()->GetBounds());
                    }
                }
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        };

        LogPipeline("MainWindow::OpenFileDirect TRACE_OFD 4 push tab");
        m_impl->tabs.push_back(std::move(tab));
        m_impl->activeTabIndex = static_cast<int>(m_impl->tabs.size()) - 1;

        m_impl->currentView = m_impl->documentView;
        m_impl->appShell->SetMode(::components::AppShellMode::Document);
        m_impl->UpdateTabs();

        LogPipeline("MainWindow::OpenFileDirect TRACE_OFD 5 bookmarks");
        if (m_impl->bookmarkPanel) {
            m_impl->bookmarkPanel->LoadBookmarks(cachedBookmarks);
            m_impl->bookmarkPanel->SetOnNavigateCallback([this](const core::interfaces::dom::NavigationTarget& tgt) {
                if (auto t = m_impl->GetActiveTab()) {
                    if (t->viewer) {
                        t->viewer->GoToPage(tgt.pageIndex);
                        if (auto status = m_impl->documentView->GetStatusBar()) {
                            status->SetPageInfo(tgt.pageIndex, t->document->PageCount());
                        }
                    }
                }
            });
            m_impl->bookmarkPanel->SetOnAddBookmarkCallback([this, sharedDoc]() {
                if (auto t = m_impl->GetActiveTab()) {
                    if (t->viewer) {
                        ::ui::dialogs::MessageDialog::Show(m_hwnd, L"Add Bookmark", L"Adding bookmarks is not currently supported by the underlying PDF engine.", ::ui::dialogs::MessageDialogType::Ok);
                    }
                }
            });
        }

        LogPipeline("MainWindow::OpenFileDirect TRACE_OFD 6 sidebar");
        if (auto sidebar = m_impl->documentView->GetLeftSidebar()) {
            sidebar->SetDocumentId(path);
            sidebar->SetDocument(sharedDoc);
            sidebar->onPageSelected = [this](int pageIndex) {
                if (auto t = m_impl->GetActiveTab()) {
                    if (t->viewer) {
                        t->viewer->GoToPage(pageIndex);
                        if (auto status = m_impl->documentView->GetStatusBar()) {
                            status->SetPageInfo(pageIndex, t->document->PageCount());
                        }
                    }
                }
            };
        }
        if (auto orgView = m_impl->documentView->GetOrganizeView()) {
            orgView->SetDocumentId(path);
            orgView->SetDocument(sharedDoc);
        }
        if (auto sidebar = m_impl->documentView->GetLeftSidebar()) {
            sidebar->onPageMoved = [this](int sourceIndex, int destIndex) {
                if (auto t = m_impl->GetActiveTab()) {
                    if (t->document) {
                        auto cmd = std::make_unique<pdf_engine::commands::MovePageCommand>(
                            static_cast<PdfDocument*>(t->document.get()), sourceIndex, destIndex);
                        t->document->GetCommandStack().ExecuteCommand(std::move(cmd));
                        RefreshAfterPageOp();
                    }
                }
            };
        }
        if (auto status = m_impl->documentView->GetStatusBar()) {
            status->SetPageInfo(0, sharedDoc->PageCount());
        }

        LogPipeline("MainWindow::OpenFileDirect TRACE_OFD 7 recent files");
        core::RecentFilesManager::Instance().AddFile(path);
        LogPipeline("MainWindow::OpenFileDirect finished successfully");

        printf("[TRACE_OFD 8: resize message]\n"); fflush(stdout);
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        SendMessageW(m_hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
        InvalidateRect(m_hwnd, nullptr, FALSE);
        printf("[TRACE_OFD 9: finished]\n"); fflush(stdout);
    }
}

void MainWindow::OpenFile() {
    OPENFILENAMEW ofn = {};
    wchar_t szFile[260] = {0};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = m_hwnd;
    ofn.lpstrFile    = szFile;
    ofn.nMaxFile     = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter  = L"PDF Files\0*.pdf\0All\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    LogPipeline("MainWindow::OpenFile before GetOpenFileNameW");
    if (GetOpenFileNameW(&ofn)) {
        LogPipeline("MainWindow::OpenFile GetOpenFileNameW returned success");
        OpenFileDirect(ofn.lpstrFile);
    } else {
        LogPipeline("MainWindow::OpenFile GetOpenFileNameW returned false");
    }
}

void MainWindow::RefreshAfterPageOp() {
    auto tab = m_impl->GetActiveTab();
    if (!tab) return;
    int current = tab->viewer->GetCurrentPage();
    int pageCount = tab->document->PageCount();

    if (auto sidebar = m_impl->documentView->GetLeftSidebar()) {
        sidebar->SetDocumentId(tab->filePath);
        sidebar->SetDocument(tab->document);
    }
    if (auto status = m_impl->documentView->GetStatusBar()) {
        status->SetPageInfo(current, pageCount);
    }
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

bool MainWindow::DoSave(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= static_cast<int>(m_impl->tabs.size())) return false;
    auto& tab = m_impl->tabs[tabIndex];
    if (!tab->document) return false;
    if (tab->filePath.empty()) return DoSaveAs(tabIndex);

    DWORD attrs = GetFileAttributesW(tab->filePath.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
        return DoSaveAs(tabIndex);
    }

    if (m_impl->GetViewer()) {
        m_impl->GetViewer()->InvalidateCaches();
    }
    tab->document->InvalidateOpenPages();

    bool ok = tab->document->SaveAs(tab->filePath.c_str());
    if (ok) {
        tab->dirty = false;
        m_impl->UpdateTabs();
    } else {
        ::ui::dialogs::MessageDialog::Show(m_hwnd, L"Save", L"Failed to save the document.", ::ui::dialogs::MessageDialogType::Ok);
    }
    return ok;
}

bool MainWindow::DoSaveAs(int tabIndex) { (void)tabIndex;
    auto tab = m_impl->GetActiveTab();
    if (!tab || !tab->document) return false;

    OPENFILENAMEW ofn = {};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = m_hwnd;
    wchar_t szFile[MAX_PATH] = {0};
    if (!tab->filePath.empty()) {
        wcsncpy_s(szFile, tab->filePath.c_str(), MAX_PATH - 1);
    }
    ofn.lpstrFile    = szFile;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrFilter  = L"PDF Documents (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags        = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt  = L"pdf";
    if (GetSaveFileNameW(&ofn)) {
        if (m_impl->GetViewer()) {
            m_impl->GetViewer()->InvalidateCaches();
        }
        tab->document->InvalidateOpenPages();
        bool ok = tab->document->SaveAs(ofn.lpstrFile);
        if (ok) {
            tab->filePath = ofn.lpstrFile;
            std::wstring pathStr = tab->filePath;
            size_t slashPos = pathStr.find_last_of(L"/\\");
            tab->title = (slashPos != std::wstring::npos) ? pathStr.substr(slashPos + 1) : pathStr;
            tab->dirty = false;
            m_impl->UpdateTabs();
        } else {
            ::ui::dialogs::MessageDialog::Show(m_hwnd, L"Save As", L"Failed to save the document.", ::ui::dialogs::MessageDialogType::Ok);
        }
        return ok;
    }
    return false;
}

bool MainWindow::PromptSaveChanges(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= static_cast<int>(m_impl->tabs.size())) return true;
    auto& tab = m_impl->tabs[tabIndex];
    if (tab->dirty) {
        std::wstring msg = L"Do you want to save changes to " + tab->title + L"?";
        auto result = ::ui::dialogs::MessageDialog::Show(m_hwnd, L"PDF Elite", msg.c_str(), ::ui::dialogs::MessageDialogType::YesNoCancel);
        if (result == ::ui::dialogs::MessageDialogResult::Cancel) return false;
        if (result == ::ui::dialogs::MessageDialogResult::Yes) {
            return DoSave(tabIndex);
        }
    }
    return true;
}

void MainWindow::CloseTab(int tabIndex) {
    if (!PromptSaveChanges(tabIndex)) return;
    if (tabIndex < 0 || tabIndex >= static_cast<int>(m_impl->tabs.size())) return;

    // Unregister from RenderController
    core::RenderController::Instance().UnregisterDocument(m_impl->tabs[tabIndex]->filePath);

    if (m_impl->tabs[tabIndex] && m_impl->tabs[tabIndex]->viewer) {
        m_impl->tabs[tabIndex]->viewer->GetInteractionManager().onSelectionChanged = nullptr;
    }

    if (auto sidebar = m_impl->documentView ? m_impl->documentView->GetLeftSidebar() : nullptr) {
        sidebar->SetDocument(nullptr);
    }
    if (auto orgView = m_impl->documentView ? m_impl->documentView->GetOrganizeView() : nullptr) {
        orgView->SetDocument(nullptr);
    }

    m_impl->tabs.erase(m_impl->tabs.begin() + tabIndex);

    if (m_impl->tabs.empty()) {
        m_impl->activeTabIndex = -1;
        m_impl->currentView = m_impl->homeView;
        if (m_impl->appShell) m_impl->appShell->SetMode(::components::AppShellMode::Home);
        if (m_impl->searchBar) m_impl->searchBar->Hide();
        if (m_impl->bookmarkPanel) m_impl->bookmarkPanel->Hide();
        m_impl->showBookmarks = false;
        if (m_impl->documentView && m_impl->documentView->GetPropertiesPanel()) {
            m_impl->documentView->GetPropertiesPanel()->SetVisible(false);
            m_impl->documentView->GetPropertiesPanel()->SetSelectedObject(nullptr);
        }
    } else {
        if (m_impl->activeTabIndex >= static_cast<int>(m_impl->tabs.size())) {
            m_impl->activeTabIndex = static_cast<int>(m_impl->tabs.size() - 1);
        }
        if (auto sidebar = m_impl->documentView ? m_impl->documentView->GetLeftSidebar() : nullptr) {
            auto activeTab = m_impl->GetActiveTab();
            if (activeTab) {
                sidebar->SetDocumentId(activeTab->filePath);
                sidebar->SetDocument(activeTab->document);
            }
        }
        if (auto orgView = m_impl->documentView ? m_impl->documentView->GetOrganizeView() : nullptr) {
            auto activeTab = m_impl->GetActiveTab();
            if (activeTab) {
                orgView->SetDocumentId(activeTab->filePath);
                orgView->SetDocument(activeTab->document);
            }
        }
    }
    m_impl->UpdateTabs();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void MainWindow::DoAddLink() {
    if (auto tab = m_impl->GetActiveTab()) {
        if (tab->document && tab->viewer) {
            ui::dialogs::LinkParams dlgParams;
            dlgParams.pageIndex  = tab->viewer->GetCurrentPage();
            dlgParams.totalPages = tab->document->PageCount();
            if (ui::dialogs::LinkDialog::Show(m_hwnd, dlgParams)) {
                pdf_engine::commands::LinkParams cmdParams;
                cmdParams.pageIndex   = dlgParams.pageIndex;
                cmdParams.x           = dlgParams.x;
                cmdParams.y           = dlgParams.y;
                cmdParams.width       = dlgParams.width;
                cmdParams.height      = dlgParams.height;
                cmdParams.isUrl       = dlgParams.isUrl;
                cmdParams.url         = dlgParams.url;
                cmdParams.targetPage  = dlgParams.targetPage - 1; // 0-based
                cmdParams.drawBorder  = dlgParams.drawBorder;
                cmdParams.borderColor = dlgParams.borderColor;
                auto cmd = std::make_unique<pdf_engine::commands::AddLinkCommand>(
                    tab->document, cmdParams);
                tab->document->GetCommandStack().ExecuteCommand(std::move(cmd));
            }
        }
    }
}

void MainWindow::DoAddBackground() {
    if (auto tab = m_impl->GetActiveTab()) {
        if (tab->document && tab->viewer) {
            ui::dialogs::BackgroundParams dlgParams;
            dlgParams.currentPage = tab->viewer->GetCurrentPage() + 1; // 1-based
            dlgParams.totalPages  = tab->document->PageCount();
            if (ui::dialogs::BackgroundDialog::Show(m_hwnd, dlgParams)) {
                pdf_engine::commands::BackgroundParams cmdParams;
                cmdParams.isColor    = dlgParams.isColor;
                cmdParams.color      = dlgParams.color;
                cmdParams.opacity    = dlgParams.opacity;
                cmdParams.imagePath  = dlgParams.imagePath;
                cmdParams.pageScope  = dlgParams.pageScope;
                cmdParams.pageRange  = dlgParams.pageRange;
                cmdParams.currentPage = dlgParams.currentPage;
                auto cmd = std::make_unique<pdf_engine::commands::AddBackgroundCommand>(
                    tab->document, cmdParams);
                if (tab->document->GetCommandStack().ExecuteCommand(std::move(cmd))) {
                    tab->viewer->InvalidateView();
                }
            }
        }
    }
}

void MainWindow::DoAddWatermark() {
    if (auto tab = m_impl->GetActiveTab()) {
        if (tab->document && tab->viewer) {
            ui::dialogs::WatermarkParams dlgParams;
            dlgParams.currentPage = tab->viewer->GetCurrentPage() + 1;
            dlgParams.totalPages  = tab->document->PageCount();
            if (ui::dialogs::WatermarkDialog::Show(m_hwnd, dlgParams)) {
                pdf_engine::commands::WatermarkParams cmdParams;
                cmdParams.text          = dlgParams.text;
                cmdParams.fontName      = dlgParams.fontName;
                cmdParams.fontSize      = dlgParams.fontSize;
                cmdParams.bold          = dlgParams.bold;
                cmdParams.italic        = dlgParams.italic;
                cmdParams.color         = dlgParams.color;
                cmdParams.opacity       = dlgParams.opacity;
                cmdParams.rotation      = dlgParams.rotation;
                cmdParams.positionIndex = dlgParams.positionIndex;
                cmdParams.layerOver     = dlgParams.layerOver;
                cmdParams.pageScope     = dlgParams.pageScope;
                cmdParams.pageRange     = dlgParams.pageRange;
                cmdParams.currentPage   = dlgParams.currentPage;
                auto cmd = std::make_unique<pdf_engine::commands::AddWatermarkCommand>(
                    tab->document, cmdParams);
                if (tab->document->GetCommandStack().ExecuteCommand(std::move(cmd))) {
                    tab->viewer->InvalidateView();
                }
            }
        }
    }
}

void MainWindow::DoAddHeaderFooter() {
    if (auto tab = m_impl->GetActiveTab()) {
        if (tab->document && tab->viewer) {
            ui::dialogs::HeaderFooterParams dlgParams;
            dlgParams.currentPage = tab->viewer->GetCurrentPage() + 1;
            dlgParams.totalPages  = tab->document->PageCount();
            if (ui::dialogs::HeaderFooterDialog::Show(m_hwnd, dlgParams)) {
                pdf_engine::commands::HeaderFooterParams cmdParams;
                cmdParams.leftHeader    = dlgParams.leftHeader;
                cmdParams.centerHeader  = dlgParams.centerHeader;
                cmdParams.rightHeader   = dlgParams.rightHeader;
                cmdParams.leftFooter    = dlgParams.leftFooter;
                cmdParams.centerFooter  = dlgParams.centerFooter;
                cmdParams.rightFooter   = dlgParams.rightFooter;
                cmdParams.fontName      = dlgParams.fontName;
                cmdParams.fontSize      = dlgParams.fontSize;
                cmdParams.color         = dlgParams.color;
                cmdParams.topMargin     = dlgParams.topMargin;
                cmdParams.bottomMargin  = dlgParams.bottomMargin;
                cmdParams.leftMargin    = dlgParams.leftMargin;
                cmdParams.rightMargin   = dlgParams.rightMargin;
                cmdParams.pageScope     = dlgParams.pageScope;
                cmdParams.pageRange     = dlgParams.pageRange;
                cmdParams.startPageNum  = dlgParams.startPageNum;
                cmdParams.currentPage   = dlgParams.currentPage;
                auto cmd = std::make_unique<pdf_engine::commands::AddHeaderFooterCommand>(
                    tab->document, cmdParams);
                if (tab->document->GetCommandStack().ExecuteCommand(std::move(cmd))) {
                    tab->viewer->InvalidateView();
                }
            }
        }
    }
}

void MainWindow::DoCreatePdf() {
    ui::dialogs::CreateBlankParams dlgParams;
    if (ui::dialogs::CreateBlankDialog::Show(m_hwnd, dlgParams)) {
        pdf_engine::operations::CreateBlankParams opParams;
        opParams.pageSizeIndex = dlgParams.pageSizeIndex;
        opParams.widthPt       = dlgParams.widthPt;
        opParams.heightPt      = dlgParams.heightPt;
        opParams.isPortrait    = dlgParams.isPortrait;
        opParams.pageCount     = dlgParams.pageCount;

        OPENFILENAMEW ofn = {};
        ofn.lStructSize  = sizeof(ofn);
        ofn.hwndOwner    = m_hwnd;
        wchar_t szFile[MAX_PATH] = {0};
        ofn.lpstrFile    = szFile;
        ofn.nMaxFile     = MAX_PATH;
        ofn.lpstrFilter  = L"PDF Documents (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags        = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt  = L"pdf";
        if (GetSaveFileNameW(&ofn)) {
            opParams.outputPath = ofn.lpstrFile;
            if (pdf_engine::operations::CreateBlankPdfFile(opParams)) {
                OpenFileDirect(opParams.outputPath);
            } else {
                ::ui::dialogs::MessageDialog::Show(m_hwnd, L"Create PDF", L"Failed to create blank PDF.", ::ui::dialogs::MessageDialogType::Ok);
            }
        }
    }
}

void MainWindow::DoExtractImages() {
    if (auto tab = m_impl->GetActiveTab()) {
        if (tab->document && tab->viewer) {
            ui::dialogs::ExtractImagesParams dlgParams;
            dlgParams.srcPdfPath  = tab->filePath;
            dlgParams.currentPage = tab->viewer->GetCurrentPage() + 1;
            dlgParams.totalPages  = tab->document->PageCount();
            if (ui::dialogs::ExtractImagesDialog::Show(m_hwnd, dlgParams)) {
                pdf_engine::operations::ExtractImagesParams opParams;
                opParams.srcPdfPath  = dlgParams.srcPdfPath;
                opParams.outputDir   = dlgParams.outputDir;
                opParams.format      = dlgParams.format;
                opParams.prefix      = dlgParams.prefix;
                opParams.pageScope   = dlgParams.pageScope;
                opParams.pageRange   = dlgParams.pageRange;
                opParams.currentPage = dlgParams.currentPage;
                opParams.totalPages  = dlgParams.totalPages;
                int count = pdf_engine::operations::ExtractImagesFromDocument(
                    tab->document, opParams);
                if (count > 0) {
                    std::wstring msg = L"Extracted " + std::to_wstring(count) + L" image(s) to:\n" + dlgParams.outputDir;
                    ::ui::dialogs::MessageDialog::Show(m_hwnd, L"Extract Images", msg.c_str(), ::ui::dialogs::MessageDialogType::Ok);
                } else if (count == 0) {
                    ::ui::dialogs::MessageDialog::Show(m_hwnd, L"Extract Images", L"No images found in the selected pages.", ::ui::dialogs::MessageDialogType::Ok);
                } else {
                    ::ui::dialogs::MessageDialog::Show(m_hwnd, L"Extract Images", L"Failed to extract images.", ::ui::dialogs::MessageDialogType::Ok);
                }
            }
        }
    }
}

void MainWindow::DoCombineFiles() {
    ui::dialogs::CombineParams dlgParams;
    if (auto tab = m_impl->GetActiveTab()) {
        if (!tab->filePath.empty()) {
            dlgParams.sourceFiles.push_back(tab->filePath);
        }
    }
    if (ui::dialogs::CombinePdfDialog::Show(m_hwnd, dlgParams)) {
        pdf_engine::operations::CombineParams opParams;
        opParams.sourceFiles    = dlgParams.sourceFiles;
        opParams.outputFile     = dlgParams.outputFile;
        opParams.openAfterMerge = dlgParams.openAfterMerge;
        if (pdf_engine::operations::CombinePdfDocuments(opParams)) {
            if (dlgParams.openAfterMerge && !dlgParams.outputFile.empty()) {
                OpenFileDirect(dlgParams.outputFile);
            }
        } else {
            ::ui::dialogs::MessageDialog::Show(m_hwnd, L"Combine PDF", L"Failed to combine PDF files.", ::ui::dialogs::MessageDialogType::Ok);
        }
    }
}

#include <thread>
void MainWindow::OpenFileDirect(const std::wstring& path) {
    auto loadingDlg = new AsyncLoadResult(path);
    std::thread([this, loadingDlg]() {
        auto result = PdfDocument::LoadFromFile(loadingDlg->path.c_str());
        if (result.has_value()) {
            loadingDlg->doc = std::move(result.value);
            // Pre-load bookmarks on the background thread since it's expensive!
            loadingDlg->bookmarks = loadingDlg->doc->GetBookmarks();
            // Pre-cache all page sizes on background thread (avoids FPDF_LoadPage per page on UI thread)
            int pgCount = loadingDlg->doc->PageCount();
            loadingDlg->pageSizes.resize(pgCount);
            for (int pi = 0; pi < pgCount; ++pi) {
                auto sz = loadingDlg->doc->GetPageSize(pi);
                loadingDlg->pageSizes[pi] = {sz.width, sz.height};
            }
            loadingDlg->success = true;
        }
        PostMessageW(m_hwnd, WM_APP_FILE_LOADED, reinterpret_cast<WPARAM>(loadingDlg), 0);
    }).detach();
}


