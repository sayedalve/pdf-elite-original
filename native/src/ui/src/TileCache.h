#pragma once
#include <unordered_map>
#include <list>
#include <memory>
#include <windows.h>
#include <d2d1_1.h>
#include <wrl/client.h>

struct TileKey {
    uint64_t documentGeneration;
    int pageIndex;
    float zoom;
    float dpiScale;
    int tileX;
    int tileY;
    int tileWidth;
    int tileHeight;

    bool operator==(const TileKey& o) const {
        return documentGeneration == o.documentGeneration && 
               pageIndex == o.pageIndex && 
               zoom == o.zoom &&
               dpiScale == o.dpiScale &&
               tileX == o.tileX && 
               tileY == o.tileY && 
               tileWidth == o.tileWidth && 
               tileHeight == o.tileHeight;
    }
};

struct TileKeyHash {
    size_t operator()(const TileKey& k) const {
        size_t h = std::hash<uint64_t>{}(k.documentGeneration);
        h ^= std::hash<int>{}(k.pageIndex) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<float>{}(k.zoom) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<float>{}(k.dpiScale) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(k.tileX) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(k.tileY) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(k.tileWidth) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(k.tileHeight) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

class TileCache {
public:
    TileCache(size_t maxBytes = 512 * 1024 * 1024);

    Microsoft::WRL::ComPtr<ID2D1Bitmap> Get(const TileKey& key);
    void Put(const TileKey& key, Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap, size_t byteSize);
    void InvalidateAll();
    void InvalidatePage(int pageIndex);

private:
    void EvictOne();

    struct CacheEntry {
        Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
        size_t byteSize;
        std::list<TileKey>::iterator lruIterator;
    };

    std::unordered_map<TileKey, CacheEntry, TileKeyHash> m_cache;
    std::list<TileKey> m_lru;
    size_t m_maxBytes;
    size_t m_currentBytes = 0;
};
