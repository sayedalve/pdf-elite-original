add_executable(PDFElite WIN32
    main.cpp
    Application.cpp
    CrashHandler.cpp
    Preferences.cpp
    PrintManager.cpp
    PDFElite.rc
)

target_link_libraries(PDFElite PRIVATE
    pdf_elite_core
    pdf_elite_engine
    pdf_elite_ui
    comctl32
)

copy_pdfium_dll(PDFElite)

# Install rules
install(TARGETS PDFElite RUNTIME DESTINATION bin)
install(FILES "${PDFIUM_ROOT}/bin/pdfium.dll" DESTINATION bin)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources" DESTINATION .)
