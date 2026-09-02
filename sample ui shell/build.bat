@echo off
setlocal
cd /d "%~dp0"

echo Building PDFEliteUIPreview.exe...

cl /nologo /EHsc /MD /O2 /W3 /DNDEBUG ^
   PreviewMain.cpp MainWindow.cpp AppShell.cpp DocumentView.cpp ^
   PdfCanvas.cpp PdfDocument.cpp Theme.cpp ThemeManager.cpp ^
   NativeDesignSystem.cpp IconSystem.cpp LayoutManager.cpp GraphicsDevice.cpp TileCache.cpp RenderWorker.cpp ^
   /link /out:PDFEliteUIPreview.exe d2d1.lib dwrite.lib user32.lib windowscodecs.lib ole32.lib shcore.lib

if %errorlevel% neq 0 (
    echo Build failed.
    exit /b %errorlevel%
)

echo Build succeeded!
exit /b 0
