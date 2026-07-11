#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <memory>

class Project;
class CommandStack;
class MediaBrowser;
class ViewerWidget;
class TimelineWidget;
class TimeRuler;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveAsProject();
    void onImportMedia();
    void onUndo();
    void onRedo();
    void onModifiedChanged(bool modified);

private:
    void setupMenuBar();
    void setupLayout();
    void updateTitle();

    Project* m_project = nullptr;
    CommandStack* m_commandStack = nullptr;

    QSplitter* m_mainSplitter = nullptr;
    QSplitter* m_verticalSplitter = nullptr;
    MediaBrowser* m_mediaBrowser = nullptr;
    ViewerWidget* m_viewer = nullptr;
    TimeRuler* m_timeRuler = nullptr;
    TimelineWidget* m_timeline = nullptr;

    QString m_currentFilePath;
};
