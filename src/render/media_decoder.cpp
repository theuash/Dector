#include "media_decoder.h"
#include <QDebug>
#include <QFile>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

MediaDecoder::MediaDecoder(QObject* parent) : QObject(parent) {}
MediaDecoder::~MediaDecoder() { close(); }

bool MediaDecoder::open(const QString& path) {
    close();

    if (avformat_open_input(&m_formatCtx, path.toUtf8().constData(), nullptr, nullptr) != 0)
        return false;
    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) { close(); return false; }

    m_videoStream = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_videoStream < 0) { close(); return false; }

    const AVCodec* codec = avcodec_find_decoder(
        m_formatCtx->streams[m_videoStream]->codecpar->codec_id);
    if (!codec) { close(); return false; }

    m_codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codecCtx, m_formatCtx->streams[m_videoStream]->codecpar);

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) { close(); return false; }

    m_width = m_codecCtx->width;
    m_height = m_codecCtx->height;
    m_duration = static_cast<double>(m_formatCtx->duration) / AV_TIME_BASE;

    AVRational r = m_formatCtx->streams[m_videoStream]->avg_frame_rate;
    m_fps = (r.num && r.den) ? static_cast<double>(r.num) / r.den : 30.0;

    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
    m_open = true;
    return true;
}

void MediaDecoder::close() {
    av_packet_free(&m_packet);
    av_frame_free(&m_frame);
    avcodec_free_context(&m_codecCtx);
    if (m_formatCtx) avformat_close_input(&m_formatCtx);
    sws_freeContext(m_swsCtx);
    m_swsCtx = nullptr;
    m_formatCtx = nullptr;
    m_codecCtx = nullptr;
    m_frame = nullptr;
    m_packet = nullptr;
    m_open = false;
    m_videoStream = -1;
    m_width = 0;
    m_height = 0;
    m_duration = 0;
    m_fps = 30.0;
}

bool MediaDecoder::seek(double seconds) {
    if (!m_open) return false;
    int64_t ts = static_cast<int64_t>(seconds /
        av_q2d(m_formatCtx->streams[m_videoStream]->time_base));
    avcodec_flush_buffers(m_codecCtx);
    return av_seek_frame(m_formatCtx, m_videoStream, ts, AVSEEK_FLAG_BACKWARD) >= 0;
}

std::shared_ptr<QImage> MediaDecoder::readFrame() {
    if (!m_open) return nullptr;

    while (av_read_frame(m_formatCtx, m_packet) >= 0) {
        if (m_packet->stream_index != m_videoStream) {
            av_packet_unref(m_packet);
            continue;
        }

        if (avcodec_send_packet(m_codecCtx, m_packet) != 0) {
            av_packet_unref(m_packet);
            continue;
        }

        if (avcodec_receive_frame(m_codecCtx, m_frame) != 0) {
            av_packet_unref(m_packet);
            continue;
        }

        m_swsCtx = sws_getCachedContext(m_swsCtx,
            m_frame->width, m_frame->height, m_codecCtx->pix_fmt,
            m_frame->width, m_frame->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

        auto image = std::make_shared<QImage>(m_frame->width, m_frame->height, QImage::Format_RGBA8888);
        uint8_t* dst[] = { image->bits() };
        int dstStride[] = { static_cast<int>(image->bytesPerLine()) };

        sws_scale(m_swsCtx, m_frame->data, m_frame->linesize,
                  0, m_frame->height, dst, dstStride);

        av_packet_unref(m_packet);
        return image;
    }

    return nullptr;
}
