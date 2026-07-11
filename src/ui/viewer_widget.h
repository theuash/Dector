#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QImage>
#include "rational_time.h"

class Project;

class ViewerWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit ViewerWidget(QWidget* parent = nullptr);
    void setProject(Project* project) { m_project = project; }

    void setFrame(const QImage& frame);
    void setFrameSize(int w, int h) { m_frameW = w; m_frameH = h; }

public slots:
    void setCurrentTime(const RationalTime& time);
    void setDuration(const RationalTime& duration);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

signals:
    void playPaused();

private:
    QString formatTime(const RationalTime& t) const;

    Project* m_project = nullptr;
    QImage m_currentFrame;
    RationalTime m_currentTime{0, 30};
    RationalTime m_duration{300, 30};
    int m_frameW = 1920;
    int m_frameH = 1080;
};
