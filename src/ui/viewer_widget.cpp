#include "viewer_widget.h"
#include "project.h"
#include <QPainter>
#include <QMouseEvent>
#include <cmath>

ViewerWidget::ViewerWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setMinimumSize(320, 240);
}

void ViewerWidget::setFrame(const QImage& frame) {
    m_currentFrame = frame;
    if (!frame.isNull()) {
        m_frameW = frame.width();
        m_frameH = frame.height();
    }
    update();
}

void ViewerWidget::setCurrentTime(const RationalTime& time) {
    m_currentTime = time;
    update();
}

void ViewerWidget::setDuration(const RationalTime& duration) {
    m_duration = duration;
}

void ViewerWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
}

void ViewerWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // Background
    p.fillRect(rect(), QColor(13, 13, 13));

    // Draw frame centered with aspect ratio
    if (!m_currentFrame.isNull()) {
        QPixmap px = QPixmap::fromImage(m_currentFrame);
        QSize sz = px.size();
        sz.scale(rect().size(), Qt::KeepAspectRatio);
        QPoint off((width() - sz.width()) / 2, (height() - sz.height()) / 2);
        p.drawPixmap(off.x(), off.y(), sz.width(), sz.height(), px);

        // Thin border around frame
        p.setPen(QPen(QColor(60, 60, 60), 1));
        p.drawRect(off.x(), off.y(), sz.width(), sz.height());
    } else {
        // Placeholder: empty frame area
        int m = 40;
        QRect fr = rect().adjusted(m, m, -m, -m);
        float aspect = static_cast<float>(m_frameW) / m_frameH;
        if (fr.width() > fr.height() * aspect) {
            int w = fr.height() * aspect;
            fr.setLeft(fr.center().x() - w / 2);
            fr.setWidth(w);
        } else {
            int h = fr.width() / aspect;
            fr.setTop(fr.center().y() - h / 2);
            fr.setHeight(h);
        }
        p.fillRect(fr, QColor(20, 20, 20));
        p.setPen(QColor(60, 60, 60));
        p.drawRect(fr);

        // Crosshair
        p.setPen(QPen(QColor(50, 50, 50), 1));
        p.drawLine(fr.center().x(), fr.top() + 10, fr.center().x(), fr.bottom() - 10);
        p.drawLine(fr.left() + 10, fr.center().y(), fr.right() - 10, fr.center().y());

        p.setPen(QColor(80, 80, 80));
        p.setFont(QFont("sans-serif", 18));
        p.drawText(fr, Qt::AlignCenter, "No Media");
    }

    // Top-left: resolution info
    p.setPen(QColor(120, 120, 120));
    p.setFont(QFont("sans-serif", 10));
    p.drawText(rect().adjusted(10, 10, -10, -10), Qt::TopLeftCorner,
               QString("%1x%2 | %3 fps").arg(m_frameW).arg(m_frameH).arg(30));

    // Bottom-right: timecode
    QString ts = formatTime(m_currentTime);
    QRect tr(rect().right() - 130, rect().bottom() - 28, 120, 20);
    p.fillRect(tr, QColor(0, 0, 0, 180));
    p.setPen(QColor(220, 220, 220));
    p.setFont(QFont("monospace", 11));
    p.drawText(tr, Qt::AlignCenter, ts);

    // Bottom-center: duration bar
    if (m_duration > RationalTime(0, 1)) {
        int barH = 3;
        int barW = rect().width() - 80;
        int barX = 40;
        int barY = rect().bottom() - 12;
        p.fillRect(barX, barY, barW, barH, QColor(60, 60, 60));

        double frac = m_duration.toSeconds() > 0
            ? std::min(1.0, m_currentTime.toSeconds() / m_duration.toSeconds()) : 0.0;
        int progW = static_cast<int>(barW * frac);
        p.fillRect(barX, barY, progW, barH, QColor(200, 80, 80));
    }

    p.end();
}

void ViewerWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void ViewerWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) emit playPaused();
}

QString ViewerWidget::formatTime(const RationalTime& t) const {
    int frame = t.toFrames(30);
    double sec = t.toSeconds();
    int hh = static_cast<int>(sec) / 3600;
    int mm = (static_cast<int>(sec) % 3600) / 60;
    int ss = static_cast<int>(sec) % 60;
    int ff = frame % 30;
    return QString("%1:%2:%3:%4")
        .arg(hh, 2, 10, QChar('0'))
        .arg(mm, 2, 10, QChar('0'))
        .arg(ss, 2, 10, QChar('0'))
        .arg(ff, 2, 10, QChar('0'));
}
