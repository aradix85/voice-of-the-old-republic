// The kills of one fight, for the summary at its end: the names from the
// engine's "<actor> killed <target>: N XP" lines, counted per name in the
// order of the first kill. Engine-free.

#pragma once

#include <cstring>
#include <string>
#include <vector>

namespace acc::combat {

// The target of a kill line: what stands between the marker and the last
// colon. Empty when the line has no marker.
inline std::string KillTarget(const char* line, const char* marker) {
    if (!line || !marker || !marker[0]) return {};
    const char* at = std::strstr(line, marker);
    if (!at) return {};
    const char* start = at + std::strlen(marker);
    const char* end = std::strrchr(start, ':');
    if (!end) end = start + std::strlen(start);
    while (start < end && *start == ' ') ++start;
    while (end > start && (end[-1] == ' ' || end[-1] == '.')) --end;
    return std::string(start, end);
}

class KillTally {
public:
    void Clear() { kills_.clear(); }

    void Note(const std::string& name) {
        if (name.empty()) return;
        for (Kill& k : kills_) {
            if (k.name == name) {
                ++k.count;
                return;
            }
        }
        kills_.push_back(Kill{name, 1});
    }

    bool Empty() const { return kills_.empty(); }

    // "2 Gamorrean Elite, Gamorrean Chief"
    std::string List() const {
        std::string out;
        for (const Kill& k : kills_) {
            if (!out.empty()) out += ", ";
            if (k.count > 1) out += std::to_string(k.count) + " ";
            out += k.name;
        }
        return out;
    }

private:
    struct Kill {
        std::string name;
        int         count;
    };
    std::vector<Kill> kills_;
};

}  // namespace acc::combat
