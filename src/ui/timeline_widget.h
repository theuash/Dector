#pragma once
#include <QWidget>
#include "rational_time.h"

class Project;
struct Clip;

enum class TimelineTool { Selection, Razor };

class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimelineWidget(Project* project, QWidget* parent = nullptr);
    void setProject(Project* project);

    void setPixelsPerSecond(int pps);
    int pixelsPerSecond() const { return m_pixelsPerSecond; }
    void setTool(TimelineTool tool);
    TimelineTool tool() const { return m_tool; }

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
    void paintTrackHeader(QPainter& p, int index, const class Track& track);
    void razorSplitAt(const QPoint& pos);
    int snapX(int x, int avoidX = -1) const;
    RationalTime snapTime(const RationalTime& t, const QString& skipClipId = {}) const;

    struct TrackLayout { int y, h; };
    int trackY(int index) const { return 2 + index * (m_trackH + 2); }
    int trackIndexAt(int y) const;
    int labelWidth() const { return 120; }
    int trackHeight() const { return m_trackH; }
    int timeToX(const RationalTime& t) const;
    RationalTime xToTime(int x) const;

    static constexpr int m_trackH = 36;
    static constexpr int m_snapThreshold = 8; // pixels

    Project* m_project = nullptr;
    TimelineTool m_tool = TimelineTool::Selection;
    int m_pixelsPerSecond = 80;
    RationalTime m_playheadTime{0, 30};

    QString m_selTrackId;
    QString m_selClipId;

    enum class DragMode { None, Clip, TrimIn, TrimOut, Playhead };
    DragMode m_dragMode = DragMode::None;
    int m_dragStartX = 0;
    QString m_dragTrackId;
    Clip* m_dragClip = nullptr;
    int m_dragTrackIndex = -1;
    RationalTime m_dragOrigOffset;
    RationalTime m_dragOrigSourceStart;
    RationalTime m_dragOrigDuration;
};
