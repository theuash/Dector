#pragma once
#include <QWidget>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QComboBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>

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
    void addParamRow(QFormLayout* form, const QString& label, 
                     QDoubleSpinBox*& spin, QSlider*& slider,
                     double min, double max, double step);

    void updateValues();
    void onSpinChanged();
    void onBlendChanged(int index);

    Project* m_project = nullptr;
    QString m_trackId;
    QString m_clipId;

    QLabel* m_clipNameLabel = nullptr;

    QDoubleSpinBox *m_posXSpin = nullptr, *m_posYSpin = nullptr;
    QDoubleSpinBox *m_scaleXSpin = nullptr, *m_scaleYSpin = nullptr;
    QDoubleSpinBox *m_rotSpin = nullptr, *m_opSpin = nullptr;
    QDoubleSpinBox *m_anchorXSpin = nullptr, *m_anchorYSpin = nullptr;

    QSlider *m_posXSlider = nullptr, *m_posYSlider = nullptr;
    QSlider *m_scaleXSlider = nullptr, *m_scaleYSlider = nullptr;
    QSlider *m_rotSlider = nullptr, *m_opSlider = nullptr;
    QSlider *m_anchorXSlider = nullptr, *m_anchorYSlider = nullptr;

    QComboBox* m_blendMode = nullptr;
    QCheckBox* m_motionBlur = nullptr;
};