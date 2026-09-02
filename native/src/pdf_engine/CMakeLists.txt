add_library(pdf_elite_engine
    src/PdfiumLibrary.cpp
    src/PdfDocument.cpp
    src/PdfPage.cpp
    src/PdfAnnotation.cpp
    src/LineAnnotationAdapter.cpp
    src/PdfImage.cpp
    src/PdfTextObject.cpp
    src/PdfTextPage.cpp
    src/TextSelection.cpp
    src/SearchEngine.cpp
    src/RenderWorker.cpp
    src/EngineAdapter.cpp
    src/EngineEditingStubs.cpp
    src/PdfFormField.cpp
    src/FontManager.cpp
    src/commands/PageCommands.cpp
    src/commands/AnnotationCommands.cpp
    src/commands/ImageCommands.cpp
    src/commands/TextCommands.cpp
    src/commands/AddLinkCommand.cpp
    src/commands/AddBackgroundCommand.cpp
    src/commands/AddWatermarkCommand.cpp
    src/commands/AddHeaderFooterCommand.cpp
    src/operations/CreateBlankPdf.cpp
    src/operations/ExtractImagesFromPdf.cpp
    src/operations/CombinePdfs.cpp
)
target_include_directories(pdf_elite_engine PUBLIC
    include
    src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(pdf_elite_engine PUBLIC
    pdf_elite_core
    pdfium
    windowscodecs
    ole32
)



