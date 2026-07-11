#pragma once
#include <QWidget>
#include "rational_time.h"

class TimeRuler : public QWidget {
    Q_OBJECT
public:
    explicit TimeRuler(QWidget* parent = nullptr);
    void setPixelsPerSecond(int pps);
    void setPlayheadTime(const RationalTime& time);

signals:
    void playheadClicked(const RationalTime& time);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    QSize sizeHint() const override { return QSize(800, 24); }

private:
    int m_pixelsPerSecond = 80;
    int m_labelWidth = 80;
    RationalTime m_playheadTime{0, 30};
};
