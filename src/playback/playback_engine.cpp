#include "playback_engine.h"
#include "project.h"
#include <QTimerEvent>
#include <QAudioFormat>
#include <QAudioSink>
#include <QBuffer>
#include <QDebug>
#include <algorithm>

PlaybackEngine::PlaybackEngine(QObject* parent) : QObject(parent) {
    m_audioFormat.setSampleRate(48000);
    m_audioFormat.setChannelCount(2);
    m_audioFormat.setSampleFormat(QAudioFormat::Float);
    m_audioFormat.setChannelConfig(QAudioFormat::ChannelConfigStereo);

    m_audioSink = new QAudioSink(m_audioFormat, this);
    m_audioSink->setBufferSize(65536);
    m_audioOutput = m_audioSink->start();
    m_audioBuffer.open(QBuffer::ReadWrite);
    connect(m_audioSink, &QAudioSink::stateChanged, this, &PlaybackEngine::onAudioStateChanged);

    m_audioSampleRate = m_audioFormat.sampleRate();
    m_audioChannels = m_audioFormat.channelCount();
    m_samplesPerFrame = m_audioSampleRate / 30; // ~1600 at 48kHz/30fps
    m_audioMixBuffer.resize(m_samplesPerFrame * m_audioChannels);
}

PlaybackEngine::~PlaybackEngine() {
    stop();
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
    }
}

void PlaybackEngine::setProject(Project* project) {
    m_project = project;
    stop();
}

void PlaybackEngine::play() {
    if (m_playing || !m_project) return;
    m_playing = true;
    m_clock.start();
    m_timerId = startTimer(1000 / static_cast<int>(m_fps));
    emit playbackStarted();
}

void PlaybackEngine::pause() {
    if (!m_playing) return;
    m_playing = false;
    if (m_timerId) killTimer(m_timerId);
    m_timerId = 0;
    if (m_audioSink) m_audioSink->suspend();
    emit playbackPaused();
}

void PlaybackEngine::stop() {
    pause();
    m_currentTime = RationalTime(0, static_cast<int64_t>(m_fps));
    if (m_audioSink) m_audioSink->stop();
    m_audioBuffer.buffer().clear();
    emit playbackStopped();
}

void PlaybackEngine::seek(const RationalTime& time) {
    m_currentTime = time;
    emit frameChanged(m_currentTime);
}

void PlaybackEngine::timerEvent(QTimerEvent* event) {
    if (event->timerId() != m_timerId) return;
    if (!m_playing) return;

    RationalTime frameDuration(1, static_cast<int64_t>(m_fps));
    m_currentTime = m_currentTime + frameDuration;

    if (m_project && m_project->currentSequence()) {
        RationalTime seqEnd = m_project->currentSequence()->calculateDuration();
        if (m_currentTime >= seqEnd && seqEnd > RationalTime(0, 1)) {
            stop();
            return;
        }
    }

    emit frameChanged(m_currentTime);
    fillAudioBuffer();
}

void PlaybackEngine::fillAudioBuffer() {
    if (!m_audioOutput || !m_project || !m_project->currentSequence()) return;

    auto* seq = m_project->currentSequence();
    RationalTime t = m_currentTime;
    RationalTime frameDur(1, static_cast<int64_t>(m_fps));

    std::fill(m_audioMixBuffer.begin(), m_audioMixBuffer.end(), 0.0f);

    for (const auto& track : seq->tracks) {
        if (!track.enabled || track.type != TrackType::Audio) continue;

        for (const auto& clip : track.clips) {
            if (t < clip.trackOffset || t >= clip.endTime()) continue;

            auto* asset = m_project->asset(clip.assetId);
            if (!asset) continue;

            // Simple audio mixing - in production you'd decode actual audio
            // For now, generate a tone based on clip position
            double phase = (t - clip.trackOffset).toSeconds() * 440.0 * 2 * M_PI;
            for (int i = 0; i < m_samplesPerFrame; ++i) {
                float sample = 0.1f * sin(phase);
                phase += 440.0 * 2 * M_PI / m_audioSampleRate;
                for (int c = 0; c < m_audioChannels; ++c) {
                    m_audioMixBuffer[i * m_audioChannels + c] += sample;
                }
            }
        }
    }

    // Write to audio buffer
    QByteArray data(reinterpret_cast<char*>(m_audioMixBuffer.data()), m_audioMixBuffer.size() * sizeof(float));
    m_audioBuffer.write(data);
    int bytesFree = m_audioSink->bytesFree();
    if (bytesFree > 0) {
        QByteArray chunk = m_audioBuffer.read(qMin(bytesFree, m_audioBuffer.size()));
        if (!chunk.isEmpty()) m_audioOutput->write(chunk);
    }
}

void PlaybackEngine::onAudioStateChanged(QAudio::State state) {
    if (state == QAudio::IdleState && m_playing) {
        fillAudioBuffer();
    } else if (state == QAudio::StoppedState) {
        m_audioBuffer.buffer().clear();
    }
}