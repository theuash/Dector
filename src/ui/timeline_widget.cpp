#include "timeline_widget.h"
#include "project.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QUuid>
#include <algorithm>
#include <limits>

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

void TimelineWidget::setTool(TimelineTool tool) {
    m_tool = tool;
    setCursor(tool == TimelineTool::Razor ? Qt::SplitHCursor : Qt::ArrowCursor);
}

void TimelineWidget::setPlayheadTime(const RationalTime& time) {
    m_playheadTime = time;
    update();
}

int TimelineWidget::trackIndexAt(int y) const {
    for (int i = 0;; i++) {
        int ty = trackY(i);
        if (y >= ty && y < ty + m_trackH) return i;
        if (ty > y) return -1;
    }
}

int TimelineWidget::timeToX(const RationalTime& t) const {
    return labelWidth() + static_cast<int>(t.toSeconds() * m_pixelsPerSecond);
}

RationalTime TimelineWidget::xToTime(int x) const {
    int rx = std::max(0, x - labelWidth());
    return RationalTime::fromSeconds(static_cast<double>(rx) / m_pixelsPerSecond, 30.0);
}

int TimelineWidget::snapX(int x, int avoidX) const {
    if (!m_project) return x;
    auto* seq = m_project->currentSequence();
    if (!seq) return x;

    int best = x;
    int bestDist = m_snapThreshold;

    // Snap to playhead
    int phx = timeToX(m_playheadTime);
    int d = std::abs(x - phx);
    if (d < bestDist && phx != avoidX) { bestDist = d; best = phx; }

    // Snap to clip edges
    for (const auto& track : seq->tracks) {
        for (const auto& clip : track.clips) {
            for (int ex : {timeToX(clip.trackOffset), timeToX(clip.endTime())}) {
                d = std::abs(x - ex);
                if (d < bestDist && ex != avoidX) { bestDist = d; best = ex; }
            }
        }
    }
    return best;
}

RationalTime TimelineWidget::snapTime(const RationalTime& t, const QString& skipClipId) const {
    if (!m_project) return t;
    auto* seq = m_project->currentSequence();
    if (!seq) return t;

    RationalTime best = t;
    int bestDist = m_snapThreshold;

    auto check = [&](const RationalTime& ct) {
        RationalTime d = ct > t ? ct - t : t - ct;
        int dist = static_cast<int>(d.toSeconds() * m_pixelsPerSecond);
        if (dist < bestDist) { bestDist = dist; best = ct; }
    };

    check(m_playheadTime);
    for (const auto& track : seq->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.id == skipClipId) continue;
            check(clip.trackOffset);
            check(clip.endTime());
        }
    }
    return best;
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

    // Divider between header and clips
    p.fillRect(labelWidth() - 1, 0, 1, height(), QColor(60, 60, 60));

    int nTracks = static_cast<int>(seq->tracks.size());
    for (int i = 0; i < nTracks; ++i) {
        const auto& track = seq->tracks[i];
        int y = trackY(i);
        int h = m_trackH;

        // Track background
        QColor tbg = (i % 2 == 0) ? QColor(42, 42, 42) : QColor(36, 36, 36);
        if (!track.enabled) tbg = tbg.darker(130);
        p.fillRect(0, y, width(), h, tbg);

        // Track row separator
        p.setPen(QColor(50, 50, 50));
        p.drawLine(0, y, width(), y);

        // Track header
        paintTrackHeader(p, i, track);

        // Clips
        for (const auto& clip : track.clips) {
            int x = timeToX(clip.trackOffset);
            int cw = std::max(2, timeToX(clip.trackOffset + clip.sourceDuration) - x);

            bool selected = (clip.id == m_selClipId && track.id == m_selTrackId);
            QColor cc = selected ? QColor(70, 140, 230) : QColor(55, 110, 190);
            if (!clip.enabled) cc = cc.darker(150);

            // Clip body
            p.fillRect(x + 1, y + 1, cw - 1, h - 2, cc);

            // Selection border
            p.setPen(selected ? QPen(QColor(180, 210, 255), 2)
                              : QPen(QColor(40, 80, 140), 1));
            p.drawRect(x, y, cw, h);

            // Trim handles
            if (cw > 24 && m_tool == TimelineTool::Selection) {
                p.fillRect(x, y + 6, 3, h - 12, QColor(0, 0, 0, 50));
                p.fillRect(x + cw - 3, y + 6, 3, h - 12, QColor(0, 0, 0, 50));
            }

            // Clip label
            p.setPen(Qt::white);
            p.setFont(QFont("sans-serif", 9));
            QRect tr(x + 8, y + 2, cw - 16, h - 4);
            QString label = clip.name.isEmpty() ? "Clip" : clip.name;
            if (m_tool == TimelineTool::Razor && cw > 40) {
                p.setPen(QColor(255, 200, 80));
            }
            p.drawText(tr, Qt::AlignLeft | Qt::AlignVCenter, label);
        }
    }

    // Duration end mark
    RationalTime dur = seq->calculateDuration();
    int durX = timeToX(dur);
    if (durX > labelWidth() && durX < width()) {
        p.setPen(QPen(QColor(70, 70, 70), 1, Qt::DashLine));
        p.drawLine(durX, 0, durX, height());
    }

    // Playhead
    int phx = timeToX(m_playheadTime);
    if (phx >= labelWidth() && phx <= width()) {
        p.setPen(QPen(QColor(255, 70, 70), 2));
        p.drawLine(phx, 0, phx, height());
        QPolygonF tri;
        tri << QPointF(phx, 0) << QPointF(phx - 6, 8) << QPointF(phx + 6, 8);
        p.setBrush(QColor(255, 70, 70));
        p.setPen(Qt::NoPen);
        p.drawPolygon(tri);
    }
}

void TimelineWidget::paintTrackHeader(QPainter& p, int index, const Track& track) {
    int y = trackY(index);
    int h = m_trackH;

    // Header background
    QColor bg = track.type == TrackType::Video ? QColor(35, 45, 60) : QColor(45, 40, 35);
    p.fillRect(0, y, labelWidth(), h, bg);

    // Type icon
    QString icon = track.type == TrackType::Video ? "\xF0\x9F\x8E\xAC" : "\xF0\x9F\x94\x8A";
    p.setFont(QFont("sans-serif", 9));
    p.drawText(4, y, 16, h, Qt::AlignCenter, icon);

    // Track name
    QColor tc = track.type == TrackType::Video ? QColor(130, 190, 255) : QColor(180, 255, 150);
    p.setPen(track.locked ? QColor(180, 180, 180) : tc);
    p.setFont(QFont("sans-serif", 8));
    p.drawText(22, y, labelWidth() - 24, h / 2, Qt::AlignLeft | Qt::AlignVCenter, track.name);

    // Mute / Solo / Lock indicators
    int btnY = y + h / 2 + 1;
    int btnH = h / 2 - 2;
    p.setFont(QFont("sans-serif", 7));

    // Mute (V)
    if (!track.enabled) {
        p.fillRect(4, btnY, 18, btnH, QColor(180, 60, 60));
        p.setPen(Qt::white);
        p.drawText(4, btnY, 18, btnH, Qt::AlignCenter, "M");
    } else {
        p.setPen(QColor(120, 120, 120));
        p.drawText(4, btnY, 18, btnH, Qt::AlignCenter, "M");
    }

    // Lock
    if (track.locked) {
        p.fillRect(24, btnY, 18, btnH, QColor(180, 150, 40));
        p.setPen(Qt::white);
        p.drawText(24, btnY, 18, btnH, Qt::AlignCenter, "L");
    } else {
        p.setPen(QColor(120, 120, 120));
        p.drawText(24, btnY, 18, btnH, Qt::AlignCenter, "L");
    }

    // Timeline tool indicator
    if (m_tool == TimelineTool::Razor) {
        p.setPen(QColor(255, 200, 80));
        p.setFont(QFont("sans-serif", 10));
        p.drawText(48, btnY, 30, btnH, Qt::AlignCenter, "\xE2\x9C\x82");
    }
}

void TimelineWidget::razorSplitAt(const QPoint& pos) {
    if (!m_project) return;
    auto* seq = m_project->currentSequence();
    if (!seq) return;

    RationalTime splitTime = xToTime(pos.x());

    for (int i = 0; i < static_cast<int>(seq->tracks.size()); ++i) {
        int y = trackY(i);
        if (pos.y() < y || pos.y() >= y + m_trackH) continue;

        auto& clips = seq->tracks[i].clips;
        for (auto it = clips.begin(); it != clips.end(); ++it) {
            if (splitTime > it->trackOffset && splitTime < it->endTime()) {
                RationalTime splitOff = splitTime - it->trackOffset;

                Clip second = *it;
                second.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
                second.sourceStart = it->sourceStart + splitOff;
                second.sourceDuration = it->sourceDuration - splitOff;
                second.trackOffset = splitTime;

                it->sourceDuration = splitOff;
                clips.insert(it + 1, second);
                m_project->notifyChanged();
                return;
            }
        }
    }
}

void TimelineWidget::mousePressEvent(QMouseEvent* event) {
    int mx = event->pos().x();
    int my = event->pos().y();

    if (!m_project) return;
    auto* seq = m_project->currentSequence();
    if (!seq) return;

    // Handle click on track header buttons
    if (mx < labelWidth()) {
        int ti = trackIndexAt(my);
        if (ti < 0 || ti >= static_cast<int>(seq->tracks.size())) return;
        int y = trackY(ti);
        int relY = my - y;
        auto& track = seq->tracks[ti];

        // Mute click (first 22px, bottom half)
        if (relY > m_trackH / 2 && mx < 22) {
            track.enabled = !track.enabled;
            m_project->notifyChanged();
            return;
        }
        // Lock click (next 22px, bottom half)
        if (relY > m_trackH / 2 && mx >= 22 && mx < 44) {
            track.locked = !track.locked;
            m_project->notifyChanged();
            return;
        }
        return;
    }

    // Razor tool
    if (m_tool == TimelineTool::Razor) {
        razorSplitAt(event->pos());
        return;
    }

    // Check playhead drag
    int phx = timeToX(m_playheadTime);
    if (std::abs(mx - phx) < 6) {
        m_dragMode = DragMode::Playhead;
        m_dragStartX = mx;
        return;
    }

    // Check clip clicks
    for (int i = 0; i < static_cast<int>(seq->tracks.size()); ++i) {
        int y = trackY(i);
        if (my < y || my >= y + m_trackH) continue;
        if (seq->tracks[i].locked) continue;

        for (auto& clip : seq->tracks[i].clips) {
            int cx = timeToX(clip.trackOffset);
            int cw = std::max(2, timeToX(clip.trackOffset + clip.sourceDuration) - cx);
            if (mx < cx || mx > cx + cw) continue;

            // Trim vs move
            if (mx - cx < 6 && cw > 24) {
                m_dragMode = DragMode::TrimIn;
            } else if (cx + cw - mx < 6 && cw > 24) {
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

    // Empty space → deselect + move playhead
    m_selTrackId.clear();
    m_selClipId.clear();
    emit selectionCleared();
    m_playheadTime = snapTime(xToTime(mx));
    emit playheadMoved(m_playheadTime);
    update();
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!m_project) { update(); return; }

    int mx = event->pos().x();

    if (m_dragMode == DragMode::Playhead) {
        m_playheadTime = snapTime(xToTime(mx));
        emit playheadMoved(m_playheadTime);
        update();
        return;
    }

    if (!m_dragClip) { update(); return; }

    int dx = mx - m_dragStartX;
    if (std::abs(dx) < 3) return;

    double seconds = static_cast<double>(dx) / m_pixelsPerSecond;
    RationalTime delta = RationalTime::fromSeconds(std::abs(seconds), 30);
    if (seconds < 0) delta = RationalTime(-delta.num, delta.den);

    switch (m_dragMode) {
    case DragMode::Clip: {
        RationalTime newOff = m_dragOrigOffset + delta;
        newOff = snapTime(newOff, m_dragClip->id);
        // Adjust delta to snapped position
        RationalTime snapDelta = newOff - m_dragOrigOffset;
        m_project->moveClip(m_dragTrackId, m_dragClip->id, newOff, -1);
        m_dragStartX = mx;
        break;
    }
    case DragMode::TrimIn: {
        RationalTime newStart = m_dragOrigSourceStart + delta;
        RationalTime newDur = m_dragOrigDuration - delta;
        if (newStart.num >= 0 && newDur.num > 0) {
            m_dragClip->sourceStart = newStart;
            m_dragClip->sourceDuration = newDur;
            m_dragStartX = mx;
            m_project->notifyChanged();
        }
        break;
    }
    case DragMode::TrimOut: {
        RationalTime newDur = m_dragOrigDuration + delta;
        if (newDur.num > 0) {
            m_dragClip->sourceDuration = newDur;
            m_dragStartX = mx;
            m_project->notifyChanged();
        }
        break;
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
