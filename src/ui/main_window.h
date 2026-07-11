#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QFrame>
#include <QButtonGroup>
#include <memory>
#include "rational_time.h"

class Project;
class CommandStack;
class MediaBrowser;
class ViewerWidget;
class TimelineWidget;
class TimeRuler;
class EffectsPanel;
class PlaybackEngine;
class ExportPipeline;
class MediaDecoder;
class QButtonGroup;

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
    void onSourcePreview(const QString& assetId);
    void onParamChanged(const QString& trackId, const QString& clipId,
                        const QString& paramName, double value);
    void onBlendChanged(const QString& trackId, const QString& clipId, int mode);
    void onToolChanged(int id);
    void onExport();

private:
    void setupMenuBar();
    void setupLayout();
    void updateTitle();
    void updateSeekRange();
    void decodeFrameAt(const QString& path, double seconds);
    bool openDecoderFor(const QString& path);

    Project* m_project = nullptr;
    CommandStack* m_commandStack = nullptr;
    PlaybackEngine* m_playback = nullptr;

    MediaBrowser* m_mediaBrowser = nullptr;
    ViewerWidget* m_viewer = nullptr;
    EffectsPanel* m_effectsPanel = nullptr;
    TimeRuler* m_timeRuler = nullptr;
    TimelineWidget* m_timeline = nullptr;

    QPushButton* m_playBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
    QSlider* m_seekSlider = nullptr;
    QLabel* m_timeLabel = nullptr;
    QLabel* m_viewerLabel = nullptr;

    QAction* m_undoAct = nullptr;
    QAction* m_redoAct = nullptr;
    QAction* m_playAct = nullptr;

    std::unique_ptr<MediaDecoder> m_decoder;
    QButtonGroup* m_toolGroup = nullptr;
    QString m_decoderPath;
    bool m_sourceMode = false;
    std::unique_ptr<ExportPipeline> m_exportPipeline;

    QString m_currentFilePath;
};
