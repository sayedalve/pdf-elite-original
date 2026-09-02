add_library(pdf_elite_ui
    src/MainWindow.cpp
    src/GraphicsDevice.cpp
    src/NativeDesignSystem.cpp
    src/PdfViewer.cpp
    src/TileCache.cpp
    src/input/PointerCaptureService.cpp
    src/input/InputRouter.cpp
    src/tools/ToolStateMachine.cpp
    src/tools/PanTool.cpp
    src/tools/SelectTool.cpp
    src/tools/TextSelectTool.cpp
    src/tools/ShapeTool.cpp
    src/tools/MarkupTool.cpp
    src/tools/InkTool.cpp
    src/tools/FreeTextTool.cpp
    src/tools/StickyNoteTool.cpp
    src/tools/StampTool.cpp
    src/tools/EraserTool.cpp
    src/tools/InsertImageTool.cpp
    src/selection/SelectionModel.cpp
    src/selection/TransformHandles.cpp
    src/selection/CursorResolver.cpp
    
    src/ThemeManager.cpp
    src/framework/UIElement.cpp
    src/framework/Panel.cpp
    src/layout/TextLayout.cpp
    src/controls/IconButton.cpp
    src/controls/QuickToolCard.cpp
    src/controls/RecentFileRow.cpp
    src/controls/SearchBox.cpp
    src/controls/TextEditor.cpp
    src/controls/IconSystem.cpp
    src/components/AppShell.cpp
    src/components/Sidebar.cpp
    src/components/SidebarItem.cpp
    src/components/ThumbnailViewer.cpp
    src/components/Toolbar.cpp
    src/components/ToolPopups.cpp
    src/components/TabBar.cpp
    src/components/ModeRail.cpp
    src/components/StatusBar.cpp
    src/components/SearchBar.cpp
    src/components/PropertiesPanel.cpp
    src/components/BookmarkPanel.cpp
    src/views/HomeView.cpp
    src/views/DocumentView.cpp
    src/views/OrganizeView.cpp
    src/components/OrganizeGrid.cpp
    src/components/OrganizeToolbar.cpp


    src/interaction/InteractionManager.cpp
    src/interaction/AnnotationSelectableObject.cpp
    src/interaction/ImageSelectableObject.cpp
    src/interaction/TextSelectableObject.cpp
    src/annotation/TextAnnotationHandler.cpp
    src/annotation/HighlightAnnotationHandler.cpp
    src/annotation/InkAnnotationHandler.cpp
    src/annotation/ShapeAnnotationHandler.cpp
    src/annotation/FreeTextAnnotationHandler.cpp
    src/annotation/AnnotationHandlerFactory.cpp
    src/utils/WicImageLoader.cpp

    src/dialogs/LinkDialog.cpp
    src/dialogs/BackgroundDialog.cpp
    src/dialogs/WatermarkDialog.cpp
    src/dialogs/HeaderFooterDialog.cpp
    src/dialogs/CreateBlankDialog.cpp
    src/dialogs/ExtractImagesDialog.cpp
    src/dialogs/CombinePdfDialog.cpp
    src/dialogs/ExtractDialog.cpp
    src/dialogs/SplitDialog.cpp
    src/dialogs/InsertDialog.cpp
    src/dialogs/ModernDialog.cpp
    src/dialogs/CropDialog.cpp
    src/dialogs/PageSizeDialog.cpp
    src/dialogs/MessageDialog.cpp

    src/viewport/ViewportEngine.cpp
    src/viewport/KineticScrollFilter.cpp
    src/menu/ContextMenuManager.cpp
    src/search/SearchHighlightOverlay.cpp
)

target_include_directories(pdf_elite_ui PUBLIC 
    include
    src
)

target_link_libraries(pdf_elite_ui PUBLIC
    pdf_elite_core
    pdf_elite_engine
    d2d1
    dwrite
    windowscodecs
    dwmapi
    comctl32
    ole32
    shlwapi
    comdlg32
)


