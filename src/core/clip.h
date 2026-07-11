#pragma once
#include "rational_time.h"
#include "parameter.h"
#include <QString>
#include <QMap>
#include <memory>

struct Clip {
    QString id;
    QString assetId;
    QString name;
    RationalTime sourceStart;
    RationalTime sourceDuration;
    RationalTime trackOffset;
    bool enabled = true;
    QMap<QString, Parameter> params;
    BlendMode blendMode = BlendMode::Normal;

    Clip();

    RationalTime endTime() const { return trackOffset + sourceDuration; }
};

void to_json(nlohmann::json& j, const Clip& c);
void from_json(const nlohmann::json& j, Clip& c);
