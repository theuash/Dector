#include "clip.h"
#include <QUuid>

Clip::Clip() : id(QUuid::createUuid().toString(QUuid::WithoutBraces)) {
    params["position.x"] = {"Position X", 0.0, 0.0, {}};
    params["position.y"] = {"Position Y", 0.0, 0.0, {}};
    params["scale.x"] = {"Scale X", 1.0, 1.0, {}};
    params["scale.y"] = {"Scale Y", 1.0, 1.0, {}};
    params["rotation"] = {"Rotation", 0.0, 0.0, {}};
    params["opacity"] = {"Opacity", 1.0, 1.0, {}};
}

void to_json(nlohmann::json& j, const Clip& c) {
    j = {
        {"id", c.id.toStdString()},
        {"asset_id", c.assetId.toStdString()},
        {"name", c.name.toStdString()},
        {"source_start", c.sourceStart},
        {"source_duration", c.sourceDuration},
        {"track_offset", c.trackOffset},
        {"enabled", c.enabled},
        {"blend_mode", static_cast<int>(c.blendMode)},
        {"params", nlohmann::json::object()}
    };
    for (auto it = c.params.begin(); it != c.params.end(); ++it) {
        j["params"][it.key().toStdString()] = it.value();
    }
}

void from_json(const nlohmann::json& j, Clip& c) {
    c.id = QString::fromStdString(j.at("id").get<std::string>());
    c.assetId = QString::fromStdString(j.at("asset_id").get<std::string>());
    c.name = QString::fromStdString(j.value("name", ""));
    c.sourceStart = j.at("source_start").get<RationalTime>();
    c.sourceDuration = j.at("source_duration").get<RationalTime>();
    c.trackOffset = j.at("track_offset").get<RationalTime>();
    c.enabled = j.value("enabled", true);
    c.blendMode = static_cast<BlendMode>(j.value("blend_mode", 0));
    if (j.contains("params")) {
        for (const auto& [key, val] : j["params"].items()) {
            c.params[QString::fromStdString(key)] = val.get<Parameter>();
        }
    }
}
