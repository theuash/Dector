#include "export_pipeline.h"
#include "project.h"
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QDebug>

ExportPipeline::ExportPipeline(QObject* parent) : QObject(parent) {}

void ExportPipeline::exportSequence(Project* project, const QString& outputPath,
                                     int width, int height, double fps) {
    auto* seq = project ? project->currentSequence() : nullptr;
    if (!seq) {
        emit finished(false, "No sequence to export");
        return;
    }

    QStringList files;
    for (const auto& track : seq->tracks) {
        for (const auto& clip : track.clips) {
            auto* asset = project->asset(clip.assetId);
            if (asset) files << asset->path;
        }
    }

    if (files.isEmpty()) {
        emit finished(false, "No clips in sequence");
        return;
    }

    QStringList args;
    args << "-y";

    QString concatFile;
    if (files.size() == 1) {
        args << "-i" << files[0];
    } else {
        concatFile = QDir::tempPath() + "/ve_concat.txt";
        QFile cf(concatFile);
        if (cf.open(QIODevice::WriteOnly)) {
            for (const auto& f : files)
                cf.write(QString("file '%1'\n").arg(f).toUtf8());
            cf.close();
        }
        args << "-f" << "concat" << "-safe" << "0" << "-i" << concatFile;
    }

    args << "-c:v" << "libx264" << "-preset" << "fast"
         << "-crf" << "22"
         << "-c:a" << "aac"
         << "-pix_fmt" << "yuv420p"
         << outputPath;

    // ponytail: trim/cut/effect export comes in week 4

    qDebug() << "FFmpeg:" << args;

    auto* proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc, concatFile](int code) {
        if (!concatFile.isEmpty()) QFile::remove(concatFile);
        emit finished(code == 0, code == 0 ? "Export complete" : "Export failed");
        proc->deleteLater();
    });
    proc->start("ffmpeg", args);
}
