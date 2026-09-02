#include <windows.h>
#include <DbgHelp.h>
#include <iostream>

#pragma comment(lib, "Dbghelp.lib")

int main() {
    HANDLE hFile = CreateFileA("C:\\Users\\sayed\\AppData\\Local\\PDFElite\\crashes\\PDFElite_Crash_x64_Release_20260818_162857.dmp", 
        GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open dump\n";
        return 1;
    }

    HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    void* pView = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);

    MINIDUMP_DIRECTORY* dir;
    void* streamPointer;
    ULONG streamSize;
    if (MiniDumpReadDumpStream(pView, ExceptionStream, &dir, &streamPointer, &streamSize)) {
        MINIDUMP_EXCEPTION_STREAM* exStream = (MINIDUMP_EXCEPTION_STREAM*)streamPointer;
        printf("ExceptionCode: 0x%08X\n", exStream->ExceptionRecord.ExceptionCode);
        printf("ExceptionFlags: 0x%08X\n", exStream->ExceptionRecord.ExceptionFlags);
        printf("ExceptionAddress: 0x%p\n", (void*)exStream->ExceptionRecord.ExceptionAddress);
    } else {
        std::cerr << "No exception stream\n";
    }

    UnmapViewOfFile(pView);
    CloseHandle(hMap);
    CloseHandle(hFile);
    return 0;
}
