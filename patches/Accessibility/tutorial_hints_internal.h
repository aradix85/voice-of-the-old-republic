// Shared by the tutorial_hints files: strref texts resolved through the
// engine's TLK, so every match works in the game's own wording.

#pragma once

#include <cstdint>

#include "engine_reads.h"  // LookupTlk
#include "log.h"
#include "tutorial_hints.h"

namespace acc::tutorial_hints {

// Up to Count strref texts of up to Len bytes, resolved on first use. Before
// the TLK is live (main menu, early load) every lookup fails, so the table
// stays unbuilt and retries on the next call; an entry that fails stays
// empty and never matches.
template <int Count, int Len>
struct TlkTexts {
    char text[Count][Len];
    int  count = 0;
    bool built = false;

    // strrefAt(i) is entry i's strref; `what` names the table in the log.
    template <typename StrrefAt>
    bool Ensure(int n, StrrefAt strrefAt, const char* what) {
        if (built) return true;
        int resolved = 0;
        for (int i = 0; i < n; ++i) {
            text[i][0] = '\0';
            if (acc::engine::LookupTlk(strrefAt(i), text[i], sizeof(text[i]))) ++resolved;
        }
        if (resolved == 0) return false;
        built = true;
        count = n;
        acclog::Write("TutorialHint", "%s map built: %d/%d strrefs resolved", what, resolved, n);
        return true;
    }

    // The first entry whose text `matches` accepts, or -1.
    template <typename Match>
    int Find(Match matches) const {
        for (int i = 0; i < count; ++i) {
            if (text[i][0] && matches(text[i])) return i;
        }
        return -1;
    }
};

}  // namespace acc::tutorial_hints
