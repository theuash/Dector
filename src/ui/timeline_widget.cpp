#include "timeline_widget.h"
#include "project.h"
#include <QPainter>
#include <QMouseEvent>
#include <algorithm>

TimelineWidget::TimelineWidget(Project* project, QWidget* parent)
    : QWidget(parent), m_project(project) {
    setMinimumHeight(120);
    setMouseTracking(true);
}

void TimelineWidget::setProject(Project* project) {
    m_project = project;
    update();
}

int TimelineWidget::trackY(int index) const {
    return 4 + index * (trackHeight() + 4);
}

int TimelineWidget::timeToX(const RationalTime& t) const {
    return labelWidth() + static_cast<int>(t.toSeconds() * m_pixelsPerSecond);
}

RationalTime TimelineWidget::xToTime(int x) const {
    int relX = std::max(0, x - labelWidth());
    return RationalTime::fromSeconds(static_cast<double>(relX) / m_pixelsPerSecond, 30.0);
}

void TimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(30, 30, 30));

    auto* seq = m_project ? m_project->currentSequence() : nullptr;
    if (!seq) {
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter, "No sequence");
        return;
    }

    for (int i = 0; i < static_cast<int>(seq->tracks.size()); ++i) {
        const auto& track = seq->tracks[i];
        int y = trackY(i);
        int h = trackHeight();

        QColor trackBg = (i % 2 == 0) ? QColor(45, 45, 45) : QColor(38, 38, 38);
        p.fillRect(0, y, width(), h, trackBg);

        p.setPen(track.type == TrackType::Video ? QColor(100, 180, 255) : QColor(100, 255, 150));
        p.setFont(QFont("sans-serif", 9));
        p.drawText(4, y, labelWidth() - 8, h, Qt::AlignLeft | Qt::AlignVCenter, track.name);

        for (const auto& clip : track.clips) {
            int x = timeToX(clip.trackOffset);
            int cw = timeToX(clip.sourceDuration) - labelWidth();
            if (cw < 2) cw = 2;

            QColor clipColor(60, 120, 200);
            if (!clip.enabled) clipColor = clipColor.darker(150);
            p.fillRect(x + 1, y + 1, cw - 1, h - 2, clipColor);

            p.setPen(QColor(40, 90, 160));
            p.drawRect(x, y, cw, h);

            p.setPen(Qt::white);
            p.setFont(QFont("sans-serif", 9));
            QRect textRect(x + 4, y + 2, cw - 8, h - 4);
            p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                       clip.name.isEmpty() ? "Clip" : clip.name);
        }
    }
}

void TimelineWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_project) return;
    auto* seq = m_project->currentSequence();
    if (!seq) return;

    int mx = event->pos().x();
    int my = event->pos().y();

    for (int i = 0; i < static_cast<int>(seq->tracks.size()); ++i) {
        int y = trackY(i);
        int h = trackHeight();
        if (my < y || my >= y + h) continue;

        for (auto& clip : seq->tracks[i].clips) {
            int cx = timeToX(clip.trackOffset);
            int cw = timeToX(clip.sourceDuration) - labelWidth();
            if (mx < cx || mx > cx + cw) continue;

            m_dragging = true;
            m_dragClip = &clip;
            m_dragTrack = i;
            m_dragStartX = mx;
            return;
        }
    }
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!m_dragging || !m_dragClip || !m_project) return;
    int dx = event->pos().x() - m_dragStartX;
    if (std::abs(dx) < 3) return;

    double seconds = static_cast<double>(dx) / m_pixelsPerSecond;
    RationalTime delta = RationalTime::fromSeconds(std::abs(seconds), 30.0);
    if (seconds < 0) delta = RationalTime(-delta.num, delta.den);

    // ponytail: direct project mutation, undo not wired for timeline drags yet
    QString trackId = m_project->currentSequence()->tracks[m_dragTrack].id;
    m_project->moveClip(trackId, m_dragClip->id,
                        m_dragClip->trackOffset + delta, -1);
    m_dragStartX = event->pos().x();
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent*) {
    m_dragging = false;
    m_dragClip = nullptr;
    m_dragTrack = -1;
}
