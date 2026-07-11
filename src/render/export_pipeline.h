#pragma once
#include <QObject>
#include <QString>

class Project;

class ExportPipeline : public QObject {
    Q_OBJECT
public:
    explicit ExportPipeline(QObject* parent = nullptr);

    void exportSequence(Project* project, const QString& outputPath,
                        int width = 1920, int height = 1080, double fps = 30.0);

signals:
    void progressChanged(double percent);
    void finished(bool success, const QString& message);
};
