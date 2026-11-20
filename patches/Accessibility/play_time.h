// The play time in a save row's text. Engine-free.

#pragma once

#include <cctype>

namespace acc::menus {

// The minutes of the first "<hours>h <minutes>m" in `text` ("QUICKSAVE - 80h
// 24m"), or -1 when there is none. The save list of KOTOR 1 shows nothing
// else that orders its rows in time.
inline int PlayMinutes(const char* text) {
    if (!text) return -1;
    for (const char* p = text; *p; ++p) {
        if (!std::isdigit(static_cast<unsigned char>(*p))) continue;
        int hours = 0;
        const char* q = p;
        while (std::isdigit(static_cast<unsigned char>(*q))) hours = hours * 10 + (*q++ - '0');
        if (*q != 'h') {
            p = q - 1;
            continue;
        }
        ++q;
        while (*q == ' ') ++q;
        if (!std::isdigit(static_cast<unsigned char>(*q))) {
            p = q - 1;
            continue;
        }
        int minutes = 0;
        while (std::isdigit(static_cast<unsigned char>(*q))) minutes = minutes * 10 + (*q++ - '0');
        if (*q == 'm') return hours * 60 + minutes;
        p = q - 1;
    }
    return -1;
}

}  // namespace acc::menus
