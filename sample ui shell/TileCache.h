// TileCache.h - Page tile cache for performance
#pragma once
#include <unordered_map>
#include <d2d1.h>
#include <wrl/client.h>

namespace PdfElite {

struct TileKey {
    int page;
    int x, y;
    float zoom;
    bool operator==(const TileKey& other) const {
        return page == other.page && x == other.x && y == other.y && zoom == other.zoom;
    }
};

struct TileKeyHash {
    size_t operator()(const TileKey& k) const {
        return std::hash<int>()(k.page) ^ std::hash<int>()(k.x) ^ std::hash<float>()(k.zoom);
    }
};

class TileCache {
public:
    void Put(const TileKey& key, ID2D1Bitmap* bitmap);
    ID2D1Bitmap* Get(const TileKey& key);
    void Clear();
    void EvictOld();

private:
    std::unordered_map<TileKey, Microsoft::WRL::ComPtr<ID2D1Bitmap>, TileKeyHash> m_cache;
    static constexpr size_t MAX_TILES = 128;
};

} // namespace PdfElite
