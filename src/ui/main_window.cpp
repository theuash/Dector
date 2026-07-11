#include "main_window.h"
#include "media_browser.h"
#include "viewer_widget.h"
#include "timeline_widget.h"
#include "time_ruler.h"
#include "project.h"
#include "command.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    resize(1600, 900);
    setWindowTitle("VE - Untitled");

    m_project = new Project(this);
    m_commandStack = new CommandStack(this);

    setupLayout();
    setupMenuBar();

    connect(m_commandStack, &CommandStack::changed, this, [this]() { updateTitle(); });
    connect(m_project, &Project::modifiedChanged, this, &MainWindow::onModifiedChanged);

    updateTitle();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupLayout() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_verticalSplitter = new QSplitter(Qt::Horizontal, central);
    m_mediaBrowser = new MediaBrowser(m_project, this);
    m_viewer = new ViewerWidget(this);
    m_verticalSplitter->addWidget(m_mediaBrowser);
    m_verticalSplitter->addWidget(m_viewer);
    m_verticalSplitter->setStretchFactor(0, 1);
    m_verticalSplitter->setStretchFactor(1, 3);

    m_timeRuler = new TimeRuler(this);
    m_timeline = new TimelineWidget(m_project, this);

    m_mainSplitter = new QSplitter(Qt::Vertical, central);
    m_mainSplitter->addWidget(m_verticalSplitter);
    m_mainSplitter->addWidget(m_timeRuler);
    m_mainSplitter->addWidget(m_timeline);
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setStretchFactor(2, 1);

    layout->addWidget(m_mainSplitter);
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
}

void MainWindow::onNewProject() {
    delete m_project;
    m_project = new Project(this);
    m_commandStack->clear();
    m_mediaBrowser->setProject(m_project);
    m_timeline->setProject(m_project);
    m_currentFilePath.clear();
    updateTitle();
}

void MainWindow::onOpenProject() {
    QString path = QFileDialog::getOpenFileName(this, "Open Project", {}, "VE Project (*.veproj)");
    if (path.isEmpty()) return;

    auto* newProject = new Project(this);
    if (!newProject->load(path)) {
        QMessageBox::warning(this, "Error", "Failed to load project.");
        delete newProject;
        return;
    }

    delete m_project;
    m_project = newProject;
    m_commandStack->clear();
    m_mediaBrowser->setProject(m_project);
    m_timeline->setProject(m_project);
    m_currentFilePath = path;
    updateTitle();
}

void MainWindow::onSaveProject() {
    if (m_currentFilePath.isEmpty()) {
        onSaveAsProject();
        return;
    }
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

    for (const auto& path : paths)
        m_commandStack->push(std::make_unique<AddAssetCommand>(m_project, path));
}

void MainWindow::onUndo() { m_commandStack->undo(); }
void MainWindow::onRedo() { m_commandStack->redo(); }

void MainWindow::onModifiedChanged(bool) { updateTitle(); }

void MainWindow::updateTitle() {
    QString title = "VE";
    if (m_project) {
        title += " - " + m_project->name();
        if (m_project->isModified()) title += " *";
    }
    setWindowTitle(title);
}
