
#include "examine_view_internal.h"

#include "engine_input.h"
#include "engine_player.h"
#include "engine_subscreen.h" // Begin/EndOverlayPause — freeze the world while open
#include "hotkeys.h"
#include "log.h"
#include "speech.h"

namespace acc::examine_view {

namespace {

void SpeakRow(int idx) {
    if (idx < 0 || idx >= g_state.rowCount) return;
    char out[256];
    std::snprintf(out, sizeof(out),
                  acc::strings::Get(acc::strings::Id::FmtExamineRowOf),
                  g_state.rows[idx], idx + 1, g_state.rowCount);
    acc::speech::Speak(out, /*interrupt=*/true);
}

}  // namespace

bool IsActive() { return g_state.active; }

void ForceDisarm(const char* reason) {
    if (!g_state.active) return;
    acclog::Write("Examine.View", "disarm reason=%s",
                  reason ? reason : "?");
    g_state.active = false;
    acc::engine::EndOverlayPause(acc::engine::OverlayPauseOwner::ExamineView);
    g_state.focusIdx = 0;
    g_state.rowCount = 0;
    g_state.targetHandle = 0;
    g_state.targetObj = nullptr;
}

bool Open() {
    uint32_t handle = ReadLastTargetHandle();
    if (IsSentinel(handle)) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::ExamineNoTarget),
                    /*interrupt=*/true);
        acclog::Write("Examine.View", "Open -- no target (handle=0x%08x)",
                      handle);
        return false;
    }

    g_state.targetHandle = handle;
    int n = BuildRows();
    if (n <= 0) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::ExamineFailed),
                    /*interrupt=*/true);
        acclog::Write("Examine.View",
                      "Open -- BuildRows returned 0 (handle=0x%08x)",
                      handle);
        g_state.targetHandle = 0;
        return false;
    }

    g_state.rowCount = n;
    g_state.focusIdx = 0;
    g_state.active = true;
    acc::engine::BeginOverlayPause(acc::engine::OverlayPauseOwner::ExamineView);

    // Pull the target name out of row 0 for the opener cue. Row 0 format
    // is "Name: <name>" so skip the prefix to get just the name; if the
    // localization changes the prefix we still fall back gracefully on
    // the whole row.
    const char* shortName = g_state.rows[0];
    const char* colon = std::strchr(shortName, ':');
    if (colon && colon[1] == ' ') shortName = colon + 2;

    char opener[256];
    std::snprintf(opener, sizeof(opener),
                  acc::strings::Get(acc::strings::Id::FmtExamineOpened),
                  shortName, n);
    acc::speech::Speak(opener, /*interrupt=*/true);

    acclog::Write("Examine.View",
                  "ARMED handle=0x%08x rows=%d opener=[%s]",
                  handle, n, opener);

    SpeakRow(0);
    return true;
}

bool HandleInputEvent(int code, int value) {
    if (!g_state.active) return false;
    if (value == 0) return false;  // press-edge only

    switch (code) {
        case kInputNavUp:
            if (g_state.focusIdx > 0) --g_state.focusIdx;
            // Rebuild on every step so HP / distance refresh live.
            g_state.rowCount = BuildRows();
            if (g_state.rowCount == 0) {
                ForceDisarm("target-lost-on-up");
                return true;
            }
            if (g_state.focusIdx >= g_state.rowCount) {
                g_state.focusIdx = g_state.rowCount - 1;
            }
            SpeakRow(g_state.focusIdx);
            return true;
        case kInputNavDown:
            g_state.rowCount = BuildRows();
            if (g_state.rowCount == 0) {
                ForceDisarm("target-lost-on-down");
                return true;
            }
            if (g_state.focusIdx + 1 < g_state.rowCount) {
                ++g_state.focusIdx;
            }
            SpeakRow(g_state.focusIdx);
            return true;
        case kInputEnter1:
        case kInputEnter2:
            // Enter = close (read-only view; nothing to commit).
            acc::speech::Speak(acc::strings::Get(acc::strings::Id::ExamineViewClosed),
                        /*interrupt=*/true);
            acclog::Write("Examine.View", "Enter -> close");
            ForceDisarm("enter");
            return true;
        case kInputEsc1:
        case kInputEsc2:
            acc::speech::Speak(acc::strings::Get(acc::strings::Id::ExamineViewClosed),
                        /*interrupt=*/true);
            acclog::Write("Examine.View", "Esc -> close");
            ForceDisarm("esc");
            return true;
        default:
            return false;
    }
}

// Engine-panel open/close logger. The engine's CSWGuiExamine is a generic
void Tick() {
    if (!g_state.active) return;
    // Self-disarm if the player loses connection to the world (area
    // transition / load). Cheap probe.
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) {
        ForceDisarm("player-pos-unresolvable");
        return;
    }
}

void PollWin32Hotkey() {
    if (!acc::hotkeys::Pressed(acc::hotkeys::Action::ExamineOpen)) return;

    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;

    // Toggle: pressing the ExamineOpen key while the view is open closes it.
    if (g_state.active) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::ExamineViewClosed),
                    /*interrupt=*/true);
        acclog::Write("Examine.View", "ExamineOpen key -> close (toggle)");
        ForceDisarm("toggle");
        return;
    }

    Open();
}

}  // namespace acc::examine_view
