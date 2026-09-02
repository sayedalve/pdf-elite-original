add_executable(PdfiumSpike PdfiumSpike.cpp)
add_executable(NewlineSpike NewlineSpike.cpp)
target_link_libraries(NewlineSpike PRIVATE pdf_elite_core pdfium)

add_executable(RegressionSuite
    RegressionSuite.cpp
    TextEditingValidation.cpp
    MultilineValidation.cpp
    Batch2Tests.cpp
    UiInteractionTests.cpp
)

target_link_libraries(PdfiumSpike PRIVATE pdf_elite_engine pdfium)
target_link_libraries(RegressionSuite PRIVATE pdf_elite_engine pdf_elite_core pdf_elite_ui pdfium windowscodecs ole32 d2d1 dwrite comctl32 shlwapi)

copy_pdfium_dll(PdfiumSpike)
copy_pdfium_dll(RegressionSuite)

add_executable(UiInteractionSuite
    UiInteractionTests.cpp
    UiInteractionRunner.cpp
)
target_link_libraries(UiInteractionSuite PRIVATE pdf_elite_engine pdf_elite_core pdf_elite_ui pdfium windowscodecs ole32 d2d1 dwrite comctl32 shlwapi)
copy_pdfium_dll(UiInteractionSuite)

add_executable(Batch2Tests
    Batch2Tests.cpp
    Batch2Runner.cpp
)
target_link_libraries(Batch2Tests PRIVATE pdf_elite_engine pdf_elite_core pdfium windowscodecs ole32)
copy_pdfium_dll(Batch2Tests)

add_executable(UnicodeFontPrototype UnicodeFontPrototype.cpp)
target_link_libraries(UnicodeFontPrototype PRIVATE pdf_elite_engine pdfium)
copy_pdfium_dll(UnicodeFontPrototype)

add_executable(M1StressTests
    Milestone1StressTests.cpp
    Milestone1StressRunner.cpp
)
target_link_libraries(M1StressTests PRIVATE pdf_elite_engine pdf_elite_core pdf_elite_ui pdfium windowscodecs ole32 d2d1 dwrite comctl32 shlwapi)
copy_pdfium_dll(M1StressTests)

add_executable(Milestone1Tests
    Milestone1Tests.cpp
)
target_link_libraries(Milestone1Tests PRIVATE pdf_elite_engine pdf_elite_core pdf_elite_ui pdfium windowscodecs ole32 d2d1 dwrite comctl32 shlwapi)
copy_pdfium_dll(Milestone1Tests)

add_executable(Milestone1Challenger2Tests
    Milestone1Challenger2Tests.cpp
)
target_link_libraries(Milestone1Challenger2Tests PRIVATE pdf_elite_engine pdf_elite_core pdf_elite_ui pdfium windowscodecs ole32 d2d1 dwrite comctl32 shlwapi)
copy_pdfium_dll(Milestone1Challenger2Tests)

add_executable(Milestone1Challenger1Tests
    Milestone1Challenger1Tests.cpp
)
target_link_libraries(Milestone1Challenger1Tests PRIVATE pdf_elite_engine pdf_elite_core pdf_elite_ui pdfium windowscodecs ole32 d2d1 dwrite comctl32 shlwapi)
copy_pdfium_dll(Milestone1Challenger1Tests)

add_executable(Milestone2Challenger2Tests
    Milestone2Challenger2Tests.cpp
)
target_link_libraries(Milestone2Challenger2Tests PRIVATE pdf_elite_engine pdf_elite_core pdf_elite_ui pdfium windowscodecs ole32 d2d1 dwrite comctl32 shlwapi)
copy_pdfium_dll(Milestone2Challenger2Tests)

add_executable(Milestone2Challenger1Tests
    Milestone2Challenger1Tests.cpp
)
target_link_libraries(Milestone2Challenger1Tests PRIVATE pdf_elite_engine pdf_elite_core pdf_elite_ui pdfium windowscodecs ole32 d2d1 dwrite comctl32 shlwapi)
copy_pdfium_dll(Milestone2Challenger1Tests)

add_executable(E2ETests
    E2ETests.cpp
)
target_link_libraries(E2ETests PRIVATE pdf_elite_engine pdf_elite_core pdf_elite_ui pdfium windowscodecs ole32 d2d1 dwrite comctl32 shlwapi)
copy_pdfium_dll(E2ETests)

add_executable(Phase1AdversarialTests
    Phase1AdversarialTests.cpp
)
target_link_libraries(Phase1AdversarialTests PRIVATE pdf_elite_engine pdf_elite_core pdf_elite_ui pdfium windowscodecs ole32 d2d1 dwrite comctl32 shlwapi)
copy_pdfium_dll(Phase1AdversarialTests)

add_executable(Milestone4Challenger1StressTests
    Milestone4Challenger1StressTests.cpp
)
target_link_libraries(Milestone4Challenger1StressTests PRIVATE pdf_elite_engine pdf_elite_core pdf_elite_ui pdfium windowscodecs ole32 d2d1 dwrite comctl32 shlwapi)
copy_pdfium_dll(Milestone4Challenger1StressTests)

add_executable(CoordinateConverterTests CoordinateConverterTests.cpp)
target_link_libraries(CoordinateConverterTests PRIVATE pdf_elite_core)
add_test(NAME CoordinateConverterTests COMMAND CoordinateConverterTests)

add_executable(CommandStackTests CommandStackTests.cpp)
target_link_libraries(CommandStackTests PRIVATE pdf_elite_core pdf_elite_engine pdf_elite_ui pdfium windowscodecs ole32 d2d1 dwrite comctl32 shlwapi)
add_test(NAME CommandStackTests COMMAND CommandStackTests)
copy_pdfium_dll(CommandStackTests)

add_executable(InteractionMathTests InteractionMathTests.cpp ../src/ui/src/interaction/StrokeStabilizer.cpp ../src/ui/src/interaction/ShapeRecognizer.cpp)
target_link_libraries(InteractionMathTests PRIVATE pdf_elite_core)
add_test(NAME InteractionMathTests COMMAND InteractionMathTests)
