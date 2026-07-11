#include "export_pipeline.h"
#include "project.h"
#include "media_decoder.h"
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QThread>
#include <QImage>
#include <QPainter>
#include <QTransform>
#include <memory>
#include <QPainter>
#include <QTransform>
#include <QByteArray>
#include <QHash>
#include <memory>

struct ExportWorker : public QObject {
    Q_OBJECT
public:
    explicit ExportWorker(Project* project, const QString& outputPath,
                          int width, int height, double fps)
        : m_project(project), m_outputPath(outputPath),
          m_width(width), m_height(height), m_fps(fps) {}

public slots:
    void run() {
        auto* seq = m_project ? m_project->currentSequence() : nullptr;
        if (!seq) { emit finished(false, "No sequence"); return; }

        RationalTime dur = seq->calculateDuration();
        int totalFrames = dur.toFrames(m_fps);
        if (totalFrames <= 0) { emit finished(false, "Empty sequence"); return; }

        // Prepare decoders per asset
        QHash<QString, std::shared_ptr<MediaDecoder>> decoders;
        for (const auto& track : seq->tracks) {
            for (const auto& clip : track.clips) {
                auto* asset = m_project->asset(clip.assetId);
                if (asset && !decoders.contains(asset->id)) {
                    auto dec = std::make_shared<MediaDecoder>(nullptr);
                    if (dec->open(asset->path)) {
                        decoders[asset->id] = dec;
                    }
                }
            }
        }

        // FFmpeg encode process
        QStringList args;
        args << "-y" << "-f" << "rawvideo" << "-pix_fmt" << "rgba"
             << "-s" << QString("%1x%2").arg(m_width).arg(m_height)
             << "-r" << QString::number(m_fps) << "-i" << "-"
             << "-c:v" << "libx264" << "-preset" << "medium"
             << "-crf" << "20" << "-pix_fmt" << "yuv420p"
             << m_outputPath;

        QProcess proc;
        proc.setProcessChannelMode(QProcess::ForwardedChannels);
        proc.start("ffmpeg", args);
        if (!proc.waitForStarted(3000)) {
            emit finished(false, "ffmpeg failed to start");
            return;
        }

        QImage frame(m_width, m_height, QImage::Format_RGBA8888);
        frame.fill(Qt::black);

        for (int f = 0; f < totalFrames; ++f) {
            RationalTime t(f, static_cast<int64_t>(m_fps));
            frame.fill(Qt::black);
            QPainter painter(&frame);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            // Render each track bottom to top
            for (const auto& track : seq->tracks) {
                if (!track.enabled) continue;
                for (const auto& clip : track.clips) {
                    if (t < clip.trackOffset || t >= clip.endTime()) continue;

                    auto it = decoders.find(clip.assetId);
                    if (it == decoders.end()) continue;
                    auto* dec = it->get();

                    RationalTime srcT = clip.sourceStart + (t - clip.trackOffset);
                    if (!dec->seek(srcT.toSeconds())) continue;
                    auto img = dec->readFrame();
                    if (!img || img->isNull()) continue;

                    // Transform
                    QTransform xform;
                    double px = 0, py = 0, sx = 1, sy = 1, rot = 0, op = 1;
                    if (clip.params.contains("position.x")) px = clip.params["position.x"].value;
                    if (clip.params.contains("position.y")) py = clip.params["position.y"].value;
                    if (clip.params.contains("scale.x")) sx = clip.params["scale.x"].value;
                    if (clip.params.contains("scale.y")) sy = clip.params["scale.y"].value;
                    if (clip.params.contains("rotation")) rot = clip.params["rotation"].value;
                    if (clip.params.contains("opacity")) op = clip.params["opacity"].value;

                    xform.translate(m_width/2.0 + px, m_height/2.0 + py);
                    xform.rotate(rot);
                    xform.scale(sx, sy);
                    xform.translate(-img->width()/2.0, -img->height()/2.0);

                    painter.setOpacity(op);
                    painter.setTransform(xform, true);
                    painter.drawImage(0, 0, *img);
                    painter.resetTransform();
                }
            }
            painter.end();

            // Write frame
            QByteArray data((const char*)frame.bits(), frame.sizeInBytes());
            proc.write(data);
            if (f % 30 == 0) {
                int pct = (f * 100) / totalFrames;
                emit progressChanged(pct);
            }
        }

        proc.closeWriteChannel();
        proc.waitForFinished(30000);
        emit finished(true, "Export complete");
    }

signals:
    void progressChanged(int pct);
    void finished(bool ok, const QString& msg);

private:
    Project* m_project;
    QString m_outputPath;
    int m_width, m_height;
    double m_fps;
};

ExportPipeline::ExportPipeline(QObject* parent) : QObject(parent) {}

void ExportPipeline::exportSequence(Project* project, const QString& outputPath,
                                     int width, int height, double fps) {
    if (!project || !project->currentSequence()) {
        emit finished(false, "No sequence");
        return;
    }

    auto* worker = new ExportWorker(project, outputPath, width, height, fps);
    auto* thread = new QThread(this);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &ExportWorker::run);
    connect(worker, &ExportWorker::progressChanged, this, &ExportPipeline::progressChanged);
    connect(worker, &ExportWorker::finished, this, [this, thread, worker](bool ok, const QString& msg) {
        emit finished(ok, msg);
        thread->quit();
        thread->wait();
        worker->deleteLater();
        thread->deleteLater();
    });

    thread->start();
}

#include "export_pipeline.moc"