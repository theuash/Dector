#pragma once
#include <QObject>
#include <QString>
#include <memory>
#include <vector>
#include "rational_time.h"
#include "parameter.h"
#include "asset.h"
#include "clip.h"
#include "track.h"

class Project;

class Command : public QObject {
    Q_OBJECT
public:
    explicit Command(QString name = {}) : QObject(nullptr), m_name(std::move(name)) {}
    ~Command() override = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    QString name() const { return m_name; }
private:
    QString m_name;
};

class CommandStack : public QObject {
    Q_OBJECT
public:
    explicit CommandStack(QObject* parent = nullptr);

    void push(std::unique_ptr<Command> cmd);
    void undo();
    void redo();
    void clear();

    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }
    QString undoText() const;
    QString redoText() const;

    void setMaxUndo(size_t n) { m_maxUndo = n; }

signals:
    void changed();

private:
    std::vector<std::unique_ptr<Command>> m_undoStack;
    std::vector<std::unique_ptr<Command>> m_redoStack;
    size_t m_maxUndo = 100;
};

class AddAssetCommand : public Command {
public:
    AddAssetCommand(Project* project, const QString& path);
    void execute() override;
    void undo() override;
private:
    Project* m_project;
    QString m_path;
    QString m_assetId;
};

class RemoveAssetCommand : public Command {
public:
    RemoveAssetCommand(Project* project, const QString& assetId);
    void execute() override;
    void undo() override;
private:
    Project* m_project;
    QString m_assetId;
    std::unique_ptr<Asset> m_savedAsset;
};

class AddTrackCommand : public Command {
public:
    AddTrackCommand(Project* project, TrackType type);
    void execute() override;
    void undo() override;
private:
    Project* m_project;
    TrackType m_type;
    QString m_trackId;
};

class AddClipCommand : public Command {
public:
    AddClipCommand(Project* project, const QString& trackId, const Clip& clip);
    void execute() override;
    void undo() override;
private:
    Project* m_project;
    QString m_trackId;
    Clip m_clip;
};

class RemoveClipCommand : public Command {
public:
    RemoveClipCommand(Project* project, const QString& trackId, const QString& clipId);
    void execute() override;
    void undo() override;
private:
    Project* m_project;
    QString m_trackId;
    QString m_clipId;
    Clip m_savedClip;
};

class MoveClipCommand : public Command {
public:
    MoveClipCommand(Project* project, const QString& trackId, const QString& clipId,
                    const RationalTime& newOffset, int newTrackIndex = -1);
    void execute() override;
    void undo() override;
private:
    Project* m_project;
    QString m_trackId;
    QString m_clipId;
    RationalTime m_oldOffset;
    RationalTime m_newOffset;
    int m_oldTrackIndex;
    int m_newTrackIndex;
};

class SetParamCommand : public Command {
public:
    SetParamCommand(Project* project, const QString& trackId, const QString& clipId,
                    const QString& paramName, double newValue);
    void execute() override;
    void undo() override;
private:
    Project* m_project;
    QString m_trackId;
    QString m_clipId;
    QString m_paramName;
    double m_oldValue;
    double m_newValue;
};
