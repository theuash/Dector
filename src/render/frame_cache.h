#pragma once
#include <QObject>
#include <QImage>
#include <QMutex>
#include <QHash>
#include <memory>
#include <list>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

class FrameCache : public QObject {
    Q_OBJECT
public:
    explicit FrameCache(size_t maxFrames = 60, QObject* parent = nullptr);
    ~FrameCache() override;

    void setDecoder(class MediaDecoder* decoder);
    std::shared_ptr<QImage> getFrame(double timeSeconds);
    void clear();

    size_t maxFrames() const { return m_maxFrames; }
    void setMaxFrames(size_t n) { m_maxFrames = n; }

signals:
    void frameReady(double timeSeconds, std::shared_ptr<QImage> frame);

private:
    struct CacheEntry {
        double timeSeconds;
        std::shared_ptr<QImage> image;
        std::list<CacheEntry>::iterator lruIt;
    };

    class MediaDecoder* m_decoder = nullptr;
    size_t m_maxFrames = 60;
    QHash<double, CacheEntry> m_cache;
    std::list<double> m_lru; // LRU list of time keys
    QMutex m_mutex;

    void evictLRU();
};

class FramePreloader : public QObject {
    Q_OBJECT
public:
    explicit FramePreloader(FrameCache* cache, QObject* parent = nullptr);
    ~FramePreloader() override;

    void setRange(double startSec, double endSec, double fps);
    void start();
    void stop();

signals:
    void progress(int current, int total);

private:
    FrameCache* m_cache = nullptr;
    bool m_running = false;
    double m_startSec = 0;
    double m_endSec = 0;
    double m_fps = 30;
    int m_currentFrame = 0;
    int m_totalFrames = 0;
};