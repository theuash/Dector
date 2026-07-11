#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QAudioSink>
#include <QBuffer>
#include "rational_time.h"

class Project;
class MediaDecoder;

class PlaybackEngine : public QObject {
    Q_OBJECT
public:
    explicit PlaybackEngine(QObject* parent = nullptr);
    ~PlaybackEngine() override;

    void setProject(Project* project);
    void play();
    void pause();
    void stop();
    void seek(const RationalTime& time);

    bool isPlaying() const { return m_playing; }
    RationalTime currentTime() const { return m_currentTime; }

signals:
    void frameChanged(const RationalTime& time);
    void playbackStarted();
    void playbackPaused();
    void playbackStopped();

private slots:
    void onAudioStateChanged(QAudio::State state);

private:
    void timerEvent(QTimerEvent* event) override;
    void fillAudioBuffer();

    Project* m_project = nullptr;
    bool m_playing = false;
    RationalTime m_currentTime{0, 30};
    double m_fps = 30.0;
    int m_timerId = 0;
    QElapsedTimer m_clock;

    // Audio
    QAudioSink* m_audioSink = nullptr;
    QIODevice* m_audioOutput = nullptr;
    QBuffer m_audioBuffer;
    std::vector<float> m_audioMixBuffer;
    int m_audioSampleRate = 48000;
    int m_audioChannels = 2;
    int m_samplesPerFrame = 0;
    QAudioFormat m_audioFormat;
};