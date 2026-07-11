#pragma once
#include "rational_time.h"
#include <QString>
#include <QUuid>

enum class AssetType { Video, Audio, Image, Other };

struct Asset {
    QString id;
    QString name;
    QString path;
    RationalTime duration;
    AssetType type = AssetType::Video;

    Asset() : id(QUuid::createUuid().toString(QUuid::WithoutBraces)) {}
};

void to_json(nlohmann::json& j, const Asset& a);
void from_json(const nlohmann::json& j, Asset& a);
