#include "track.h"
#include <QUuid>

Track::Track() : id(QUuid::createUuid().toString(QUuid::WithoutBraces)) {}

void to_json(nlohmann::json& j, const Track& t) {
    j = {
        {"id", t.id.toStdString()},
        {"name", t.name.toStdString()},
        {"type", static_cast<int>(t.type)},
        {"enabled", t.enabled},
        {"locked", t.locked},
        {"clips", nlohmann::json::array()}
    };
    for (const auto& clip : t.clips) {
        j["clips"].push_back(clip);
    }
}

void from_json(const nlohmann::json& j, Track& t) {
    t.id = QString::fromStdString(j.at("id").get<std::string>());
    t.name = QString::fromStdString(j.value("name", ""));
    t.type = static_cast<TrackType>(j.value("type", 0));
    t.enabled = j.value("enabled", true);
    t.locked = j.value("locked", false);
    t.clips.clear();
    for (const auto& cj : j.at("clips")) {
        t.clips.push_back(cj.get<Clip>());
    }
}
