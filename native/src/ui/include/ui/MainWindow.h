#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <string>
#include <memory>
#include <vector>
#include <utility>
#include "core/interfaces/dom/IDocument.h"
#include "core/interfaces/dom/Navigation.h"

class MainWindowImpl;

class MainWindow {
public:
    MainWindow();
    void OpenFileDirect(const std::wstring& path);
    void FinishOpenFileDirect(const std::wstring& path, std::shared_ptr<core::interfaces::dom::IDocument> sharedDoc, std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> cachedBookmarks, std::vector<std::pair<float,float>> cachedPageSizes);
    void SwitchToTab(int index);
    bool DoSave(int tabIndex);
    bool DoSaveAs(int tabIndex);
    bool PromptSaveChanges(int tabIndex);
    void CloseTab(int tabIndex);
    ~MainWindow();

    bool Create(const std::wstring& title, int width, int height);
    void Show(int nCmdShow);
    HWND GetHwnd() const { return m_hwnd; }

protected:
    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    void OpenFile();
    // Refreshes the sidebar thumbnails, status-bar page count and bookmark
    // panel to match the active document after a structural page operation
    // (rotate / delete / insert). The viewer refreshes its own canvas.
    void RefreshAfterPageOp();

    void DoAddLink();
    void DoAddBackground();
    void DoAddWatermark();
    void DoAddHeaderFooter();
    void DoCreatePdf();
    void DoExtractImages();
    void DoCombineFiles();

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = nullptr;
    std::wstring m_className;
    MainWindowImpl* m_impl;
};
