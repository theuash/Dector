#pragma once
#include <QObject>
#include <QImage>
#include <memory>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

class MediaDecoder : public QObject {
    Q_OBJECT
public:
    explicit MediaDecoder(QObject* parent = nullptr);
    ~MediaDecoder() override;

    bool open(const QString& path);
    void close();
    bool seek(double seconds);
    std::shared_ptr<QImage> readFrame();

    double duration() const { return m_duration; }
    double fps() const { return m_fps; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool isOpen() const { return m_open; }

private:
    AVFormatContext* m_formatCtx = nullptr;
    AVCodecContext* m_codecCtx = nullptr;
    SwsContext* m_swsCtx = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;
    int m_videoStream = -1;

    bool m_open = false;
    double m_duration = 0;
    double m_fps = 30.0;
    int m_width = 0;
    int m_height = 0;
};
