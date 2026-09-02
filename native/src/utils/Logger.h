#pragma once
#include <string>
#include <fstream>
#include <shlobj.h>
#include <mutex>

namespace utils {
    class Logger {
    public:
        static void Log(const std::string& message) {
            static std::mutex mtx;
            std::lock_guard<std::mutex> lock(mtx);
            
            char path[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
                std::string logPath = std::string(path) + "\\PDFElite\\logs\\open_pipeline.log";
                std::ofstream file(logPath, std::ios_base::app);
                file << message << "\n";
            }
        }
    };
}
