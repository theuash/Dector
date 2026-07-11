#include "effects_panel.h"
#include "project.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

EffectsPanel::EffectsPanel(Project* project, QWidget* parent)
    : QWidget(parent), m_project(project) {
    setupUI();
}

void EffectsPanel::setProject(Project* project) {
    m_project = project;
    clearSelection();
}

void EffectsPanel::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    m_clipNameLabel = new QLabel("No clip selected", this);
    m_clipNameLabel->setStyleSheet("font-weight: bold; padding: 4px;");
    layout->addWidget(m_clipNameLabel);

    auto* transformGroup = new QGroupBox("Transform", this);
    auto* form = new QFormLayout(transformGroup);

    m_posX = new QDoubleSpinBox(this);
    m_posX->setRange(-10000, 10000);
    m_posX->setSuffix(" px");
    form->addRow("Position X:", m_posX);

    m_posY = new QDoubleSpinBox(this);
    m_posY->setRange(-10000, 10000);
    m_posY->setSuffix(" px");
    form->addRow("Position Y:", m_posY);

    m_scaleX = new QDoubleSpinBox(this);
    m_scaleX->setRange(0.01, 100);
    m_scaleX->setSingleStep(0.1);
    m_scaleX->setDecimals(2);
    form->addRow("Scale X:", m_scaleX);

    m_scaleY = new QDoubleSpinBox(this);
    m_scaleY->setRange(0.01, 100);
    m_scaleY->setSingleStep(0.1);
    m_scaleY->setDecimals(2);
    form->addRow("Scale Y:", m_scaleY);

    m_rotation = new QDoubleSpinBox(this);
    m_rotation->setRange(-360, 360);
    m_rotation->setSuffix(" \xC2\xB0");
    form->addRow("Rotation:", m_rotation);

    m_opacity = new QDoubleSpinBox(this);
    m_opacity->setRange(0, 1);
    m_opacity->setSingleStep(0.01);
    m_opacity->setDecimals(2);
    form->addRow("Opacity:", m_opacity);

    layout->addWidget(transformGroup);

    auto* blendGroup = new QGroupBox("Blending", this);
    auto* blendLayout = new QVBoxLayout(blendGroup);
    m_blendMode = new QComboBox(this);
    m_blendMode->addItems({"Normal", "Multiply", "Screen", "Overlay", "Add"});
    blendLayout->addWidget(m_blendMode);
    layout->addWidget(blendGroup);

    layout->addStretch();

    connect(m_posX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EffectsPanel::onSpinChanged);
    connect(m_posY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EffectsPanel::onSpinChanged);
    connect(m_scaleX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EffectsPanel::onSpinChanged);
    connect(m_scaleY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EffectsPanel::onSpinChanged);
    connect(m_rotation, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EffectsPanel::onSpinChanged);
    connect(m_opacity, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EffectsPanel::onSpinChanged);
    connect(m_blendMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EffectsPanel::onBlendChanged);

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

void EffectsPanel::updateValues() {
    if (!m_project || m_clipId.isEmpty()) { clearSelection(); return; }
    auto* clip = m_project->findClip(m_trackId, m_clipId);
    if (!clip) { clearSelection(); return; }

    setEnabled(true);
    m_clipNameLabel->setText(clip->name.isEmpty() ? "Clip" : clip->name);

    m_posX->blockSignals(true);
    m_posY->blockSignals(true);
    m_scaleX->blockSignals(true);
    m_scaleY->blockSignals(true);
    m_rotation->blockSignals(true);
    m_opacity->blockSignals(true);
    m_blendMode->blockSignals(true);

    auto g = [&](const QString& n, double d) {
        return clip->params.contains(n) ? clip->params[n].value : d;
    };
    m_posX->setValue(g("position.x", 0));
    m_posY->setValue(g("position.y", 0));
    m_scaleX->setValue(g("scale.x", 1));
    m_scaleY->setValue(g("scale.y", 1));
    m_rotation->setValue(g("rotation", 0));
    m_opacity->setValue(g("opacity", 1));
    m_blendMode->setCurrentIndex(static_cast<int>(clip->blendMode));

    m_posX->blockSignals(false);
    m_posY->blockSignals(false);
    m_scaleX->blockSignals(false);
    m_scaleY->blockSignals(false);
    m_rotation->blockSignals(false);
    m_opacity->blockSignals(false);
    m_blendMode->blockSignals(false);
}

void EffectsPanel::onSpinChanged() {
    auto* sb = qobject_cast<QDoubleSpinBox*>(sender());
    if (!sb || m_clipId.isEmpty()) return;
    QString n;
    if (sb == m_posX) n = "position.x";
    else if (sb == m_posY) n = "position.y";
    else if (sb == m_scaleX) n = "scale.x";
    else if (sb == m_scaleY) n = "scale.y";
    else if (sb == m_rotation) n = "rotation";
    else if (sb == m_opacity) n = "opacity";
    else return;
    emit paramChanged(m_trackId, m_clipId, n, sb->value());
}

void EffectsPanel::onBlendChanged(int index) {
    if (!m_clipId.isEmpty()) emit blendChanged(m_trackId, m_clipId, index);
}
