cmake_minimum_required(VERSION 3.28)
project(PdfElite
    VERSION 3.0.0
    DESCRIPTION "PDF Elite — Native Windows PDF Viewer & Editor"
    LANGUAGES CXX
)

# --- Options ---
option(PDFELITE_BUILD_TESTS "Build unit and integration tests" ON)
option(PDFELITE_USE_LTO "Enable link-time optimization in Release" ON)
option(PDFELITE_PDFIUM_PATH "Path to pre-built PDFium (auto-detected if empty)" "")

# --- Standards ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Shared flags ---
include(cmake/CompilerFlags.cmake)

# --- PDFium ---
include(cmake/PdfiumSetup.cmake)

# --- Subdirectories ---
add_subdirectory(src/core)
add_subdirectory(src/pdf_engine)
add_subdirectory(src/ui)
add_subdirectory(src/app)

# Set up tests directory if needed
if(PDFELITE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# --- Packaging ---
set(CPACK_PACKAGE_NAME "PDF Elite")
set(CPACK_PACKAGE_VENDOR "PDF Elite Contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "PDF Elite Native Viewer and Editor")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_FILE_NAME "PDFEliteSetup")

set(CPACK_GENERATOR "ZIP;NSIS")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
set(CPACK_NSIS_DISPLAY_NAME "PDF Elite")
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "PDFElite.exe")

include(CPack)
