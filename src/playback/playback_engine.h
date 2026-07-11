#pragma once
#include <QObject>
#include <QElapsedTimer>
#include "rational_time.h"

class Project;

class PlaybackEngine : public QObject {
    Q_OBJECT
public:
    explicit PlaybackEngine(QObject* parent = nullptr);

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

protected:
    void timerEvent(QTimerEvent* event) override;

private:
    Project* m_project = nullptr;
    bool m_playing = false;
    RationalTime m_currentTime{0, 30};
    double m_fps = 30.0;
    int m_timerId = 0;
    QElapsedTimer m_clock;
};
