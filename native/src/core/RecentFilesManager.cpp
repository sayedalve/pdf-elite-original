#include "RecentFilesManager.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>

namespace core {

RecentFilesManager& RecentFilesManager::Instance() {
    static RecentFilesManager instance;
    return instance;
}

RecentFilesManager::RecentFilesManager() {
    Load();
}

std::wstring RecentFilesManager::GetRecentFilePath() const {
    WCHAR path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        std::wstring dir = std::wstring(path) + L"\\PDFElite";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir + L"\\recent.ini";
    }
    return L"recent.ini";
}

uint64_t RecentFilesManager::GetCurrentTimeMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

uint64_t RecentFilesManager::GetFileSize(const std::wstring& path) const {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad))
        return 0;
    LARGE_INTEGER size;
    size.HighPart = fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;
    return size.QuadPart;
}

void RecentFilesManager::AddFile(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(m_mutex);

    bool previouslyStarred = false;
    auto it = std::find_if(m_recentFiles.begin(), m_recentFiles.end(), [&](const RecentFile& rf) {
        return rf.path == path;
    });
    if (it != m_recentFiles.end()) {
        previouslyStarred = it->isStarred;
        m_recentFiles.erase(it);
    }

    RecentFile rf;
    rf.path = path;
    
    size_t slashPos = path.find_last_of(L"\\/");
    if (slashPos != std::wstring::npos) {
        rf.filename = path.substr(slashPos + 1);
    } else {
        rf.filename = path;
    }
    
    rf.lastAccessed = GetCurrentTimeMs();
    rf.fileSize = GetFileSize(path);
    rf.isStarred = previouslyStarred;

    // Insert at front
    m_recentFiles.insert(m_recentFiles.begin(), rf);

    if (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles.resize(MAX_RECENT_FILES);
    }

    Save();
}

void RecentFilesManager::RemoveFile(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::remove_if(m_recentFiles.begin(), m_recentFiles.end(), [&](const RecentFile& rf) {
        return rf.path == path;
    });
    if (it != m_recentFiles.end()) {
        m_recentFiles.erase(it, m_recentFiles.end());
        Save();
    }
}

std::vector<RecentFile> RecentFilesManager::GetRecentFiles() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_recentFiles;
}

std::vector<RecentFile> RecentFilesManager::GetStarredFiles() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<RecentFile> starred;
    for (const auto& rf : m_recentFiles) {
        if (rf.isStarred) {
            starred.push_back(rf);
        }
    }
    return starred;
}

bool RecentFilesManager::IsStarred(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& rf : m_recentFiles) {
        if (rf.path == path) {
            return rf.isStarred;
        }
    }
    return false;
}

void RecentFilesManager::SetStarred(const std::wstring& path, bool starred) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& rf : m_recentFiles) {
        if (rf.path == path) {
            rf.isStarred = starred;
            Save();
            return;
        }
    }
    if (starred) {
        RecentFile rf;
        rf.path = path;
        size_t slashPos = path.find_last_of(L"\\/");
        if (slashPos != std::wstring::npos) {
            rf.filename = path.substr(slashPos + 1);
        } else {
            rf.filename = path;
        }
        rf.lastAccessed = GetCurrentTimeMs();
        rf.fileSize = GetFileSize(path);
        rf.isStarred = true;
        m_recentFiles.insert(m_recentFiles.begin(), rf);
        if (m_recentFiles.size() > MAX_RECENT_FILES) {
            m_recentFiles.resize(MAX_RECENT_FILES);
        }
        Save();
    }
}

void RecentFilesManager::ToggleStar(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& rf : m_recentFiles) {
        if (rf.path == path) {
            rf.isStarred = !rf.isStarred;
            Save();
            return;
        }
    }
    RecentFile rf;
    rf.path = path;
    size_t slashPos = path.find_last_of(L"\\/");
    if (slashPos != std::wstring::npos) {
        rf.filename = path.substr(slashPos + 1);
    } else {
        rf.filename = path;
    }
    rf.lastAccessed = GetCurrentTimeMs();
    rf.fileSize = GetFileSize(path);
    rf.isStarred = true;
    m_recentFiles.insert(m_recentFiles.begin(), rf);
    if (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles.resize(MAX_RECENT_FILES);
    }
    Save();
}

std::vector<RecentFolder> RecentFilesManager::GetRecentFolders() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    struct FolderInfo {
        std::wstring folderName;
        uint64_t lastAccessed = 0;
        int fileCount = 0;
    };
    
    std::vector<std::pair<std::wstring, FolderInfo>> folderList;

    for (const auto& rf : m_recentFiles) {
        size_t slashPos = rf.path.find_last_of(L"\\/");
        if (slashPos != std::wstring::npos) {
            std::wstring folderPath = rf.path.substr(0, slashPos);
            if (folderPath.empty()) continue;

            auto it = std::find_if(folderList.begin(), folderList.end(), [&](const auto& pair) {
                return pair.first == folderPath;
            });

            if (it == folderList.end()) {
                size_t parentSlash = folderPath.find_last_of(L"\\/");
                std::wstring name = (parentSlash != std::wstring::npos)
                    ? folderPath.substr(parentSlash + 1)
                    : folderPath;
                if (name.empty()) name = folderPath;
                folderList.push_back({ folderPath, { name, rf.lastAccessed, 1 } });
            } else {
                it->second.fileCount++;
                if (rf.lastAccessed > it->second.lastAccessed) {
                    it->second.lastAccessed = rf.lastAccessed;
                }
            }
        }
    }

    std::vector<RecentFolder> result;
    result.reserve(folderList.size());
    for (const auto& item : folderList) {
        RecentFolder folder;
        folder.path = item.first;
        folder.folderName = item.second.folderName;
        folder.lastAccessed = item.second.lastAccessed;
        folder.fileCount = item.second.fileCount;
        result.push_back(folder);
    }
    return result;
}

void RecentFilesManager::Load() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::wifstream file(GetRecentFilePath());
    if (!file.is_open()) return;

    m_recentFiles.clear();
    std::wstring line;
    RecentFile current;
    
    while (std::getline(file, line)) {
        if (line.empty()) {
            if (!current.path.empty()) {
                m_recentFiles.push_back(current);
                current = RecentFile();
            }
            continue;
        }
        size_t eqPos = line.find(L'=');
        if (eqPos != std::wstring::npos) {
            std::wstring key = line.substr(0, eqPos);
            std::wstring val = line.substr(eqPos + 1);
            
            if (key == L"path") current.path = val;
            else if (key == L"filename") current.filename = val;
            else if (key == L"lastAccessed") {
                try { current.lastAccessed = std::stoull(val); } catch (...) {}
            }
            else if (key == L"fileSize") {
                try { current.fileSize = std::stoull(val); } catch (...) {}
            }
            else if (key == L"starred" || key == L"isStarred") {
                current.isStarred = (val == L"1" || val == L"true");
            }
        }
    }
    if (!current.path.empty()) {
        m_recentFiles.push_back(current);
    }
}

void RecentFilesManager::Save() {
    std::wofstream file(GetRecentFilePath());
    if (!file.is_open()) return;

    for (const auto& rf : m_recentFiles) {
        file << L"path=" << rf.path << L"\n";
        file << L"filename=" << rf.filename << L"\n";
        file << L"lastAccessed=" << rf.lastAccessed << L"\n";
        file << L"fileSize=" << rf.fileSize << L"\n";
        file << L"starred=" << (rf.isStarred ? 1 : 0) << L"\n\n";
    }
}

} // namespace core
