#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class Project;

class ViewerWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit ViewerWidget(QWidget* parent = nullptr);
    void setProject(Project* project) { m_project = project; update(); }

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    Project* m_project = nullptr;
};
