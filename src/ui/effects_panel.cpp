#include "effects_panel.h"
#include "project.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>

EffectsPanel::EffectsPanel(Project* project, QWidget* parent)
    : QWidget(parent), m_project(project) {
    setupUI();
    clearSelection();
}

void EffectsPanel::setProject(Project* project) {
    m_project = project;
    clearSelection();
}

void EffectsPanel::showClip(const QString& trackId, const QString& clipId) {
    m_trackId = trackId;
    m_clipId = clipId;
    updateValues();
}

void EffectsPanel::clearSelection() {
    m_trackId.clear();
    m_clipId.clear();
    m_clipNameLabel->setText("No clip selected");
    setEnabled(false);
}

void EffectsPanel::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(8);

    m_clipNameLabel = new QLabel("No clip selected", this);
    m_clipNameLabel->setStyleSheet("font-weight: bold; color: #ddd; padding: 4px; font-size: 11px;");
    mainLayout->addWidget(m_clipNameLabel);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea { border: none; background: #1e1e1e; }");
    mainLayout->addWidget(scroll, 1);

    auto* content = new QWidget(scroll);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(12);
    scroll->setWidget(content);

    // Transform group
    auto* transformGroup = new QGroupBox("Transform", content);
    transformGroup->setStyleSheet("QGroupBox { color: #aaa; font-weight: bold; border: 1px solid #333; border-radius: 4px; margin-top: 12px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }");
    auto* tfLayout = new QFormLayout(transformGroup);
    tfLayout->setContentsMargins(8, 16, 8, 8);
    tfLayout->setSpacing(6);
    layout->addWidget(transformGroup);

    auto addRow = [&](const QString& label, QDoubleSpinBox*& spin, QSlider*& slider,
                        double min, double max, double step, double def) {
        spin = new QDoubleSpinBox(transformGroup);
        spin->setRange(min, max);
        spin->setSingleStep(step);
        spin->setDecimals(step < 1 ? 2 : 1);
        spin->setValue(def);
        spin->setButtonSymbols(QAbstractSpinBox::PlusMinus);
        spin->setStyleSheet("QDoubleSpinBox { background: #2a2a2a; color: #ddd; border: 1px solid #444; padding: 4px; border-radius: 3px; } QDoubleSpinBox:focus { border-color: #3a70b0; }");

        slider = new QSlider(Qt::Horizontal, transformGroup);
        slider->setRange(0, 1000);
        slider->setStyleSheet("QSlider::groove:horizontal { height: 4px; background: #333; border-radius: 2px; } QSlider::handle:horizontal { background: #888; width: 12px; height: 12px; margin: -4px 0; border-radius: 6px; } QSlider::sub-page:horizontal { background: #3a70b0; border-radius: 2px; }");

        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EffectsPanel::onSpinChanged);
        connect(slider, &QSlider::valueChanged, this, [this, spin](int v) {
            double val = spin->minimum() + (v / 1000.0) * (spin->maximum() - spin->minimum());
            spin->blockSignals(true);
            spin->setValue(val);
            spin->blockSignals(false);
        });

        tfLayout->addRow(label, spin);
        tfLayout->addRow("", slider);
    };

    addRow("Position X:", m_posXSpin, m_posXSlider, -2000, 2000, 1, 0);
    addRow("Position Y:", m_posYSpin, m_posYSlider, -2000, 2000, 1, 0);
    addRow("Scale X:", m_scaleXSpin, m_scaleXSlider, 0.01, 10, 0.01, 1);
    addRow("Scale Y:", m_scaleYSpin, m_scaleYSlider, 0.01, 10, 0.01, 1);
    addRow("Rotation:", m_rotSpin, m_rotSlider, -360, 360, 1, 0);
    addRow("Opacity:", m_opSpin, m_opSlider, 0, 1, 0.01, 1);
    addRow("Anchor X:", m_anchorXSpin, m_anchorXSlider, 0, 1, 0.01, 0.5);
    addRow("Anchor Y:", m_anchorYSpin, m_anchorYSlider, 0, 1, 0.01, 0.5);

    // Blending group
    auto* blendGroup = new QGroupBox("Blending", content);
    blendGroup->setStyleSheet(transformGroup->styleSheet());
    auto* blendLayout = new QVBoxLayout(blendGroup);
    blendLayout->setContentsMargins(8, 16, 8, 8);
    layout->addWidget(blendGroup);

    m_blendMode = new QComboBox(blendGroup);
    m_blendMode->addItems({"Normal", "Multiply", "Screen", "Overlay", "Add", "Subtract", "Difference"});
    m_blendMode->setStyleSheet("QComboBox { background: #2a2a2a; color: #ddd; border: 1px solid #444; padding: 4px; border-radius: 3px; } QComboBox::drop-down { border: none; }");
    connect(m_blendMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EffectsPanel::onBlendChanged);
    blendLayout->addWidget(new QLabel("Mode:", blendGroup));
    blendLayout->addWidget(m_blendMode);

    m_motionBlur = new QCheckBox("Motion Blur", blendGroup);
    m_motionBlur->setStyleSheet("QCheckBox { color: #ddd; spacing: 8px; } QCheckBox::indicator { width: 14px; height: 14px; }");
    connect(m_motionBlur, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_clipId.isEmpty()) {
            emit paramChanged(m_trackId, m_clipId, "motionBlur", checked ? 1.0 : 0.0);
        }
    });
    blendLayout->addWidget(m_motionBlur);

    // Color group
    auto* colorGroup = new QGroupBox("Color", content);
    colorGroup->setStyleSheet(transformGroup->styleSheet());
    auto* clLayout = new QFormLayout(colorGroup);
    clLayout->setContentsMargins(8, 16, 8, 8);
    layout->addWidget(colorGroup);

    auto addColorParam = [&](const QString& label, double min, double max, double step, double def) {
        QDoubleSpinBox* spin = new QDoubleSpinBox(colorGroup);
        spin->setRange(min, max);
        spin->setSingleStep(step);
        spin->setDecimals(2);
        spin->setValue(def);
        spin->setStyleSheet("QDoubleSpinBox { background: #2a2a2a; color: #ddd; border: 1px solid #444; padding: 4px; border-radius: 3px; }");
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, label=label](double v) {
            if (!m_clipId.isEmpty()) emit paramChanged(m_trackId, m_clipId, label.toLower(), v);
        });
        clLayout->addRow(label + ":", spin);
    };

    addColorParam("Exposure", -5, 5, 0.1, 0);
    addColorParam("Contrast", -1, 1, 0.01, 0);
    addColorParam("Saturation", -1, 2, 0.01, 0);
    addColorParam("Temperature", -1, 1, 0.01, 0);
    addColorParam("Tint", -1, 1, 0.01, 0);
    addColorParam("Highlights", -1, 1, 0.01, 0);
    addColorParam("Shadows", -1, 1, 0.01, 0);
    addColorParam("Vignette", 0, 1, 0.01, 0);

    layout->addStretch();
}

void EffectsPanel::updateValues() {
    if (!m_project || m_clipId.isEmpty()) { clearSelection(); return; }
    auto* clip = m_project->findClip(m_trackId, m_clipId);
    if (!clip) { clearSelection(); return; }

    setEnabled(true);
    m_clipNameLabel->setText(clip->name.isEmpty() ? "Clip" : clip->name);

    auto getVal = [&](const QString& n, double d) {
        return clip->params.contains(n) ? clip->params[n].value : d;
    };

    auto setPair = [&](QDoubleSpinBox* s, QSlider* sl, double v) {
        s->blockSignals(true); s->setValue(v); s->blockSignals(false);
        sl->blockSignals(true); 
        sl->setValue(static_cast<int>((v - s->minimum()) / (s->maximum() - s->minimum()) * 1000));
        sl->blockSignals(false);
    };

    setPair(m_posXSpin, m_posXSlider, getVal("position.x", 0));
    setPair(m_posYSpin, m_posYSlider, getVal("position.y", 0));
    setPair(m_scaleXSpin, m_scaleXSlider, getVal("scale.x", 1));
    setPair(m_scaleYSpin, m_scaleYSlider, getVal("scale.y", 1));
    setPair(m_rotSpin, m_rotSlider, getVal("rotation", 0));
    setPair(m_opSpin, m_opSlider, getVal("opacity", 1));
    setPair(m_anchorXSpin, m_anchorXSlider, getVal("anchor.x", 0.5));
    setPair(m_anchorYSpin, m_anchorYSlider, getVal("anchor.y", 0.5));

    m_blendMode->blockSignals(true);
    m_blendMode->setCurrentIndex(static_cast<int>(clip->blendMode));
    m_blendMode->blockSignals(false);

    m_motionBlur->blockSignals(true);
    m_motionBlur->setChecked(clip->params.contains("motionBlur") && clip->params["motionBlur"].value > 0.5);
    m_motionBlur->blockSignals(false);
}

void EffectsPanel::onSpinChanged() {
    auto* sb = qobject_cast<QDoubleSpinBox*>(sender());
    if (!sb || m_clipId.isEmpty()) return;
    QString n;
    if (sb == m_posXSpin) n = "position.x";
    else if (sb == m_posYSpin) n = "position.y";
    else if (sb == m_scaleXSpin) n = "scale.x";
    else if (sb == m_scaleYSpin) n = "scale.y";
    else if (sb == m_rotSpin) n = "rotation";
    else if (sb == m_opSpin) n = "opacity";
    else if (sb == m_anchorXSpin) n = "anchor.x";
    else if (sb == m_anchorYSpin) n = "anchor.y";
    else return;
    emit paramChanged(m_trackId, m_clipId, n, sb->value());
}

void EffectsPanel::onBlendChanged(int index) {
    if (!m_clipId.isEmpty())
        emit blendChanged(m_trackId, m_clipId, index);
}