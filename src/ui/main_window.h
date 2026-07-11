#pragma once
#include <QMainWindow>
#include <QSplitter>
#include "rational_time.h"
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <memory>

class Project;
class CommandStack;
class MediaBrowser;
class ViewerWidget;
class TimelineWidget;
class TimeRuler;
class EffectsPanel;
class PlaybackEngine;

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
    void onPlayPause();
    void onStop();
    void onSeekSlider(int value);
    void onPlayheadMoved(const RationalTime& time);
    void onFrameChanged(const RationalTime& time);
    void onClipSelected(const QString& trackId, const QString& clipId);
    void onSelectionCleared();
    void onClipAddedToTimeline(const QString& assetId);
    void onParamChanged(const QString& trackId, const QString& clipId,
                        const QString& paramName, double value);
    void onBlendChanged(const QString& trackId, const QString& clipId, int mode);

private:
    void setupMenuBar();
    void setupLayout();
    void updateTitle();
    void updateSeekRange();

    Project* m_project = nullptr;
    CommandStack* m_commandStack = nullptr;
    PlaybackEngine* m_playback = nullptr;

    // Panels
    MediaBrowser* m_mediaBrowser = nullptr;
    ViewerWidget* m_viewer = nullptr;
    EffectsPanel* m_effectsPanel = nullptr;
    TimeRuler* m_timeRuler = nullptr;
    TimelineWidget* m_timeline = nullptr;

    // Viewer controls
    QPushButton* m_playBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
    QSlider* m_seekSlider = nullptr;
    QLabel* m_timeLabel = nullptr;

    // Toolbar actions
    QAction* m_undoAct = nullptr;
    QAction* m_redoAct = nullptr;
    QAction* m_playAct = nullptr;

    QString m_currentFilePath;
};
