#include "sequence.h"
#include <QUuid>
#include <algorithm>

Sequence::Sequence() : id(QUuid::createUuid().toString(QUuid::WithoutBraces)), name("Sequence 1") {
    tracks.push_back(Track{});
    tracks.back().name = "Video 1";
}

RationalTime Sequence::calculateDuration() const {
    RationalTime maxEnd(0, 1);
    for (const auto& track : tracks) {
        for (const auto& clip : track.clips) {
            RationalTime clipEnd = clip.endTime();
            if (clipEnd > maxEnd) maxEnd = clipEnd;
        }
    }
    return maxEnd;
}

void to_json(nlohmann::json& j, const Sequence& s) {
    j = {
        {"id", s.id.toStdString()},
        {"name", s.name.toStdString()},
        {"tracks", nlohmann::json::array()}
    };
    for (const auto& track : s.tracks) {
        j["tracks"].push_back(track);
    }
}

void from_json(const nlohmann::json& j, Sequence& s) {
    s.id = QString::fromStdString(j.at("id").get<std::string>());
    s.name = QString::fromStdString(j.value("name", "Sequence"));
    s.tracks.clear();
    for (const auto& tj : j.at("tracks")) {
        s.tracks.push_back(tj.get<Track>());
    }
}
