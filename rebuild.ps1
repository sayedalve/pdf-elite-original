Stop-Process -Name "PDFElite" -Force -ErrorAction SilentlyContinue
cmd.exe /c "call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" && cd C:\Users\sayed\Downloads\PDF-Elite\native\build && cmake --build . --config Release"
