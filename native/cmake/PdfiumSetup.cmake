# cmake/PdfiumSetup.cmake
include(FetchContent)

if(NOT PDFELITE_PDFIUM_PATH)
    set(PDFELITE_PDFIUM_PATH "${CMAKE_SOURCE_DIR}/third_party/pdfium")
endif()

if(NOT EXISTS "${PDFELITE_PDFIUM_PATH}/include/fpdfview.h")
    message(FATAL_ERROR 
        "PDFium could not be found at: ${PDFELITE_PDFIUM_PATH}\n"
        "This project requires a pre-built PDFium binary to compile.\n"
        "Please download the latest 'pdfium-win-x64.tgz' from https://github.com/bblanchon/pdfium-binaries/releases\n"
        "and extract it into native/third_party/pdfium/ so that 'native/third_party/pdfium/include/fpdfview.h' exists."
    )
endif()

message(STATUS "Using local PDFium at ${PDFELITE_PDFIUM_PATH}")
set(PDFIUM_ROOT ${PDFELITE_PDFIUM_PATH})

# We define pdfium as an imported static target
add_library(pdfium STATIC IMPORTED)

# Setup includes and libs
set_target_properties(pdfium PROPERTIES
    IMPORTED_LOCATION "${PDFIUM_ROOT}/lib/pdfium.dll.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${PDFIUM_ROOT}/include"
)

# For runtime we will need to copy the pdfium.dll next to our executable
# The caller will do this via a post-build step or we can add it here
function(copy_pdfium_dll target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PDFIUM_ROOT}/bin/pdfium.dll"
            $<TARGET_FILE_DIR:${target}>
    )
endfunction()
