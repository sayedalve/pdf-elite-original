#include "CrashHandler.h"
#include <windows.h>
#include <DbgHelp.h>
#include <shlobj.h>
#include <stdio.h>

#pragma comment(lib, "DbgHelp.lib")

namespace app {

LONG WINAPI TopLevelExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    // Use static/stack buffers to avoid heap allocation during crash
    WCHAR appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath))) {
        wcscat_s(appDataPath, MAX_PATH, L"\\PDFElite");
        CreateDirectoryW(appDataPath, NULL);
        wcscat_s(appDataPath, MAX_PATH, L"\\crashes");
        CreateDirectoryW(appDataPath, NULL);

        SYSTEMTIME st;
        GetLocalTime(&st);

#if defined(_M_X64)
        const WCHAR* arch = L"x64";
#elif defined(_M_IX86)
        const WCHAR* arch = L"x86";
#elif defined(_M_ARM64)
        const WCHAR* arch = L"ARM64";
#else
        const WCHAR* arch = L"Unknown";
#endif

#if defined(NDEBUG)
        const WCHAR* config = L"Release";
#else
        const WCHAR* config = L"Debug";
#endif

        WCHAR dumpPath[MAX_PATH];
        swprintf_s(dumpPath, MAX_PATH, L"%s\\PDFElite_Crash_%s_%s_%04d%02d%02d_%02d%02d%02d.dmp", 
            appDataPath, arch, config,
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        HANDLE hFile = CreateFileW(dumpPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION exInfo;
            exInfo.ThreadId = GetCurrentThreadId();
            exInfo.ExceptionPointers = pExceptionInfo;
            exInfo.ClientPointers = FALSE;

            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &exInfo, NULL, NULL);
            CloseHandle(hFile);
        }

        // Write readable crash log
        WCHAR logPath[MAX_PATH];
        swprintf_s(logPath, MAX_PATH, L"%s\\PDFElite_CrashLog_%04d%02d%02d_%02d%02d%02d.txt", 
            appDataPath, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        HANDLE hLog = CreateFileW(logPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hLog != INVALID_HANDLE_VALUE) {
            char msg[256];
            sprintf_s(msg, "CRASH! ExceptionCode: 0x%08X, ExceptionAddress: 0x%p\n",
                pExceptionInfo->ExceptionRecord->ExceptionCode,
                pExceptionInfo->ExceptionRecord->ExceptionAddress);
            DWORD written;
            WriteFile(hLog, msg, (DWORD)strlen(msg), &written, NULL);
            CloseHandle(hLog);
        }
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

void CrashHandler::Initialize() {
    SetUnhandledExceptionFilter(TopLevelExceptionHandler);
}

}
