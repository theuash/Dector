#include "command.h"
#include "project.h"

CommandStack::CommandStack(QObject* parent) : QObject(parent) {}

void CommandStack::push(std::unique_ptr<Command> cmd) {
    cmd->execute();
    m_undoStack.push_back(std::move(cmd));
    m_redoStack.clear();
    if (m_undoStack.size() > m_maxUndo) {
        m_undoStack.erase(m_undoStack.begin());
    }
    emit changed();
}

void CommandStack::undo() {
    if (m_undoStack.empty()) return;
    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    cmd->undo();
    m_redoStack.push_back(std::move(cmd));
    emit changed();
}

void CommandStack::redo() {
    if (m_redoStack.empty()) return;
    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    cmd->execute();
    m_undoStack.push_back(std::move(cmd));
    emit changed();
}

void CommandStack::clear() {
    m_undoStack.clear();
    m_redoStack.clear();
    emit changed();
}

QString CommandStack::undoText() const {
    return m_undoStack.empty() ? "" : m_undoStack.back()->name();
}

QString CommandStack::redoText() const {
    return m_redoStack.empty() ? "" : m_redoStack.back()->name();
}

#include "project.h"

AddAssetCommand::AddAssetCommand(Project* project, const QString& path)
    : Command("Import Media"), m_project(project), m_path(path) {}

void AddAssetCommand::execute() {
    auto asset = std::make_unique<Asset>();
    asset->path = m_path;
    asset->name = m_path.section('/', -1);
    asset->duration = RationalTime(300, 30);
    m_assetId = asset->id;
    m_project->addAsset(std::move(asset));
}

void AddAssetCommand::undo() {
    m_project->removeAsset(m_assetId);
}

RemoveAssetCommand::RemoveAssetCommand(Project* project, const QString& assetId)
    : Command("Remove Media"), m_project(project), m_assetId(assetId) {}

void RemoveAssetCommand::execute() {
    m_savedAsset = m_project->takeAsset(m_assetId);
}

void RemoveAssetCommand::undo() {
    m_project->addAsset(std::move(m_savedAsset));
}

AddTrackCommand::AddTrackCommand(Project* project, TrackType type)
    : Command("Add Track"), m_project(project), m_type(type) {}

void AddTrackCommand::execute() {
    m_trackId = m_project->addTrack(m_type);
}

void AddTrackCommand::undo() {
    m_project->removeTrack(m_trackId);
}

AddClipCommand::AddClipCommand(Project* project, const QString& trackId, const Clip& clip)
    : Command("Add Clip"), m_project(project), m_trackId(trackId), m_clip(clip) {}

void AddClipCommand::execute() {
    m_project->addClipToTrack(m_trackId, m_clip);
}

void AddClipCommand::undo() {
    m_project->removeClipFromTrack(m_trackId, m_clip.id);
}

RemoveClipCommand::RemoveClipCommand(Project* project, const QString& trackId, const QString& clipId)
    : Command("Remove Clip"), m_project(project), m_trackId(trackId), m_clipId(clipId) {}

void RemoveClipCommand::execute() {
    auto taken = m_project->takeClipFromTrack(m_trackId, m_clipId);
    if (taken) m_savedClip = *taken;
}

void RemoveClipCommand::undo() {
    m_project->addClipToTrack(m_trackId, m_savedClip);
}

MoveClipCommand::MoveClipCommand(Project* project, const QString& trackId, const QString& clipId,
                                  const RationalTime& newOffset, int newTrackIndex)
    : Command("Move Clip"), m_project(project), m_trackId(trackId), m_clipId(clipId),
      m_newOffset(newOffset), m_newTrackIndex(newTrackIndex) {
    auto* c = m_project->findClip(trackId, clipId);
    m_oldOffset = c ? c->trackOffset : RationalTime(0, 30);
    m_oldTrackIndex = -1;
}

void MoveClipCommand::execute() {
    m_project->moveClip(m_trackId, m_clipId, m_newOffset, m_newTrackIndex);
}

void MoveClipCommand::undo() {
    m_project->moveClip(m_trackId, m_clipId, m_oldOffset, m_oldTrackIndex);
}

SetParamCommand::SetParamCommand(Project* project, const QString& trackId, const QString& clipId,
                                  const QString& paramName, double newValue)
    : Command("Set Parameter"), m_project(project), m_trackId(trackId), m_clipId(clipId),
      m_paramName(paramName), m_newValue(newValue) {
    auto* clip = m_project->findClip(trackId, clipId);
    if (clip && clip->params.contains(paramName)) {
        m_oldValue = clip->params[paramName].value;
    }
}

void SetParamCommand::execute() {
    m_project->setClipParam(m_trackId, m_clipId, m_paramName, m_newValue);
}

void SetParamCommand::undo() {
    m_project->setClipParam(m_trackId, m_clipId, m_paramName, m_oldValue);
}
