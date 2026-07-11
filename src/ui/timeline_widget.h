#pragma once
#include <QWidget>
#include "rational_time.h"

class Project;
struct Clip;

class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimelineWidget(Project* project, QWidget* parent = nullptr);
    void setProject(Project* project);

    QSize sizeHint() const override { return QSize(800, 200); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    int trackY(int index) const;
    int labelWidth() const { return 80; }
    int trackHeight() const { return 40; }
    int timeToX(const RationalTime& t) const;
    RationalTime xToTime(int x) const;

    Project* m_project = nullptr;
    int m_pixelsPerSecond = 80;
    int m_dragStartX = 0;
    bool m_dragging = false;
    Clip* m_dragClip = nullptr;
    int m_dragTrack = -1;
};
