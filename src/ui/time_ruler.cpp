#include "time_ruler.h"
#include <QPainter>

TimeRuler::TimeRuler(QWidget* parent) : QWidget(parent) {
    setFixedHeight(24);
}

void TimeRuler::setScrollOffset(int offset) {
    m_scrollOffset = offset;
    update();
}

void TimeRuler::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(50, 50, 50));
    p.setPen(QColor(180, 180, 180));
    p.setFont(QFont("sans-serif", 8));

    int startSec = m_scrollOffset / m_pixelsPerSecond;
    int endSec = (m_scrollOffset + width()) / m_pixelsPerSecond + 1;

    for (int s = startSec; s <= endSec; ++s) {
        int x = s * m_pixelsPerSecond - m_scrollOffset + m_labelWidth;
        if (x < m_labelWidth || x > width()) continue;

        p.drawLine(x, 4, x, height() - 4);

        int minutes = s / 60;
        int seconds = s % 60;
        p.drawText(x + 3, 2, 60, height() - 4, Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0')));

        for (int f = 1; f < 4; ++f) {
            int fx = x + f * m_pixelsPerSecond / 4;
            if (fx > m_labelWidth && fx < width())
                p.drawLine(fx, height() - 8, fx, height() - 4);
        }
    }
}
