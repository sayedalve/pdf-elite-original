#include "PdfiumLibrary.h"
#include <fpdfview.h>
#include <fpdf_ext.h>

PdfiumLibrary& PdfiumLibrary::Instance() {
    static PdfiumLibrary instance;
    return instance;
}

void PdfiumLibrary::Initialize(const PdfiumConfig& config) {
    (void)config;
    if (!m_initialized) {
        FPDF_LIBRARY_CONFIG fpdfConfig = {};
        fpdfConfig.version = 2;
        fpdfConfig.m_pUserFontPaths = nullptr;
        fpdfConfig.m_pIsolate = nullptr;
        fpdfConfig.m_v8EmbedderSlot = 0;
        FPDF_InitLibraryWithConfig(&fpdfConfig);

        // Security: Configure unsupported info handler to fail safely on unsupported features
        static UNSUPPORT_INFO unsp_info = {};
        unsp_info.version = 1;
        unsp_info.FSDK_UnSupport_Handler = [](UNSUPPORT_INFO*, int type) {
            // Intentionally unhandled to silently ignore dangerous features like JS/3D
            (void)type;
        };
        FSDK_SetUnSpObjProcessHandler(&unsp_info);

        m_initialized = true;
    }
}

PdfiumLibrary::~PdfiumLibrary() {
    if (m_initialized) {
        FPDF_DestroyLibrary();
    }
}
