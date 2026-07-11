#pragma once
#include <cstdint>
#include <compare>
#include <nlohmann/json.hpp>

struct RationalTime {
    int64_t num = 0;
    int64_t den = 1;

    explicit RationalTime(int64_t num = 0, int64_t den = 1);

    double toSeconds() const;
    int64_t toFrames(double fps) const;

    RationalTime rescaled(int64_t newDen) const;

    RationalTime operator+(const RationalTime& o) const;
    RationalTime operator-(const RationalTime& o) const;
    RationalTime operator*(int64_t s) const;
    RationalTime operator/(int64_t s) const;
    RationalTime& operator+=(const RationalTime& o);
    RationalTime& operator-=(const RationalTime& o);
    auto operator<=>(const RationalTime&) const = default;
    bool operator==(const RationalTime&) const = default;

    bool isValid() const { return den > 0; }

    static RationalTime fromSeconds(double s, double rate = 24.0);
    static RationalTime fromFrames(int64_t f, double fps);
};

void to_json(nlohmann::json& j, const RationalTime& t);
void from_json(const nlohmann::json& j, RationalTime& t);
