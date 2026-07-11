#include "media_decoder.h"
#include <QDebug>
#include <QAudioFormat>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

MediaDecoder::MediaDecoder(QObject* parent) : QObject(parent) {}
MediaDecoder::~MediaDecoder() { close(); }

bool MediaDecoder::open(const QString& path) {
    close();

    if (avformat_open_input(&m_formatCtx, path.toUtf8().constData(), nullptr, nullptr) != 0)
        return false;
    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) { close(); return false; }

    // Video stream
    m_videoStream = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_videoStream >= 0) {
        const AVCodec* codec = avcodec_find_decoder(
            m_formatCtx->streams[m_videoStream]->codecpar->codec_id);
        if (codec) {
            m_videoCodecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(m_videoCodecCtx, m_formatCtx->streams[m_videoStream]->codecpar);
            if (avcodec_open2(m_videoCodecCtx, codec, nullptr) >= 0) {
                m_width = m_videoCodecCtx->width;
                m_height = m_videoCodecCtx->height;
                AVRational r = m_formatCtx->streams[m_videoStream]->avg_frame_rate;
                m_fps = (r.num && r.den) ? static_cast<double>(r.num) / r.den : 30.0;
            }
        }
    }

    // Audio stream
    m_audioStream = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (m_audioStream >= 0) {
        const AVCodec* codec = avcodec_find_decoder(
            m_formatCtx->streams[m_audioStream]->codecpar->codec_id);
        if (codec) {
            m_audioCodecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(m_audioCodecCtx, m_formatCtx->streams[m_audioStream]->codecpar);
            if (avcodec_open2(m_audioCodecCtx, codec, nullptr) >= 0) {
                // Setup resampler to float planar
                AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_STEREO;
                int ret = swr_alloc_set_opts2(&m_swrCtx,
                    &outLayout, AV_SAMPLE_FMT_FLT, m_audioCodecCtx->sample_rate,
                    &m_audioCodecCtx->ch_layout, m_audioCodecCtx->sample_fmt, m_audioCodecCtx->sample_rate,
                    0, nullptr);
                if (ret >= 0 && m_swrCtx) swr_init(m_swrCtx);

                m_audioFormat.setSampleRate(m_audioCodecCtx->sample_rate);
                m_audioFormat.setChannelCount(2);
                m_audioFormat.setSampleFormat(QAudioFormat::Float);
            }
        }
    }

    m_duration = static_cast<double>(m_formatCtx->duration) / AV_TIME_BASE;
    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
    m_open = true;
    return true;
}

void MediaDecoder::close() {
    av_packet_free(&m_packet);
    av_frame_free(&m_frame);
    avcodec_free_context(&m_videoCodecCtx);
    avcodec_free_context(&m_audioCodecCtx);
    if (m_formatCtx) avformat_close_input(&m_formatCtx);
    sws_freeContext(m_swsCtx);
    swr_free(&m_swrCtx);
    m_swsCtx = nullptr;
    m_swrCtx = nullptr;
    m_formatCtx = nullptr;
    m_videoStream = -1;
    m_audioStream = -1;
    m_open = false;
    m_duration = 0;
    m_fps = 30.0;
    m_width = 0;
    m_height = 0;
}

bool MediaDecoder::seek(double seconds) {
    if (!m_open) return false;
    int64_t ts = static_cast<int64_t>(seconds / av_q2d(m_formatCtx->streams[m_videoStream >= 0 ? m_videoStream : m_audioStream]->time_base));
    avcodec_flush_buffers(m_videoCodecCtx);
    if (m_audioCodecCtx) avcodec_flush_buffers(m_audioCodecCtx);
    return av_seek_frame(m_formatCtx, m_videoStream >= 0 ? m_videoStream : m_audioStream, ts, AVSEEK_FLAG_BACKWARD) >= 0;
}

std::shared_ptr<QImage> MediaDecoder::readFrame() {
    if (!m_open || m_videoStream < 0) return nullptr;

    while (av_read_frame(m_formatCtx, m_packet) >= 0) {
        if (m_packet->stream_index == m_videoStream) {
            if (avcodec_send_packet(m_videoCodecCtx, m_packet) == 0) {
                if (avcodec_receive_frame(m_videoCodecCtx, m_frame) == 0) {
                    m_swsCtx = sws_getCachedContext(m_swsCtx,
                        m_frame->width, m_frame->height, m_videoCodecCtx->pix_fmt,
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
            }
        }
        av_packet_unref(m_packet);
    }
    return nullptr;
}

std::vector<float> MediaDecoder::readAudioFrame(int maxSamples) {
    if (!m_open || m_audioStream < 0 || !m_audioCodecCtx || !m_swrCtx) return {};

    std::vector<float> output;
    AVFrame* frame = av_frame_alloc();

    while ((int)output.size() < maxSamples) {
        while (av_read_frame(m_formatCtx, m_packet) >= 0) {
            if (m_packet->stream_index != m_audioStream) {
                av_packet_unref(m_packet);
                continue;
            }
            if (avcodec_send_packet(m_audioCodecCtx, m_packet) == 0) {
                while (avcodec_receive_frame(m_audioCodecCtx, frame) == 0) {
                    // Resample
                    uint8_t* outData[2] = { nullptr, nullptr };
                    int outLinesize[2] = { 0, 0 };
                    int outSamples = swr_get_out_samples(m_swrCtx, frame->nb_samples);
                    av_samples_alloc(outData, outLinesize, 2, outSamples, AV_SAMPLE_FMT_FLT, 0);

                    int converted = swr_convert(m_swrCtx, outData, outSamples,
                        (const uint8_t**)frame->data, frame->nb_samples);
                    if (converted > 0) {
                        float* f = (float*)outData[0];
                        output.insert(output.end(), f, f + converted * 2);
                    }
                    av_freep(&outData[0]);
                    if ((int)output.size() >= maxSamples) break;
                }
            }
            av_packet_unref(m_packet);
            if ((int)output.size() >= maxSamples) break;
        }
        if (av_read_frame(m_formatCtx, m_packet) < 0) break;
    }
    av_frame_free(&frame);
    return output;
}