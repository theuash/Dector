#pragma once
#include "rational_time.h"
#include <QString>
#include <vector>

struct Keyframe {
    RationalTime time;
    double value;
};

enum class BlendMode {
    Normal, Multiply, Screen, Overlay, Add
};

struct Parameter {
    QString name;
    double defaultValue = 0.0;
    double value = 0.0;
    std::vector<Keyframe> keyframes;

    double getValueAt(const RationalTime& t) const;
};

void to_json(nlohmann::json& j, const Parameter& p);
void from_json(const nlohmann::json& j, Parameter& p);
