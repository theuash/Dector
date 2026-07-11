#include "viewer_widget.h"
#include "project.h"
#include <QPainter>
#include <QFont>

ViewerWidget::ViewerWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setMinimumSize(320, 240);
}

void ViewerWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
}

void ViewerWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ponytail: placeholder view until week 2 when real rendering is wired
    QPainter painter(this);
    painter.setPen(Qt::gray);
    painter.setFont(QFont("sans-serif", 24));
    painter.drawText(rect(), Qt::AlignCenter, "Viewer");
    painter.end();
}

void ViewerWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}
