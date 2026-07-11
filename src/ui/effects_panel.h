#pragma once
#include <QWidget>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>

class Project;

class EffectsPanel : public QWidget {
    Q_OBJECT
public:
    explicit EffectsPanel(Project* project, QWidget* parent = nullptr);
    void setProject(Project* project);
    void showClip(const QString& trackId, const QString& clipId);
    void clearSelection();

signals:
    void paramChanged(const QString& trackId, const QString& clipId,
                      const QString& paramName, double value);
    void blendChanged(const QString& trackId, const QString& clipId, int mode);

private:
    void setupUI();
    void updateValues();
    void onSpinChanged();
    void onBlendChanged(int index);

    Project* m_project = nullptr;
    QString m_trackId;
    QString m_clipId;

    QLabel* m_clipNameLabel;
    QDoubleSpinBox* m_posX;
    QDoubleSpinBox* m_posY;
    QDoubleSpinBox* m_scaleX;
    QDoubleSpinBox* m_scaleY;
    QDoubleSpinBox* m_rotation;
    QDoubleSpinBox* m_opacity;
    QComboBox* m_blendMode;
};
