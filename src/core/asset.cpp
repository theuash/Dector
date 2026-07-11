#include "asset.h"

void to_json(nlohmann::json& j, const Asset& a) {
    j = {
        {"id", a.id.toStdString()},
        {"name", a.name.toStdString()},
        {"path", a.path.toStdString()},
        {"duration", a.duration},
        {"type", static_cast<int>(a.type)}
    };
}

void from_json(const nlohmann::json& j, Asset& a) {
    a.id = QString::fromStdString(j.at("id").get<std::string>());
    a.name = QString::fromStdString(j.at("name").get<std::string>());
    a.path = QString::fromStdString(j.at("path").get<std::string>());
    a.duration = j.at("duration").get<RationalTime>();
    a.type = static_cast<AssetType>(j.at("type").get<int>());
}
