#pragma once
#include "clip.h"
#include <QString>
#include <vector>

enum class TrackType { Video, Audio };

struct Track {
    QString id;
    QString name;
    TrackType type = TrackType::Video;
    bool enabled = true;
    bool locked = false;
    std::vector<Clip> clips;

    Track();
};

void to_json(nlohmann::json& j, const Track& t);
void from_json(const nlohmann::json& j, Track& t);
