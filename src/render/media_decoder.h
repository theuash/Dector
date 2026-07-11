#pragma once
#include <QObject>
#include <QImage>
#include <QAudioFormat>
#include <memory>
#include <vector>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct SwrContext;

class MediaDecoder : public QObject {
    Q_OBJECT
public:
    explicit MediaDecoder(QObject* parent = nullptr);
    ~MediaDecoder() override;

    bool open(const QString& path);
    void close();
    bool seek(double seconds);
    std::shared_ptr<QImage> readFrame();

    // Audio
    bool hasAudio() const { return m_audioStream >= 0; }
    const QAudioFormat& audioFormat() const { return m_audioFormat; }
    std::vector<float> readAudioFrame(int maxSamples = 4096);

    double duration() const { return m_duration; }
    double fps() const { return m_fps; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool isOpen() const { return m_open; }

private:
    AVFormatContext* m_formatCtx = nullptr;
    AVCodecContext* m_videoCodecCtx = nullptr;
    AVCodecContext* m_audioCodecCtx = nullptr;
    SwsContext* m_swsCtx = nullptr;
    SwrContext* m_swrCtx = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;
    int m_videoStream = -1;
    int m_audioStream = -1;

    bool m_open = false;
    double m_duration = 0;
    double m_fps = 30.0;
    int m_width = 0;
    int m_height = 0;

    QAudioFormat m_audioFormat;
};