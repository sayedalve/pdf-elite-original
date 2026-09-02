#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace core {

struct RecentFile {
    std::wstring path;
    std::wstring filename;
    uint64_t lastAccessed = 0;
    uint64_t fileSize = 0;
    bool isStarred = false;
};

struct RecentFolder {
    std::wstring path;
    std::wstring folderName;
    uint64_t lastAccessed = 0;
    int fileCount = 0;
};

class RecentFilesManager {
public:
    static RecentFilesManager& Instance();

    void AddFile(const std::wstring& path);
    void RemoveFile(const std::wstring& path);
    std::vector<RecentFile> GetRecentFiles();
    std::vector<RecentFile> GetStarredFiles();
    bool IsStarred(const std::wstring& path);
    void SetStarred(const std::wstring& path, bool starred);
    void ToggleStar(const std::wstring& path);
    void ToggleStarred(const std::wstring& path) { ToggleStar(path); }
    std::vector<RecentFolder> GetRecentFolders();

private:
    RecentFilesManager();
    ~RecentFilesManager() = default;
    RecentFilesManager(const RecentFilesManager&) = delete;
    RecentFilesManager& operator=(const RecentFilesManager&) = delete;

    void Load();
    void Save();
    std::wstring GetRecentFilePath() const;
    uint64_t GetCurrentTimeMs() const;
    uint64_t GetFileSize(const std::wstring& path) const;

    std::vector<RecentFile> m_recentFiles;
    std::mutex m_mutex;
    const size_t MAX_RECENT_FILES = 20;
};

} // namespace core
