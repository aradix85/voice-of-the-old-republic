#include "save_points.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace acc::save_points {

Spot At(const Vector& p, const std::string& value) {
    Spot s;
    s.x     = std::lround(p.x);
    s.y     = std::lround(p.y);
    s.value = value;
    return s;
}

std::vector<Spot> Parse(const char* text) {
    std::vector<Spot> out;
    const char* p = text ? text : "";
    while (*p) {
        char* end = nullptr;
        Spot s;
        s.x = std::strtol(p, &end, 10);
        if (end == p || *end != ',') break;
        p = end + 1;
        s.y = std::strtol(p, &end, 10);
        if (end == p) break;
        p = end;
        bool keyed = false;
        if (*p == '=') {
            keyed = true;
            ++p;
            while (*p && *p != ';') s.value.push_back(*p++);
        }
        if (!keyed || !s.value.empty()) out.push_back(std::move(s));
        if (*p == ';') {
            ++p;
        } else if (*p) {
            break;
        }
    }
    return out;
}

std::string Format(const std::vector<Spot>& spots) {
    std::string out;
    for (const Spot& s : spots) {
        if (!out.empty()) out.push_back(';');
        out += std::to_string(s.x) + ',' + std::to_string(s.y);
        if (!s.value.empty()) out += '=' + s.value;
    }
    return out;
}

const Spot* Nearest(const std::vector<Spot>& spots, const Vector& p, float maxM) {
    const Spot* best   = nullptr;
    float       bestSq = maxM * maxM;
    for (const Spot& s : spots) {
        const Vector at{static_cast<float>(s.x), static_cast<float>(s.y), 0.0f};
        const float  d2 = DistXYSq(at, p);
        if (d2 <= bestSq) {
            bestSq = d2;
            best   = &s;
        }
    }
    return best;
}

void Put(std::vector<Spot>& spots, const Spot& s, long sameM, size_t max) {
    const long same2 = sameM * sameM;
    spots.erase(std::remove_if(spots.begin(), spots.end(),
                               [&](const Spot& o) {
                                   const long dx = o.x - s.x;
                                   const long dy = o.y - s.y;
                                   return dx * dx + dy * dy <= same2;
                               }),
                spots.end());
    spots.push_back(s);
    if (spots.size() > max) {
        spots.erase(spots.begin(), spots.end() - static_cast<std::ptrdiff_t>(max));
    }
}

}  // namespace acc::save_points
