// TileCache.cpp - LRU tile cache for smooth scrolling
#include "TileCache.h"

namespace PdfElite {

void TileCache::Put(const TileKey& key, ID2D1Bitmap* bitmap) {
    if (m_cache.size() >= MAX_TILES) {
        EvictOld();
    }
    m_cache[key] = bitmap;
}

ID2D1Bitmap* TileCache::Get(const TileKey& key) {
    auto it = m_cache.find(key);
    if (it != m_cache.end()) return it->second.Get();
    return nullptr;
}

void TileCache::Clear() {
    m_cache.clear();
}

void TileCache::EvictOld() {
    // Simple eviction - remove first
    if (!m_cache.empty()) {
        m_cache.erase(m_cache.begin());
    }
}

} // namespace PdfElite
