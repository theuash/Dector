#include "playback_engine.h"
#include "project.h"
#include <QTimerEvent>

PlaybackEngine::PlaybackEngine(QObject* parent) : QObject(parent) {}

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
    emit playbackPaused();
}

void PlaybackEngine::stop() {
    pause();
    m_currentTime = RationalTime(0, static_cast<int64_t>(m_fps));
    emit frameChanged(m_currentTime);
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
}
