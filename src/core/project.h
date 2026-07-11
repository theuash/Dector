#pragma once
#include "sequence.h"
#include "asset.h"
#include <QObject>
#include <QString>
#include <vector>
#include <memory>

class Project : public QObject {
    Q_OBJECT
public:
    explicit Project(QObject* parent = nullptr);

    Asset* addAsset(std::unique_ptr<Asset> asset);
    std::unique_ptr<Asset> takeAsset(const QString& id);
    Asset* asset(const QString& id) const;
    void removeAsset(const QString& id);
    const std::vector<std::unique_ptr<Asset>>& assets() const { return m_assets; }

    QString addTrack(TrackType type);
    void removeTrack(const QString& id);

    void addClipToTrack(const QString& trackId, const Clip& clip);
    std::unique_ptr<Clip> takeClipFromTrack(const QString& trackId, const QString& clipId);
    void removeClipFromTrack(const QString& trackId, const QString& clipId);
    Clip* findClip(const QString& trackId, const QString& clipId) const;
    void moveClip(const QString& trackId, const QString& clipId, const RationalTime& newOffset, int newTrackIndex = -1);
    void setClipParam(const QString& trackId, const QString& clipId, const QString& paramName, double value);

    void setCurrentSequence(const QString& id);
    Sequence* currentSequence() const;
    Sequence* sequence(const QString& id) const;
    QString addSequence(const QString& name = QString());

    bool save(const QString& path);
    bool load(const QString& path);
    bool isModified() const { return m_modified; }
    QString filePath() const { return m_path; }
    QString name() const { return m_name; }
    void setName(const QString& n);

signals:
    void assetAdded(const QString& id);
    void assetRemoved(const QString& id);
    void sequenceChanged();
    void currentSequenceChanged(const QString& id);
    void modifiedChanged(bool modified);

private:
    QString m_name = "Untitled";
    QString m_path;
    bool m_modified = false;
    std::vector<std::unique_ptr<Asset>> m_assets;
    std::vector<std::unique_ptr<Sequence>> m_sequences;
    QString m_currentSequenceId;

    Track* findTrack(const QString& id) const;
    void setModified(bool m);
};
