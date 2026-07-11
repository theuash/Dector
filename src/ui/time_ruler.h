#pragma once
#include <QWidget>

class TimeRuler : public QWidget {
    Q_OBJECT
public:
    explicit TimeRuler(QWidget* parent = nullptr);

    void setScrollOffset(int offset);
    QSize sizeHint() const override { return QSize(800, 24); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_scrollOffset = 0;
    int m_pixelsPerSecond = 80;
    int m_labelWidth = 80;
};
