add_library(pdf_elite_core
    CommandStack.cpp
    Clipboard.cpp
    CoordinateConverter.cpp
    ConfigurationManager.cpp
    RecentFilesManager.cpp
    TabManager.cpp
    DocumentController.cpp
    RenderController.cpp
)
target_include_directories(pdf_elite_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)



