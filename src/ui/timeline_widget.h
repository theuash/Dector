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

    void setPixelsPerSecond(int pps);
    int pixelsPerSecond() const { return m_pixelsPerSecond; }

public slots:
    void setPlayheadTime(const RationalTime& time);

signals:
    void clipSelected(const QString& trackId, const QString& clipId);
    void selectionCleared();
    void playheadMoved(const RationalTime& time);
    void zoomChanged(int pixelsPerSecond);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    QSize sizeHint() const override { return QSize(800, 200); }

private:
    int trackY(int index) const;
    int labelWidth() const { return 80; }
    int trackHeight() const { return 36; }
    int timeToX(const RationalTime& t) const;
    RationalTime xToTime(int x) const;

    enum class DragMode { None, Clip, TrimIn, TrimOut, Playhead };

    Project* m_project = nullptr;
    int m_pixelsPerSecond = 80;
    RationalTime m_playheadTime{0, 30};

    QString m_selTrackId;
    QString m_selClipId;

    DragMode m_dragMode = DragMode::None;
    int m_dragStartX = 0;
    QString m_dragTrackId;
    Clip* m_dragClip = nullptr;
    int m_dragTrackIndex = -1;
    RationalTime m_dragOrigOffset;
    RationalTime m_dragOrigSourceStart;
    RationalTime m_dragOrigDuration;
};
