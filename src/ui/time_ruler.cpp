#include "time_ruler.h"
#include <QPainter>
#include <QMouseEvent>

TimeRuler::TimeRuler(QWidget* parent) : QWidget(parent) {
    setFixedHeight(24);
    setMouseTracking(true);
}

void TimeRuler::setPixelsPerSecond(int pps) {
    m_pixelsPerSecond = pps;
    update();
}

void TimeRuler::setPlayheadTime(const RationalTime& time) {
    m_playheadTime = time;
    update();
}

void TimeRuler::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(50, 50, 50));

    int w = width();
    int h = height();

    // Draw time ticks
    int startSec = 0;
    int endSec = (w - m_labelWidth) / m_pixelsPerSecond + 2;

    p.setPen(QColor(140, 140, 140));
    p.setFont(QFont("sans-serif", 8));

    int tickInterval = 1;
    int subticks = 4;

    // Auto-adjust tick spacing based on zoom
    if (m_pixelsPerSecond < 30) { tickInterval = 5; subticks = 5; }
    else if (m_pixelsPerSecond < 60) { tickInterval = 2; subticks = 4; }
    else if (m_pixelsPerSecond < 120) { tickInterval = 1; subticks = 4; }
    else { tickInterval = 1; subticks = 8; }

    for (int s = startSec; s <= endSec; s += tickInterval) {
        int x = s * m_pixelsPerSecond + m_labelWidth;
        if (x < m_labelWidth - 20 || x > w) continue;

        // Major tick
        p.drawLine(x, 4, x, h - 4);

        // Label
        int minutes = s / 60;
        int seconds = s % 60;
        p.drawText(x + 3, 2, 60, h - 4, Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0')));

        // Sub ticks
        for (int f = 1; f < subticks; ++f) {
            int fx = x + f * m_pixelsPerSecond * tickInterval / subticks;
            if (fx > m_labelWidth && fx < w)
                p.drawLine(fx, h - 8, fx, h - 4);
        }
    }

    // Playhead
    int phx = static_cast<int>(m_playheadTime.toSeconds() * m_pixelsPerSecond) + m_labelWidth;
    if (phx >= m_labelWidth && phx <= w) {
        p.setPen(QPen(QColor(255, 80, 80), 2));
        p.drawLine(phx, 0, phx, h);
        // Triangle handle
        QPolygonF tri;
        tri << QPointF(phx, 0) << QPointF(phx - 6, 8) << QPointF(phx + 6, 8);
        p.setBrush(QColor(255, 80, 80));
        p.setPen(Qt::NoPen);
        p.drawPolygon(tri);
    }
}

void TimeRuler::mousePressEvent(QMouseEvent* event) {
    int x = event->pos().x();
    if (x < m_labelWidth) return;
    int rx = x - m_labelWidth;
    RationalTime t = RationalTime::fromSeconds(static_cast<double>(rx) / m_pixelsPerSecond, 30);
    emit playheadClicked(t);
}

void TimeRuler::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        int x = event->pos().x();
        if (x < m_labelWidth) return;
        int rx = x - m_labelWidth;
        RationalTime t = RationalTime::fromSeconds(static_cast<double>(rx) / m_pixelsPerSecond, 30);
        emit playheadClicked(t);
    }
}
