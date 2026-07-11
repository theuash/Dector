#include "parameter.h"

double Parameter::getValueAt(const RationalTime& t) const {
    if (keyframes.empty()) return value;
    if (t <= keyframes.front().time) return keyframes.front().value;
    if (t >= keyframes.back().time) return keyframes.back().value;

    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (t >= keyframes[i].time && t < keyframes[i + 1].time) {
            RationalTime range = keyframes[i + 1].time - keyframes[i].time;
            RationalTime offset = t - keyframes[i].time;
            double frac = range.toSeconds() > 0 ? offset.toSeconds() / range.toSeconds() : 0;
            return keyframes[i].value + frac * (keyframes[i + 1].value - keyframes[i].value);
        }
    }
    return value;
}

void to_json(nlohmann::json& j, const Parameter& p) {
    j = {
        {"name", p.name.toStdString()},
        {"default", p.defaultValue},
        {"value", p.value},
        {"keyframes", nlohmann::json::array()}
    };
    for (const auto& kf : p.keyframes) {
        j["keyframes"].push_back({{"time", kf.time}, {"value", kf.value}});
    }
}

void from_json(const nlohmann::json& j, Parameter& p) {
    p.name = QString::fromStdString(j.at("name").get<std::string>());
    p.defaultValue = j.value("default", 0.0);
    p.value = j.value("value", 0.0);
    p.keyframes.clear();
    if (j.contains("keyframes")) {
        for (const auto& kf : j["keyframes"]) {
            p.keyframes.push_back({kf.at("time").get<RationalTime>(), kf.at("value").get<double>()});
        }
    }
}
