#include "viewer_widget.h"
#include "project.h"
#include <QPainter>
#include <QMouseEvent>

ViewerWidget::ViewerWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setMinimumSize(320, 240);
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

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(13, 13, 13));

    // Frame rectangle (16:9 centered)
    int m = 40;
    QRect fr = rect().adjusted(m, m, -m, -m);
    float aspect = 16.0f / 9.0f;
    if (fr.width() > fr.height() * aspect) {
        int w = fr.height() * aspect;
        fr.setLeft(fr.center().x() - w / 2);
        fr.setWidth(w);
    } else {
        int h = fr.width() / aspect;
        fr.setTop(fr.center().y() - h / 2);
        fr.setHeight(h);
    }
    painter.fillRect(fr, QColor(20, 20, 20));
    painter.setPen(QColor(60, 60, 60));
    painter.drawRect(fr);

    // Crosshair
    painter.setPen(QPen(QColor(50, 50, 50), 1));
    painter.drawLine(fr.center().x(), fr.top() + 10, fr.center().x(), fr.bottom() - 10);
    painter.drawLine(fr.left() + 10, fr.center().y(), fr.right() - 10, fr.center().y());

    // Timecode overlay
    int currentF = m_currentTime.toFrames(30);
    double sec = m_currentTime.toSeconds();
    int hh = static_cast<int>(sec) / 3600;
    int mm = (static_cast<int>(sec) % 3600) / 60;
    int ss = static_cast<int>(sec) % 60;
    int ff = currentF % 30;

    painter.setFont(QFont("monospace", 11));
    QString ts = QString("%1:%2:%3:%4")
        .arg(hh, 2, 10, QChar('0'))
        .arg(mm, 2, 10, QChar('0'))
        .arg(ss, 2, 10, QChar('0'))
        .arg(ff, 2, 10, QChar('0'));

    QRect tr(rect().right() - 130, rect().bottom() - 30, 120, 22);
    painter.fillRect(tr, QColor(0, 0, 0, 180));
    painter.setPen(QColor(200, 200, 200));
    painter.drawText(tr, Qt::AlignCenter, ts);

    // Info
    painter.setPen(QColor(100, 100, 100));
    painter.setFont(QFont("sans-serif", 10));
    painter.drawText(rect().adjusted(10, 10, -10, -10), Qt::TopLeftCorner, "1920x1080 | 30 fps");

    painter.end();
}

void ViewerWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void ViewerWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) emit playPaused();
}
