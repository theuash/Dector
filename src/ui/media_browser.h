#pragma once
#include <QWidget>
#include <QListWidget>

class Project;

class MediaBrowser : public QWidget {
    Q_OBJECT
public:
    explicit MediaBrowser(Project* project, QWidget* parent = nullptr);
    void setProject(Project* project);

signals:
    void clipAddedToTimeline(const QString& assetId);
    void sourcePreviewRequested(const QString& assetId);

private:
    void refresh();
    Project* m_project = nullptr;
    QListWidget* m_list = nullptr;
};
