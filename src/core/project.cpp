#include "project.h"
#include <fstream>

Project::Project(QObject* parent) : QObject(parent) {
    addSequence("Sequence 1");
}

Asset* Project::addAsset(std::unique_ptr<Asset> asset) {
    auto* ptr = asset.get();
    m_assets.push_back(std::move(asset));
    setModified(true);
    emit assetAdded(ptr->id);
    return ptr;
}

std::unique_ptr<Asset> Project::takeAsset(const QString& id) {
    for (auto it = m_assets.begin(); it != m_assets.end(); ++it) {
        if ((*it)->id == id) {
            auto asset = std::move(*it);
            m_assets.erase(it);
            setModified(true);
            emit assetRemoved(id);
            return asset;
        }
    }
    return nullptr;
}

Asset* Project::asset(const QString& id) const {
    for (const auto& a : m_assets) {
        if (a->id == id) return a.get();
    }
    return nullptr;
}

void Project::removeAsset(const QString& id) {
    takeAsset(id);
}

QString Project::addTrack(TrackType type) {
    auto* seq = currentSequence();
    if (!seq) return {};
    Track track;
    track.type = type;
    track.name = QString(type == TrackType::Video ? "Video " : "Audio ")
                  + QString::number(seq->tracks.size() + 1);
    QString id = track.id;
    seq->tracks.push_back(std::move(track));
    setModified(true);
    emit sequenceChanged();
    return id;
}

void Project::removeTrack(const QString& id) {
    auto* seq = currentSequence();
    if (!seq) return;
    for (auto it = seq->tracks.begin(); it != seq->tracks.end(); ++it) {
        if (it->id == id) {
            seq->tracks.erase(it);
            setModified(true);
            emit sequenceChanged();
            return;
        }
    }
}

void Project::addClipToTrack(const QString& trackId, const Clip& clip) {
    auto* t = findTrack(trackId);
    if (!t) return;
    t->clips.push_back(clip);
    setModified(true);
    emit sequenceChanged();
}

std::unique_ptr<Clip> Project::takeClipFromTrack(const QString& trackId, const QString& clipId) {
    auto* t = findTrack(trackId);
    if (!t) return nullptr;
    for (auto it = t->clips.begin(); it != t->clips.end(); ++it) {
        if (it->id == clipId) {
            auto clip = std::make_unique<Clip>(std::move(*it));
            t->clips.erase(it);
            setModified(true);
            emit sequenceChanged();
            return clip;
        }
    }
    return nullptr;
}

void Project::removeClipFromTrack(const QString& trackId, const QString& clipId) {
    takeClipFromTrack(trackId, clipId);
}

Clip* Project::findClip(const QString& trackId, const QString& clipId) const {
    auto* t = findTrack(trackId);
    if (!t) return nullptr;
    for (auto& c : t->clips) {
        if (c.id == clipId) return &c;
    }
    return nullptr;
}

void Project::moveClip(const QString& trackId, const QString& clipId,
                        const RationalTime& newOffset, int newTrackIndex) {
    auto clip = takeClipFromTrack(trackId, clipId);
    if (!clip) return;
    clip->trackOffset = newOffset;
    if (newTrackIndex >= 0 && newTrackIndex < static_cast<int>(currentSequence()->tracks.size())) {
        currentSequence()->tracks[newTrackIndex].clips.push_back(std::move(*clip));
    } else {
        addClipToTrack(trackId, *clip);
    }
}

void Project::setClipParam(const QString& trackId, const QString& clipId,
                            const QString& paramName, double value) {
    auto* c = findClip(trackId, clipId);
    if (!c || !c->params.contains(paramName)) return;
    c->params[paramName].value = value;
    setModified(true);
    emit sequenceChanged();
}

void Project::setCurrentSequence(const QString& id) {
    m_currentSequenceId = id;
    emit currentSequenceChanged(id);
}

Sequence* Project::currentSequence() const {
    if (m_currentSequenceId.isEmpty() && !m_sequences.empty())
        return m_sequences.front().get();
    return sequence(m_currentSequenceId);
}

Sequence* Project::sequence(const QString& id) const {
    for (const auto& s : m_sequences) {
        if (s->id == id) return s.get();
    }
    return nullptr;
}

QString Project::addSequence(const QString& name) {
    auto seq = std::make_unique<Sequence>();
    if (!name.isEmpty()) seq->name = name;
    QString id = seq->id;
    m_sequences.push_back(std::move(seq));
    if (m_currentSequenceId.isEmpty()) m_currentSequenceId = id;
    setModified(true);
    emit sequenceChanged();
    return id;
}

bool Project::save(const QString& path) {
    nlohmann::json j;
    j["version"] = "0.1.0";
    j["name"] = m_name.toStdString();

    j["assets"] = nlohmann::json::array();
    for (const auto& a : m_assets) j["assets"].push_back(*a);

    j["sequences"] = nlohmann::json::array();
    for (const auto& s : m_sequences) j["sequences"].push_back(*s);

    j["current_sequence"] = m_currentSequenceId.toStdString();

    std::ofstream file(path.toStdString());
    if (!file) return false;
    file << j.dump(2);
    file.close();

    m_path = path;
    setModified(false);
    return true;
}

bool Project::load(const QString& path) {
    std::ifstream file(path.toStdString());
    if (!file) return false;
    nlohmann::json j;
    try { file >> j; } catch (...) { return false; }

    m_name = QString::fromStdString(j.value("name", "Untitled"));

    m_assets.clear();
    if (j.contains("assets")) {
        for (const auto& aj : j["assets"])
            m_assets.push_back(std::make_unique<Asset>(aj.get<Asset>()));
    }

    m_sequences.clear();
    if (j.contains("sequences")) {
        for (const auto& sj : j["sequences"])
            m_sequences.push_back(std::make_unique<Sequence>(sj.get<Sequence>()));
    }
    m_currentSequenceId = QString::fromStdString(j.value("current_sequence", ""));

    m_path = path;
    setModified(false);
    emit sequenceChanged();
    return true;
}

void Project::setName(const QString& n) {
    m_name = n;
    setModified(true);
}

Track* Project::findTrack(const QString& id) const {
    auto* seq = currentSequence();
    if (!seq) return nullptr;
    for (auto& t : seq->tracks) {
        if (t.id == id) return &t;
    }
    return nullptr;
}

void Project::setModified(bool m) {
    if (m_modified != m) {
        m_modified = m;
        emit modifiedChanged(m);
    }
}
