#include "main_window.h"
#include "media_browser.h"
#include "viewer_widget.h"
#include "timeline_widget.h"
#include "time_ruler.h"
#include "effects_panel.h"
#include "project.h"
#include "command.h"
#include "playback_engine.h"
#include "media_decoder.h"
#include "export_pipeline.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QShortcut>
#include <QFrame>
#include <QButtonGroup>
#include <QSpinBox>
#include <QDoubleSpinBox>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    resize(1600, 900);
    setWindowTitle("VE - Untitled");

    m_project = new Project(this);
    m_commandStack = new CommandStack(this);
    m_playback = new PlaybackEngine(this);
    m_playback->setProject(m_project);

    setupLayout();
    setupMenuBar();

    auto* tb = addToolBar("Transport");
    tb->setMovable(false);
    tb->setIconSize(QSize(16, 16));

    m_playAct = tb->addAction("\xE2\x96\xB6 Play", this, &MainWindow::onPlayPause);
    m_playAct->setShortcut(QKeySequence(Qt::Key_Space));
    tb->addAction("\xE2\x96\xA0 Stop", this, &MainWindow::onStop);
    tb->addSeparator();

    m_undoAct = tb->addAction("\xE2\x86\xA9 Undo", this, &MainWindow::onUndo);
    m_undoAct->setShortcut(QKeySequence::Undo);
    m_redoAct = tb->addAction("\xE2\x86\xAA Redo", this, &MainWindow::onRedo);
    m_redoAct->setShortcut(QKeySequence::Redo);
    tb->addSeparator();

    tb->addAction("\xF0\x9F\x94\x8D Zoom In", this, [this]() {
        m_timeline->setPixelsPerSecond(m_timeline->pixelsPerSecond() * 1.3);
    })->setShortcut(QKeySequence::ZoomIn);

    tb->addAction("\xF0\x9F\x94\x8D Zoom Out", this, [this]() {
        m_timeline->setPixelsPerSecond(m_timeline->pixelsPerSecond() / 1.3);
    })->setShortcut(QKeySequence::ZoomOut);

    statusBar()->showMessage("Ready");

    // Keyboard shortcuts for tools
    auto* vShortcut = new QShortcut(QKeySequence(Qt::Key_V), this);
    connect(vShortcut, &QShortcut::activated, this, [this]() {
        if (m_toolGroup->button(0)) m_toolGroup->button(0)->click();
    });
    auto* cShortcut = new QShortcut(QKeySequence(Qt::Key_C), this);
    connect(cShortcut, &QShortcut::activated, this, [this]() {
        if (m_toolGroup->button(1)) m_toolGroup->button(1)->click();
    });

    connect(m_commandStack, &CommandStack::changed, this, [this]() { updateTitle(); });
    connect(m_project, &Project::modifiedChanged, this, [this](bool) { updateTitle(); });
    connect(m_project, &Project::sequenceChanged, this, [this]() { updateSeekRange(); m_timeline->update(); });
    connect(m_project, &Project::assetAdded, this, [this](const QString&) {
        statusBar()->showMessage("Media imported", 3000);
    });

    connect(m_timeline, &TimelineWidget::clipSelected, this, &MainWindow::onClipSelected);
    connect(m_timeline, &TimelineWidget::selectionCleared, this, &MainWindow::onSelectionCleared);
    connect(m_timeline, &TimelineWidget::playheadMoved, this, &MainWindow::onPlayheadMoved);
    connect(m_timeline, &TimelineWidget::zoomChanged, m_timeRuler, &TimeRuler::setPixelsPerSecond);
    connect(m_timeRuler, &TimeRuler::playheadClicked, this, &MainWindow::onPlayheadMoved);
    connect(m_mediaBrowser, &MediaBrowser::clipAddedToTimeline, this, &MainWindow::onClipAddedToTimeline);
    connect(m_mediaBrowser, &MediaBrowser::sourcePreviewRequested, this, &MainWindow::onSourcePreview);
    connect(m_effectsPanel, &EffectsPanel::paramChanged, this, &MainWindow::onParamChanged);
    connect(m_effectsPanel, &EffectsPanel::blendChanged, this, &MainWindow::onBlendChanged);
    connect(m_playback, &PlaybackEngine::frameChanged, this, &MainWindow::onFrameChanged);
    connect(m_playback, &PlaybackEngine::playbackStarted, this, [this]() {
        m_playAct->setText("\xE2\x96\xB6 Pause");
        m_sourceMode = false;
        m_viewerLabel->setText(" Program");
        statusBar()->showMessage("Playing");
    });
    connect(m_playback, &PlaybackEngine::playbackPaused, this, [this]() {
        m_playAct->setText("\xE2\x96\xB6 Play");
        statusBar()->showMessage("Paused");
    });
    connect(m_playback, &PlaybackEngine::playbackStopped, this, [this]() {
        m_playAct->setText("\xE2\x96\xB6 Play");
        m_seekSlider->setValue(0);
        statusBar()->showMessage("Stopped");
    });

    updateTitle();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupLayout() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* outer = new QVBoxLayout(central);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Top section: tool panel + panels
    auto* topWidget = new QWidget(this);
    auto* topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);

    // Tool panel
    auto* toolPanel = new QFrame(this);
    toolPanel->setFixedWidth(36);
    toolPanel->setStyleSheet("QFrame { background: #1e1e1e; border-right: 1px solid #333; }");
    auto* toolLayout = new QVBoxLayout(toolPanel);
    toolLayout->setContentsMargins(2, 4, 2, 4);
    toolLayout->setSpacing(2);

    m_toolGroup = new QButtonGroup(this);
    m_toolGroup->setExclusive(true);

    auto* selBtn = new QPushButton("\xE2\x96\xBC", this);
    selBtn->setCheckable(true);
    selBtn->setChecked(true);
    selBtn->setToolTip("Selection Tool (V)");
    selBtn->setFixedSize(30, 30);
    selBtn->setStyleSheet(
        "QPushButton { background: #333; border: 1px solid #555; color: #ccc; "
        "border-radius: 3px; font-size: 14px; }"
        "QPushButton:checked { background: #3a6090; border-color: #5a80b0; color: #fff; }"
        "QPushButton:hover { background: #444; }");
    m_toolGroup->addButton(selBtn, 0);
    toolLayout->addWidget(selBtn);

    auto* razorBtn = new QPushButton("\xE2\x9C\x82", this);
    razorBtn->setCheckable(true);
    razorBtn->setToolTip("Razor Tool (C)");
    razorBtn->setFixedSize(30, 30);
    razorBtn->setStyleSheet(selBtn->styleSheet());
    m_toolGroup->addButton(razorBtn, 1);
    toolLayout->addWidget(razorBtn);

    toolLayout->addStretch();
    topLayout->addWidget(toolPanel);

    connect(m_toolGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &MainWindow::onToolChanged);

    auto* hSplit = new QSplitter(Qt::Horizontal, central);

    m_mediaBrowser = new MediaBrowser(m_project, this);
    hSplit->addWidget(m_mediaBrowser);

    auto* vc = new QWidget(this);
    auto* vl = new QVBoxLayout(vc);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    m_viewerLabel = new QLabel(" Program", this);
    m_viewerLabel->setStyleSheet(
        "background: #1a1a1a; color: #999; font-size: 10px; padding: 4px; "
        "border-bottom: 1px solid #333;");
    vl->addWidget(m_viewerLabel);

    m_viewer = new ViewerWidget(this);
    m_viewer->setProject(m_project);
    vl->addWidget(m_viewer, 1);

    auto* cBar = new QWidget(this);
    cBar->setStyleSheet("background: #222; border-top: 1px solid #333;");
    cBar->setFixedHeight(36);
    auto* cl = new QHBoxLayout(cBar);
    cl->setContentsMargins(8, 2, 8, 2);
    cl->setSpacing(4);

    m_playBtn = new QPushButton("\xE2\x96\xB6", this);
    m_playBtn->setFixedSize(28, 28);
    m_playBtn->setToolTip("Play / Pause (Space)");
    m_playBtn->setStyleSheet(
        "QPushButton { background: #333; border: 1px solid #555; color: #ddd; "
        "border-radius: 3px; } QPushButton:hover { background: #444; }");
    connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);

    m_stopBtn = new QPushButton("\xE2\x96\xA0", this);
    m_stopBtn->setFixedSize(28, 28);
    m_stopBtn->setToolTip("Stop");
    m_stopBtn->setStyleSheet(m_playBtn->styleSheet());
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);

    m_seekSlider = new QSlider(Qt::Horizontal, this);
    m_seekSlider->setRange(0, 0);
    m_seekSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 4px; background: #444; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #888; width: 12px; height: 12px; "
        "margin: -4px 0; border-radius: 6px; }"
        "QSlider::handle:horizontal:hover { background: #aaa; }"
        "QSlider::sub-page:horizontal { background: #c85050; border-radius: 2px; }");
    connect(m_seekSlider, &QSlider::sliderMoved, this, &MainWindow::onSeekSlider);

    m_timeLabel = new QLabel("00:00:00:00", this);
    m_timeLabel->setStyleSheet("color: #aaa; font-family: monospace; font-size: 11px; padding: 0 4px;");
    m_timeLabel->setFixedWidth(100);

    cl->addWidget(m_playBtn);
    cl->addWidget(m_stopBtn);
    cl->addWidget(m_seekSlider, 1);
    cl->addWidget(m_timeLabel);
    vl->addWidget(cBar);

    hSplit->addWidget(vc);

    m_effectsPanel = new EffectsPanel(m_project, this);
    hSplit->addWidget(m_effectsPanel);
    hSplit->setStretchFactor(0, 1);
    hSplit->setStretchFactor(1, 3);
    hSplit->setStretchFactor(2, 1);
    topLayout->addWidget(hSplit, 1);

    auto* bw = new QWidget(this);
    auto* bl = new QVBoxLayout(bw);
    bl->setContentsMargins(0, 0, 0, 0);
    bl->setSpacing(0);
    m_timeRuler = new TimeRuler(this);
    m_timeline = new TimelineWidget(m_project, this);
    bl->addWidget(m_timeRuler);
    bl->addWidget(m_timeline, 1);

    auto* vSplit = new QSplitter(Qt::Vertical, central);
    vSplit->addWidget(topWidget);
    vSplit->addWidget(bw);
    vSplit->setStretchFactor(0, 3);
    vSplit->setStretchFactor(1, 1);

    outer->addWidget(vSplit);
}

void MainWindow::setupMenuBar() {
    auto* file = menuBar()->addMenu("&File");
    file->addAction("&New Project", QKeySequence::New, this, &MainWindow::onNewProject);
    file->addAction("&Open Project...", QKeySequence::Open, this, &MainWindow::onOpenProject);
    file->addSeparator();
    file->addAction("&Save", QKeySequence::Save, this, &MainWindow::onSaveProject);
    file->addAction("Save &As...", QKeySequence("Ctrl+Shift+S"), this, &MainWindow::onSaveAsProject);
    file->addSeparator();
    file->addAction("&Import Media...", QKeySequence("Ctrl+I"), this, &MainWindow::onImportMedia);
    file->addSeparator();
    file->addAction("&Export...", QKeySequence("Ctrl+E"), this, &MainWindow::onExport);
    file->addSeparator();
    file->addAction("&Quit", QKeySequence::Quit, this, &QWidget::close);

    auto* edit = menuBar()->addMenu("&Edit");
    edit->addAction("&Undo", QKeySequence::Undo, this, &MainWindow::onUndo);
    edit->addAction("&Redo", QKeySequence::Redo, this, &MainWindow::onRedo);

    auto* timeline = menuBar()->addMenu("&Timeline");
    timeline->addAction("Add &Video Track", QKeySequence(), this, [this]() {
        m_commandStack->push(std::make_unique<AddTrackCommand>(m_project, TrackType::Video));
    });
    timeline->addAction("Add &Audio Track", QKeySequence(), this, [this]() {
        m_commandStack->push(std::make_unique<AddTrackCommand>(m_project, TrackType::Audio));
    });

    auto* view = menuBar()->addMenu("&View");
    view->addAction("Reset Layout", QKeySequence(), this, [this]() {
        m_mainSplitter->setSizes({300, 1000, 250});
        m_verticalSplitter->setSizes({700, 300});
    });
}

void MainWindow::onNewProject() {
    delete m_project;
    m_project = new Project(this);
    m_commandStack->clear();
    m_playback->setProject(m_project);
    m_mediaBrowser->setProject(m_project);
    m_timeline->setProject(m_project);
    m_effectsPanel->setProject(m_project);
    m_viewer->setProject(m_project);
    m_decoder.reset();
    m_decoderPath.clear();
    m_currentFilePath.clear();
    updateTitle();
}

void MainWindow::onOpenProject() {
    QString path = QFileDialog::getOpenFileName(this, "Open Project", {}, "VE Project (*.veproj)");
    if (path.isEmpty()) return;
    auto* np = new Project(this);
    if (!np->load(path)) { QMessageBox::warning(this, "Error", "Failed to load."); delete np; return; }
    delete m_project;
    m_project = np;
    m_commandStack->clear();
    m_playback->setProject(m_project);
    m_mediaBrowser->setProject(m_project);
    m_timeline->setProject(m_project);
    m_effectsPanel->setProject(m_project);
    m_viewer->setProject(m_project);
    m_decoder.reset();
    m_decoderPath.clear();
    m_currentFilePath = path;
    updateSeekRange();
    updateTitle();
}

void MainWindow::onSaveProject() {
    if (m_currentFilePath.isEmpty()) { onSaveAsProject(); return; }
    if (!m_project->save(m_currentFilePath))
        QMessageBox::warning(this, "Error", "Failed to save.");
}

void MainWindow::onSaveAsProject() {
    QString path = QFileDialog::getSaveFileName(this, "Save Project", {}, "VE Project (*.veproj)");
    if (path.isEmpty()) return;
    m_currentFilePath = path;
    onSaveProject();
}

void MainWindow::onImportMedia() {
    QStringList paths = QFileDialog::getOpenFileNames(this, "Import Media", {},
        "Video (*.mp4 *.avi *.mov *.mkv *.webm);;Audio (*.mp3 *.wav *.flac *.aac);;All Files (*)");
    if (paths.isEmpty()) return;
    for (const auto& p : paths) {
        m_commandStack->push(std::make_unique<AddAssetCommand>(m_project, p));
        // Update duration after command executes
        MediaDecoder decoder;
        if (decoder.open(p)) {
            double dur = decoder.duration();
            if (dur > 0) {
                // Find asset by path and update
                const auto& assets = m_project->assets();
                for (const auto& a : assets) {
                    if (a->path == p) {
                        a->duration = RationalTime::fromSeconds(dur, 30.0);
                        break;
                    }
                }
            }
        }
    }
}

void MainWindow::onUndo() { m_commandStack->undo(); }
void MainWindow::onRedo() { m_commandStack->redo(); }

void MainWindow::onPlayPause() {
    if (m_playback->isPlaying()) m_playback->pause();
    else m_playback->play();
}

void MainWindow::onStop() { m_playback->stop(); }

void MainWindow::onSeekSlider(int value) {
    RationalTime t(value, 30);
    m_playback->seek(t);
    m_timeRuler->setPlayheadTime(t);
    m_timeline->setPlayheadTime(t);
    m_viewer->setCurrentTime(t);

    // Decode frame at this position
    if (!m_sourceMode) {
        auto* seq = m_project->currentSequence();
        if (seq) {
            for (const auto& track : seq->tracks) {
                for (const auto& clip : track.clips) {
                    if (t >= clip.trackOffset && t < clip.endTime()) {
                        auto* asset = m_project->asset(clip.assetId);
                        if (asset) {
                            double srcSec = clip.sourceStart.toSeconds()
                                + (t - clip.trackOffset).toSeconds();
                            decodeFrameAt(asset->path, srcSec);
                        }
                        return;
                    }
                }
            }
        }
    }
}

void MainWindow::onPlayheadMoved(const RationalTime& time) {
    m_playback->seek(time);
    m_timeRuler->setPlayheadTime(time);
    m_timeline->setPlayheadTime(time);
    m_viewer->setCurrentTime(time);
    m_sourceMode = false;
    m_viewerLabel->setText(" Program");
    updateSeekRange();

    auto* seq = m_project->currentSequence();
    if (seq) {
        for (const auto& track : seq->tracks) {
            for (const auto& clip : track.clips) {
                if (time >= clip.trackOffset && time < clip.endTime()) {
                    auto* asset = m_project->asset(clip.assetId);
                    if (asset) {
                        double srcSec = clip.sourceStart.toSeconds()
                            + (time - clip.trackOffset).toSeconds();
                        decodeFrameAt(asset->path, srcSec);
                    }
                    return;
                }
            }
        }
    }
}

void MainWindow::onFrameChanged(const RationalTime& time) {
    m_timeRuler->setPlayheadTime(time);
    m_timeline->setPlayheadTime(time);
    m_viewer->setCurrentTime(time);

    int frame = time.toFrames(30);
    int maxF = m_seekSlider->maximum();
    m_seekSlider->setValue(std::min(frame, maxF));

    double sec = time.toSeconds();
    int hh = static_cast<int>(sec) / 3600;
    int mm = (static_cast<int>(sec) % 3600) / 60;
    int ss = static_cast<int>(sec) % 60;
    int ff = frame % 30;
    m_timeLabel->setText(QString("%1:%2:%3:%4")
        .arg(hh, 2, 10, QChar('0'))
        .arg(mm, 2, 10, QChar('0'))
        .arg(ss, 2, 10, QChar('0'))
        .arg(ff, 2, 10, QChar('0')));

    // Decode frame for timeline playback
    if (!m_sourceMode) {
        auto* seq = m_project->currentSequence();
        if (seq) {
            for (const auto& track : seq->tracks) {
                for (const auto& clip : track.clips) {
                    if (time >= clip.trackOffset && time < clip.endTime()) {
                        auto* asset = m_project->asset(clip.assetId);
                        if (asset) {
                            double srcSec = clip.sourceStart.toSeconds()
                                + (time - clip.trackOffset).toSeconds();
                            decodeFrameAt(asset->path, srcSec);
                        }
                        return;
                    }
                }
            }
        }
    }
}

void MainWindow::onClipSelected(const QString& trackId, const QString& clipId) {
    m_effectsPanel->showClip(trackId, clipId);

    // Show first frame of selected clip in viewer
    auto* clip = m_project->findClip(trackId, clipId);
    if (!clip) return;
    auto* asset = m_project->asset(clip->assetId);
    if (!asset) return;

    m_sourceMode = false;
    m_viewerLabel->setText(" Program - " + clip->name);
    double srcSec = clip->sourceStart.toSeconds();
    decodeFrameAt(asset->path, srcSec);
    m_playback->seek(clip->trackOffset);
}

void MainWindow::onSelectionCleared() {
    m_effectsPanel->clearSelection();
}

void MainWindow::onClipAddedToTimeline(const QString& assetId) {
    auto* asset = m_project->asset(assetId);
    if (!asset) return;
    auto* seq = m_project->currentSequence();
    if (!seq || seq->tracks.empty()) return;

    Clip clip;
    clip.assetId = assetId;
    clip.name = asset->name;
    clip.sourceDuration = RationalTime(300, 30); // 10s default
    clip.sourceStart = RationalTime(0, 30);

    // Place at playhead
    clip.trackOffset = m_playback->currentTime();

    m_commandStack->push(std::make_unique<AddClipCommand>(m_project, seq->tracks[0].id, clip));
}

void MainWindow::onSourcePreview(const QString& assetId) {
    auto* asset = m_project->asset(assetId);
    if (!asset) return;

    m_sourceMode = true;
    m_viewerLabel->setText(" Source - " + asset->name);
    decodeFrameAt(asset->path, 0.0);
}

void MainWindow::onParamChanged(const QString& trackId, const QString& clipId,
                                 const QString& paramName, double value) {
    m_commandStack->push(
        std::make_unique<SetParamCommand>(m_project, trackId, clipId, paramName, value));
}

void MainWindow::onBlendChanged(const QString& trackId, const QString& clipId, int mode) {
    auto* clip = m_project->findClip(trackId, clipId);
    if (!clip) return;
    clip->blendMode = static_cast<BlendMode>(mode);
    m_project->notifyChanged();
}

void MainWindow::onToolChanged(int id) {
    switch (id) {
    case 0: m_timeline->setTool(TimelineTool::Selection); break;
    case 1: m_timeline->setTool(TimelineTool::Razor); break;
    }
    statusBar()->showMessage(
        id == 0 ? "Selection Tool (V)" : "Razor Tool (C)", 2000);
}

void MainWindow::updateSeekRange() {
    if (!m_project || !m_project->currentSequence()) return;
    auto dur = m_project->currentSequence()->calculateDuration();
    int frames = dur.toFrames(30);
    m_seekSlider->setRange(0, std::max(0, frames));
    m_viewer->setDuration(dur);
}

void MainWindow::updateTitle() {
    QString t = "VE";
    if (m_project) {
        t += " - " + m_project->name();
        if (m_project->isModified()) t += " *";
    }
    setWindowTitle(t);
}

bool MainWindow::openDecoderFor(const QString& path) {
    if (m_decoder && m_decoderPath == path && m_decoder->isOpen())
        return true;
    m_decoder = std::make_unique<MediaDecoder>(this);
    if (!m_decoder->open(path)) {
        m_decoderPath.clear();
        return false;
    }
    m_decoderPath = path;
    m_viewer->setFrameSize(m_decoder->width(), m_decoder->height());
    return true;
}

void MainWindow::decodeFrameAt(const QString& path, double seconds) {
    if (!openDecoderFor(path)) return;
    m_decoder->seek(seconds);
    auto frame = m_decoder->readFrame();
    if (frame) {
        m_viewer->setFrame(*frame);
    } else {
        // Retry with consume loop
        m_decoder->seek(seconds);
        for (int i = 0; i < 200; i++) {
            frame = m_decoder->readFrame();
            if (frame) { m_viewer->setFrame(*frame); return; }
        }
    }
}

void MainWindow::onExport() {
    if (!m_project || !m_project->currentSequence()) {
        QMessageBox::warning(this, "Export", "No sequence to export");
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, "Export Video", {},
        "MP4 (*.mp4);;MOV (*.mov);;AVI (*.avi);;All Files (*)");
    if (path.isEmpty()) return;

    // Simple export dialog
    QDialog dlg(this);
    dlg.setWindowTitle("Export Settings");
    dlg.setMinimumWidth(350);
    auto* layout = new QVBoxLayout(&dlg);

    auto* form = new QFormLayout();
    auto* widthSpin = new QSpinBox(&dlg);
    widthSpin->setRange(160, 7680);
    widthSpin->setValue(1920);
    form->addRow("Width:", widthSpin);

    auto* heightSpin = new QSpinBox(&dlg);
    heightSpin->setRange(120, 4320);
    heightSpin->setValue(1080);
    form->addRow("Height:", heightSpin);

    auto* fpsSpin = new QDoubleSpinBox(&dlg);
    fpsSpin->setRange(1, 120);
    fpsSpin->setValue(30);
    form->addRow("FPS:", fpsSpin);

    auto* crfSpin = new QSpinBox(&dlg);
    crfSpin->setRange(0, 51);
    crfSpin->setValue(22);
    form->addRow("Quality (CRF):", crfSpin);

    auto* presetCombo = new QComboBox(&dlg);
    presetCombo->addItems({"ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow"});
    presetCombo->setCurrentText("medium");
    form->addRow("Preset:", presetCombo);

    layout->addLayout(form);

    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(btnBox);

    if (dlg.exec() != QDialog::Accepted) return;

    m_exportPipeline = std::make_unique<ExportPipeline>(this);
    connect(m_exportPipeline.get(), &ExportPipeline::progressChanged, this, [this](double p) {
        statusBar()->showMessage(QString("Exporting: %1%").arg(p, 0, 'f', 1));
    });
    connect(m_exportPipeline.get(), &ExportPipeline::finished, this, [this](bool ok, const QString& msg) {
        statusBar()->showMessage(ok ? "Export complete" : ("Export failed: " + msg), 5000);
        QMessageBox::information(this, "Export", ok ? "Export completed successfully!" : msg);
    });

    m_exportPipeline->exportSequence(m_project, path,
        widthSpin->value(), heightSpin->value(), fpsSpin->value());
}
