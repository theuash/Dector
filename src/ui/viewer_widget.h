#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include "rational_time.h"
#include <QElapsedTimer>

class Project;

class ViewerWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit ViewerWidget(QWidget* parent = nullptr);
    void setProject(Project* project) { m_project = project; }

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
    void stopped();
    void seeked(const RationalTime& time);

private:
    Project* m_project = nullptr;
    RationalTime m_currentTime{0, 30};
    RationalTime m_duration{300, 30};
};
