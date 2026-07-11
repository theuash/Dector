#include "main_window.h"
#include "media_browser.h"
#include "viewer_widget.h"
#include "timeline_widget.h"
#include "time_ruler.h"
#include "effects_panel.h"
#include "project.h"
#include "command.h"
#include "playback_engine.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    resize(1600, 900);
    setWindowTitle("VE - Untitled");

    m_project = new Project(this);
    m_commandStack = new CommandStack(this);
    m_playback = new PlaybackEngine(this);
    m_playback->setProject(m_project);

    setupLayout();
    setupMenuBar();

    // Toolbar
    auto* toolbar = addToolBar("Transport");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));

    m_playAct = toolbar->addAction("\xE2\x96\xB6 Play", this, &MainWindow::onPlayPause);
    m_playAct->setShortcut(QKeySequence(Qt::Key_Space));
    toolbar->addAction("\xE2\x96\xA0 Stop", this, &MainWindow::onStop);
    toolbar->addSeparator();

    m_undoAct = toolbar->addAction("\xE2\x86\xA9 Undo", this, &MainWindow::onUndo);
    m_undoAct->setShortcut(QKeySequence::Undo);
    m_redoAct = toolbar->addAction("\xE2\x86\xAA Redo", this, &MainWindow::onRedo);
    m_redoAct->setShortcut(QKeySequence::Redo);
    toolbar->addSeparator();

    auto* zoomIn = toolbar->addAction("\xF0\x9F\x94\x8D Zoom In", this, [this]() {
        m_timeline->setPixelsPerSecond(m_timeline->pixelsPerSecond() * 1.3);
    });
    zoomIn->setShortcut(QKeySequence::ZoomIn);
    auto* zoomOut = toolbar->addAction("\xF0\x9F\x94\x8D Zoom Out", this, [this]() {
        m_timeline->setPixelsPerSecond(m_timeline->pixelsPerSecond() / 1.3);
    });
    zoomOut->setShortcut(QKeySequence::ZoomOut);

    // Status bar
    statusBar()->showMessage("Ready");

    // Connect signals
    connect(m_commandStack, &CommandStack::changed, this, [this]() { updateTitle(); });
    connect(m_project, &Project::modifiedChanged, this, [this](bool) { updateTitle(); });
    connect(m_project, &Project::sequenceChanged, this, [this]() {
        updateSeekRange();
        m_timeline->update();
    });
    connect(m_project, &Project::assetAdded, this, [this](const QString&) {
        statusBar()->showMessage("Media imported", 3000);
    });

    // Timeline signals
    connect(m_timeline, &TimelineWidget::clipSelected, this, &MainWindow::onClipSelected);
    connect(m_timeline, &TimelineWidget::selectionCleared, this, &MainWindow::onSelectionCleared);
    connect(m_timeline, &TimelineWidget::playheadMoved, this, &MainWindow::onPlayheadMoved);
    connect(m_timeline, &TimelineWidget::zoomChanged, m_timeRuler, &TimeRuler::setPixelsPerSecond);

    // Time ruler signals
    connect(m_timeRuler, &TimeRuler::playheadClicked, this, &MainWindow::onPlayheadMoved);

    // Media browser signals
    connect(m_mediaBrowser, &MediaBrowser::clipAddedToTimeline, this, &MainWindow::onClipAddedToTimeline);

    // Effect panel signals
    connect(m_effectsPanel, &EffectsPanel::paramChanged, this, &MainWindow::onParamChanged);
    connect(m_effectsPanel, &EffectsPanel::blendChanged, this, &MainWindow::onBlendChanged);

    // Playback signals
    connect(m_playback, &PlaybackEngine::frameChanged, this, &MainWindow::onFrameChanged);
    connect(m_playback, &PlaybackEngine::playbackStarted, this, [this]() {
        m_playAct->setText("\xE2\x96\xB6 Pause");
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
    auto* outerLayout = new QVBoxLayout(central);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // Horizontal: MediaBrowser | Viewer+Controls | EffectsPanel
    auto* hSplit = new QSplitter(Qt::Horizontal, central);

    m_mediaBrowser = new MediaBrowser(m_project, this);
    hSplit->addWidget(m_mediaBrowser);

    // Viewer + controls container
    auto* viewerContainer = new QWidget(this);
    auto* vLayout = new QVBoxLayout(viewerContainer);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(0);

    // Header label
    auto* viewerHeader = new QLabel(" Program", this);
    viewerHeader->setStyleSheet("background: #1a1a1a; color: #999; font-size: 10px; padding: 4px; border-bottom: 1px solid #333;");
    vLayout->addWidget(viewerHeader);

    m_viewer = new ViewerWidget(this);
    m_viewer->setProject(m_project);
    vLayout->addWidget(m_viewer, 1);

    // Controls bar
    auto* controlsBar = new QWidget(this);
    controlsBar->setStyleSheet("background: #222; border-top: 1px solid #333;");
    controlsBar->setFixedHeight(36);
    auto* cLayout = new QHBoxLayout(controlsBar);
    cLayout->setContentsMargins(8, 2, 8, 2);
    cLayout->setSpacing(4);

    m_playBtn = new QPushButton("\xE2\x96\xB6", this);
    m_playBtn->setFixedSize(28, 28);
    m_playBtn->setToolTip("Play / Pause (Space)");
    m_playBtn->setStyleSheet("QPushButton { background: #333; border: 1px solid #555; color: #ddd; border-radius: 3px; } QPushButton:hover { background: #444; }");
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
        "QSlider::handle:horizontal { background: #888; width: 12px; height: 12px; margin: -4px 0; border-radius: 6px; }"
        "QSlider::handle:horizontal:hover { background: #aaa; }"
        "QSlider::sub-page:horizontal { background: #4a80c0; border-radius: 2px; }");
    connect(m_seekSlider, &QSlider::sliderMoved, this, &MainWindow::onSeekSlider);

    m_timeLabel = new QLabel("00:00:00:00", this);
    m_timeLabel->setStyleSheet("color: #aaa; font-family: monospace; font-size: 11px; padding: 0 4px;");
    m_timeLabel->setFixedWidth(100);

    cLayout->addWidget(m_playBtn);
    cLayout->addWidget(m_stopBtn);
    cLayout->addWidget(m_seekSlider, 1);
    cLayout->addWidget(m_timeLabel);
    vLayout->addWidget(controlsBar);

    hSplit->addWidget(viewerContainer);

    m_effectsPanel = new EffectsPanel(m_project, this);
    hSplit->addWidget(m_effectsPanel);

    hSplit->setStretchFactor(0, 1);
    hSplit->setStretchFactor(1, 3);
    hSplit->setStretchFactor(2, 1);

    // Bottom: ruler + timeline
    auto* bottomWidget = new QWidget(this);
    auto* bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);

    m_timeRuler = new TimeRuler(this);
    m_timeline = new TimelineWidget(m_project, this);

    bottomLayout->addWidget(m_timeRuler);
    bottomLayout->addWidget(m_timeline, 1);

    // Vertical split: (Media|Viewer|Effects) | (Ruler|Timeline)
    auto* vSplit = new QSplitter(Qt::Vertical, central);
    vSplit->addWidget(hSplit);
    vSplit->addWidget(bottomWidget);
    vSplit->setStretchFactor(0, 3);
    vSplit->setStretchFactor(1, 1);

    outerLayout->addWidget(vSplit);
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
    m_currentFilePath.clear();
    updateTitle();
}

void MainWindow::onOpenProject() {
    QString path = QFileDialog::getOpenFileName(this, "Open Project", {}, "VE Project (*.veproj)");
    if (path.isEmpty()) return;
    auto* np = new Project(this);
    if (!np->load(path)) {
        QMessageBox::warning(this, "Error", "Failed to load project.");
        delete np; return;
    }
    delete m_project;
    m_project = np;
    m_commandStack->clear();
    m_playback->setProject(m_project);
    m_mediaBrowser->setProject(m_project);
    m_timeline->setProject(m_project);
    m_effectsPanel->setProject(m_project);
    m_viewer->setProject(m_project);
    m_currentFilePath = path;
    updateSeekRange();
    updateTitle();
}

void MainWindow::onSaveProject() {
    if (m_currentFilePath.isEmpty()) { onSaveAsProject(); return; }
    if (!m_project->save(m_currentFilePath))
        QMessageBox::warning(this, "Error", "Failed to save project.");
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
    for (const auto& p : paths)
        m_commandStack->push(std::make_unique<AddAssetCommand>(m_project, p));
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
}

void MainWindow::onPlayheadMoved(const RationalTime& time) {
    m_playback->seek(time);
    m_timeRuler->setPlayheadTime(time);
    m_timeline->setPlayheadTime(time);
    m_viewer->setCurrentTime(time);
    updateSeekRange();
}

void MainWindow::onFrameChanged(const RationalTime& time) {
    m_timeRuler->setPlayheadTime(time);
    m_timeline->setPlayheadTime(time);
    m_viewer->setCurrentTime(time);

    int frame = time.toFrames(30);
    if (frame >= 0 && frame <= m_seekSlider->maximum())
        m_seekSlider->setValue(frame);

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
}

void MainWindow::onClipSelected(const QString& trackId, const QString& clipId) {
    m_effectsPanel->showClip(trackId, clipId);
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
    clip.sourceDuration = RationalTime(300, 30); // 10 seconds default
    // ponytail: real duration from decoder in week 2

    m_commandStack->push(std::make_unique<AddClipCommand>(m_project, seq->tracks[0].id, clip));
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

void MainWindow::updateSeekRange() {
    if (!m_project || !m_project->currentSequence()) return;
    auto dur = m_project->currentSequence()->calculateDuration();
    int frames = dur.toFrames(30);
    m_seekSlider->setRange(0, std::max(0, frames));
    m_viewer->setDuration(dur);
}

void MainWindow::updateTitle() {
    QString title = "VE";
    if (m_project) {
        title += " - " + m_project->name();
        if (m_project->isModified()) title += " *";
    }
    setWindowTitle(title);
}
