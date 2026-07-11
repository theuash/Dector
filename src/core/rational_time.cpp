#include "rational_time.h"
#include <cmath>

RationalTime::RationalTime(int64_t num, int64_t den) : num(num), den(den > 0 ? den : 1) {
    if (den <= 0) den = 1;
}

double RationalTime::toSeconds() const {
    return static_cast<double>(num) / den;
}

int64_t RationalTime::toFrames(double fps) const {
    return static_cast<int64_t>(std::round(num * fps / den));
}

RationalTime RationalTime::rescaled(int64_t newDen) const {
    if (newDen == den) return *this;
    return RationalTime(num * newDen / den, newDen);
}

RationalTime RationalTime::operator+(const RationalTime& o) const {
    int64_t d = den * o.den;
    int64_t n = num * o.den + o.num * den;
    return RationalTime(n, d);
}

RationalTime RationalTime::operator-(const RationalTime& o) const {
    int64_t d = den * o.den;
    int64_t n = num * o.den - o.num * den;
    return RationalTime(n, d);
}

RationalTime RationalTime::operator*(int64_t s) const {
    return RationalTime(num * s, den);
}

RationalTime RationalTime::operator/(int64_t s) const {
    return RationalTime(num, den * s);
}

RationalTime& RationalTime::operator+=(const RationalTime& o) {
    *this = *this + o;
    return *this;
}

RationalTime& RationalTime::operator-=(const RationalTime& o) {
    *this = *this - o;
    return *this;
}

RationalTime RationalTime::fromSeconds(double s, double rate) {
    int64_t d = static_cast<int64_t>(rate);
    int64_t n = static_cast<int64_t>(std::round(s * rate));
    return RationalTime(n, d);
}

RationalTime RationalTime::fromFrames(int64_t f, double fps) {
    return RationalTime(f, static_cast<int64_t>(fps));
}

void to_json(nlohmann::json& j, const RationalTime& t) {
    j = { {"num", t.num}, {"den", t.den} };
}

void from_json(const nlohmann::json& j, RationalTime& t) {
    t.num = j.at("num").get<int64_t>();
    t.den = j.at("den").get<int64_t>();
}
