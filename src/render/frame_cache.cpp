#include "frame_cache.h"
#include "media_decoder.h"
#include <QThread>
#include <QDebug>

FrameCache::FrameCache(size_t maxFrames, QObject* parent) : QObject(parent), m_maxFrames(maxFrames) {}
FrameCache::~FrameCache() { clear(); }

void FrameCache::setDecoder(MediaDecoder* decoder) {
    m_decoder = decoder;
}

std::shared_ptr<QImage> FrameCache::getFrame(double timeSeconds) {
    QMutexLocker lock(&m_mutex);
    
    // Check cache
    auto it = m_cache.find(timeSeconds);
    if (it != m_cache.end()) {
        // Move to front of LRU
        m_lru.erase(it->lruIt);
        m_lru.push_front(timeSeconds);
        it->lruIt = m_lru.begin();
        return it->image;
    }
    
    // Decode frame
    if (!m_decoder || !m_decoder->isOpen()) return nullptr;
    
    if (!m_decoder->seek(timeSeconds)) return nullptr;
    auto frame = m_decoder->readFrame();
    if (!frame || frame->isNull()) return nullptr;
    
    // Add to cache
    if (m_cache.size() >= m_maxFrames) evictLRU();
    
    CacheEntry entry;
    entry.timeSeconds = timeSeconds;
    entry.image = frame;
    entry.lruIt = m_lru.insert(m_lru.begin(), timeSeconds);
    m_cache[timeSeconds] = entry;
    
    return frame;
}

void FrameCache::evictLRU() {
    if (m_lru.empty()) return;
    double oldest = m_lru.back();
    m_lru.pop_back();
    m_cache.remove(oldest);
}

void FrameCache::clear() {
    QMutexLocker lock(&m_mutex);
    m_cache.clear();
    m_lru.clear();
}

FramePreloader::FramePreloader(FrameCache* cache, QObject* parent) : QObject(parent), m_cache(cache) {}
FramePreloader::~FramePreloader() { stop(); }

void FramePreloader::setRange(double startSec, double endSec, double fps) {
    m_startSec = startSec;
    m_endSec = endSec;
    m_fps = fps;
    m_totalFrames = static_cast<int>((endSec - startSec) * fps);
    m_currentFrame = 0;
}

void FramePreloader::start() {
    if (m_running) return;
    m_running = true;
    m_currentFrame = 0;
    
    while (m_running && m_currentFrame < m_totalFrames) {
        double t = m_startSec + m_currentFrame / m_fps;
        m_cache->getFrame(t);
        
        ++m_currentFrame;
        if (m_currentFrame % 10 == 0) {
            emit progress(m_currentFrame, m_totalFrames);
            QThread::msleep(5); // Yield
        }
    }
    m_running = false;
    emit progress(m_totalFrames, m_totalFrames);
}

void FramePreloader::stop() {
    m_running = false;
}