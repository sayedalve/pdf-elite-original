#include "TileCache.h"

TileCache::TileCache(size_t maxBytes) : m_maxBytes(maxBytes) {}

Microsoft::WRL::ComPtr<ID2D1Bitmap> TileCache::Get(const TileKey& key) {
    auto it = m_cache.find(key);
    if (it == m_cache.end()) return nullptr;

    // Move to front (MRU)
    m_lru.erase(it->second.lruIterator);
    m_lru.push_front(key);
    it->second.lruIterator = m_lru.begin();

    return it->second.bitmap;
}

void TileCache::Put(const TileKey& key, Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap, size_t byteSize) {
    if (m_cache.find(key) != m_cache.end()) return; // Already exists

    while (m_currentBytes + byteSize > m_maxBytes && !m_lru.empty()) {
        EvictOne();
    }

    m_lru.push_front(key);
    m_cache[key] = { bitmap, byteSize, m_lru.begin() };
    m_currentBytes += byteSize;
}

void TileCache::InvalidateAll() {
    m_cache.clear();
    m_lru.clear();
    m_currentBytes = 0;
}

void TileCache::InvalidatePage(int pageIndex) {
    auto it = m_cache.begin();
    while (it != m_cache.end()) {
        if (it->first.pageIndex == pageIndex) {
            m_currentBytes -= it->second.byteSize;
            m_lru.erase(it->second.lruIterator);
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}

void TileCache::EvictOne() {
    if (m_lru.empty()) return;
    auto key = m_lru.back();
    m_lru.pop_back();

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_currentBytes -= it->second.byteSize;
        m_cache.erase(it);
    }
}
