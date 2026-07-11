#include "timeline_widget.h"
#include "project.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <algorithm>
#include <cmath>

TimelineWidget::TimelineWidget(Project* project, QWidget* parent)
    : QWidget(parent), m_project(project) {
    setMinimumHeight(120);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void TimelineWidget::setProject(Project* project) {
    m_project = project;
    m_selTrackId.clear();
    m_selClipId.clear();
    update();
}

void TimelineWidget::setPixelsPerSecond(int pps) {
    m_pixelsPerSecond = std::clamp(pps, 10, 800);
    update();
    emit zoomChanged(m_pixelsPerSecond);
}

void TimelineWidget::setPlayheadTime(const RationalTime& time) {
    m_playheadTime = time;
    update();
}

int TimelineWidget::trackY(int index) const {
    return 4 + index * (trackHeight() + 4);
}

int TimelineWidget::timeToX(const RationalTime& t) const {
    return labelWidth() + static_cast<int>(t.toSeconds() * m_pixelsPerSecond);
}

RationalTime TimelineWidget::xToTime(int x) const {
    int rx = std::max(0, x - labelWidth());
    return RationalTime::fromSeconds(static_cast<double>(rx) / m_pixelsPerSecond, 30.0);
}

void TimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(30, 30, 30));

    auto* seq = m_project ? m_project->currentSequence() : nullptr;
    if (!seq) {
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter, "No sequence");
        return;
    }

    // Draw tracks
    int nTracks = static_cast<int>(seq->tracks.size());
    for (int i = 0; i < nTracks; ++i) {
        const auto& track = seq->tracks[i];
        int y = trackY(i);
        int h = trackHeight();

        // Track background
        QColor tbg = (i % 2 == 0) ? QColor(45, 45, 45) : QColor(38, 38, 38);
        if (!track.enabled) tbg = tbg.darker(130);
        p.fillRect(0, y, width(), h, tbg);

        // Track label
        QColor tc = track.type == TrackType::Video ? QColor(100, 180, 255) : QColor(100, 255, 150);
        p.setPen(tc);
        p.setFont(QFont("sans-serif", 9));
        p.drawText(4, y, labelWidth() - 8, h, Qt::AlignLeft | Qt::AlignVCenter,
                   track.locked ? track.name + " \xF0\x9F\x94\x92" : track.name);

        // Clips
        for (const auto& clip : track.clips) {
            int x = timeToX(clip.trackOffset);
            int cw = std::max(2, timeToX(clip.trackOffset + clip.sourceDuration) - x);

            // Clip fill
            bool selected = (clip.id == m_selClipId && track.id == m_selTrackId);
            QColor cc = selected ? QColor(80, 150, 240) : QColor(60, 120, 200);
            if (!clip.enabled) cc = cc.darker(150);
            p.fillRect(x + 1, y + 1, cw - 1, h - 2, cc);

            // Selection border
            if (selected) {
                p.setPen(QPen(QColor(200, 220, 255), 2));
                p.drawRect(x, y, cw, h);
            } else {
                p.setPen(QColor(40, 90, 160));
                p.drawRect(x, y, cw, h);
            }

            // Trim handles (subtle)
            if (cw > 20) {
                p.fillRect(x, y + 4, 3, h - 8, QColor(0, 0, 0, 60));
                p.fillRect(x + cw - 3, y + 4, 3, h - 8, QColor(0, 0, 0, 60));
            }

            // Clip name
            p.setPen(Qt::white);
            p.setFont(QFont("sans-serif", 9));
            p.drawText(x + 8, y + 2, cw - 16, h - 4, Qt::AlignLeft | Qt::AlignVCenter,
                       clip.name.isEmpty() ? "Clip" : clip.name);
        }
    }

    // Playhead
    int phx = timeToX(m_playheadTime);
    if (phx >= labelWidth() && phx <= width()) {
        p.setPen(QPen(QColor(255, 80, 80), 2));
        p.drawLine(phx, 0, phx, height());

        // Playhead handle (triangle at top)
        QPolygonF tri;
        tri << QPointF(phx, 0) << QPointF(phx - 6, 8) << QPointF(phx + 6, 8);
        p.setBrush(QColor(255, 80, 80));
        p.setPen(Qt::NoPen);
        p.drawPolygon(tri);
    }

    // Duration end mark
    if (seq) {
        RationalTime dur = seq->calculateDuration();
        int durX = timeToX(dur);
        if (durX > labelWidth() && durX < width()) {
            p.setPen(QPen(QColor(80, 80, 80), 1, Qt::DashLine));
            p.drawLine(durX, 0, durX, height());
        }
    }
}

void TimelineWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_project) return;
    auto* seq = m_project->currentSequence();
    if (!seq) return;

    int mx = event->pos().x();
    int my = event->pos().y();

    // Check if clicking on playhead area first
    int phx = timeToX(m_playheadTime);
    if (std::abs(mx - phx) < 6) {
        m_dragMode = DragMode::Playhead;
        m_dragStartX = mx;
        return;
    }

    // Check clip clicks
    for (int i = 0; i < static_cast<int>(seq->tracks.size()); ++i) {
        int y = trackY(i);
        int h = trackHeight();
        if (my < y || my >= y + h) continue;

        for (auto& clip : seq->tracks[i].clips) {
            int cx = timeToX(clip.trackOffset);
            int cw = std::max(2, timeToX(clip.trackOffset + clip.sourceDuration) - cx);
            if (mx < cx || mx > cx + cw) continue;

            // Distinguish trim vs move
            if (mx - cx < 6 && cw > 20) {
                m_dragMode = DragMode::TrimIn;
            } else if (cx + cw - mx < 6 && cw > 20) {
                m_dragMode = DragMode::TrimOut;
            } else {
                m_dragMode = DragMode::Clip;
                m_selTrackId = seq->tracks[i].id;
                m_selClipId = clip.id;
                emit clipSelected(m_selTrackId, m_selClipId);
            }

            m_dragStartX = mx;
            m_dragTrackId = seq->tracks[i].id;
            m_dragClip = &clip;
            m_dragTrackIndex = i;
            m_dragOrigOffset = clip.trackOffset;
            m_dragOrigSourceStart = clip.sourceStart;
            m_dragOrigDuration = clip.sourceDuration;
            return;
        }
    }

    // Clicked empty space → deselect + move playhead
    m_selTrackId.clear();
    m_selClipId.clear();
    emit selectionCleared();
    m_playheadTime = xToTime(mx);
    emit playheadMoved(m_playheadTime);
    update();
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!m_dragClip || !m_project) { update(); return; }

    int dx = event->pos().x() - m_dragStartX;
    if (std::abs(dx) < 3) return;

    double seconds = static_cast<double>(dx) / m_pixelsPerSecond;
    RationalTime delta = RationalTime::fromSeconds(std::abs(seconds), 30);
    if (seconds < 0) delta = RationalTime(-delta.num, delta.den);

    switch (m_dragMode) {
    case DragMode::Clip: {
        // ponytail: direct project mutation, undo not wired for timeline drags
        m_project->moveClip(m_dragTrackId, m_dragClip->id,
                            m_dragOrigOffset + delta, -1);
        m_dragStartX = event->pos().x();
        break;
    }
    case DragMode::TrimIn: {
        RationalTime newStart = m_dragOrigSourceStart + delta;
        RationalTime newDur = m_dragOrigDuration - delta;
        if (newStart.num >= 0 && newDur.num > 0) {
            m_dragClip->sourceStart = newStart;
            m_dragClip->sourceDuration = newDur;
            m_dragStartX = event->pos().x();
            m_project->notifyChanged();
        }
        break;
    }
    case DragMode::TrimOut: {
        RationalTime newDur = m_dragOrigDuration + delta;
        if (newDur.num > 0) {
            m_dragClip->sourceDuration = newDur;
            m_dragStartX = event->pos().x();
            m_project->notifyChanged();
        }
        break;
    }
    case DragMode::Playhead: {
        m_playheadTime = xToTime(event->pos().x());
        emit playheadMoved(m_playheadTime);
        m_dragStartX = event->pos().x();
        return;
    }
    default: break;
    }
    update();
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent*) {
    m_dragMode = DragMode::None;
    m_dragClip = nullptr;
    m_dragTrackIndex = -1;
}

void TimelineWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double f = event->angleDelta().y() > 0 ? 1.3 : 1.0 / 1.3;
        setPixelsPerSecond(static_cast<int>(m_pixelsPerSecond * f));
        event->accept();
    }
}

void TimelineWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (!m_selClipId.isEmpty() && m_project) {
            m_project->removeClipFromTrack(m_selTrackId, m_selClipId);
            m_selClipId.clear();
            m_selTrackId.clear();
            emit selectionCleared();
            update();
        }
    }
}
