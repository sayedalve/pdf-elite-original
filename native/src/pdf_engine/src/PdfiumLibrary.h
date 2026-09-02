#pragma once

struct PdfiumConfig {
    // future config
};

class PdfiumLibrary {
public:
    static PdfiumLibrary& Instance();
    void Initialize(const PdfiumConfig& config = {});
    ~PdfiumLibrary();

    PdfiumLibrary(const PdfiumLibrary&) = delete;
    PdfiumLibrary& operator=(const PdfiumLibrary&) = delete;

private:
    PdfiumLibrary() = default;
    bool m_initialized = false;
};
