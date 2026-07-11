#pragma once
#include "track.h"
#include <vector>

struct Sequence {
    QString id;
    QString name;
    std::vector<Track> tracks;

    Sequence();

    RationalTime calculateDuration() const;
};

void to_json(nlohmann::json& j, const Sequence& s);
void from_json(const nlohmann::json& j, Sequence& s);
