// The examine panel: a read-only list of rows about the current target (name,
// faction, condition, hit points, level, distance, flags, equipment, effects,
// feats), walked with Up/Down while the world is paused.

#include "examine_view_internal.h"

#include "engine_input.h"
#include "engine_player.h"
#include "engine_subscreen.h"  // Begin/EndOverlayPause: the world waits while open
#include "hotkeys.h"
#include "log.h"
#include "speech.h"

#include <cstdio>
#include <cstring>

namespace acc::examine_view {

State g_state;

namespace {

void SpeakRow(int idx) {
    if (idx < 0 || idx >= g_state.rowCount) return;
    char out[256];
    std::snprintf(out, sizeof(out),
                  acc::strings::Get(acc::strings::Id::FmtExamineRowOf),
                  g_state.rows[idx], idx + 1, g_state.rowCount);
    acc::speech::Speak(out, /*interrupt=*/true);
}

bool InWorld() {
    Vector unused;
    return acc::engine::GetPlayerPosition(unused);
}

void Close(const char* reason) {
    acc::speech::Speak(acc::strings::Get(acc::strings::Id::ExamineViewClosed),
                       /*interrupt=*/true);
    ForceDisarm(reason);
}

// Rows are rebuilt on every step, so hit points and distance stay live.
// False (and disarmed) when the target is gone.
bool Refresh(const char* lostReason) {
    g_state.rowCount = BuildRows();
    if (g_state.rowCount > 0) return true;
    ForceDisarm(lostReason);
    return false;
}

}  // namespace

bool IsActive() { return g_state.active; }

void ForceDisarm(const char* reason) {
    if (!g_state.active) return;
    acclog::Write("Examine.View", "disarm reason=%s", reason ? reason : "?");
    g_state.active = false;
    acc::engine::EndOverlayPause(acc::engine::OverlayPauseOwner::ExamineView);
    g_state.focusIdx = 0;
    g_state.rowCount = 0;
    g_state.targetHandle = 0;
    g_state.targetObj = nullptr;
}

bool Open() {
    const uint32_t handle = ReadLastTargetHandle();
    if (IsSentinel(handle)) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::ExamineNoTarget),
                           /*interrupt=*/true);
        acclog::Write("Examine.View", "Open -- no target (handle=0x%08x)", handle);
        return false;
    }

    g_state.targetHandle = handle;
    const int n = BuildRows();
    if (n <= 0) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::ExamineFailed),
                           /*interrupt=*/true);
        acclog::Write("Examine.View", "Open -- BuildRows returned 0 (handle=0x%08x)",
                      handle);
        g_state.targetHandle = 0;
        return false;
    }

    g_state.rowCount = n;
    g_state.focusIdx = 0;
    g_state.active = true;
    acc::engine::BeginOverlayPause(acc::engine::OverlayPauseOwner::ExamineView);

    // Row 0 is "Name: <name>"; the opener wants the name alone (the whole row
    // if the prefix ever changes shape).
    const char* shortName = g_state.rows[0];
    const char* colon = std::strchr(shortName, ':');
    if (colon && colon[1] == ' ') shortName = colon + 2;

    char opener[256];
    std::snprintf(opener, sizeof(opener),
                  acc::strings::Get(acc::strings::Id::FmtExamineOpened), shortName, n);
    acc::speech::Speak(opener, /*interrupt=*/true);
    acclog::Write("Examine.View", "ARMED handle=0x%08x rows=%d opener=[%s]",
                  handle, n, opener);
    SpeakRow(0);
    return true;
}

bool HandleInputEvent(int code, int value) {
    if (!g_state.active) return false;
    if (value == 0) return false;  // press edge only

    switch (code) {
        case kInputNavUp:
            if (g_state.focusIdx > 0) --g_state.focusIdx;
            if (!Refresh("target-lost-on-up")) return true;
            if (g_state.focusIdx >= g_state.rowCount) g_state.focusIdx = g_state.rowCount - 1;
            SpeakRow(g_state.focusIdx);
            return true;
        case kInputNavDown:
            if (!Refresh("target-lost-on-down")) return true;
            if (g_state.focusIdx + 1 < g_state.rowCount) ++g_state.focusIdx;
            SpeakRow(g_state.focusIdx);
            return true;
        case kInputEnter1:
        case kInputEnter2:
            Close("enter");  // a read-only view: Enter closes too
            return true;
        case kInputEsc1:
        case kInputEsc2:
            Close("esc");
            return true;
        default:
            return false;
    }
}

void Tick() {
    if (g_state.active && !InWorld()) ForceDisarm("player-pos-unresolvable");
}

// The ExamineOpen key toggles the panel.
void PollWin32Hotkey() {
    if (!acc::hotkeys::Pressed(acc::hotkeys::Action::ExamineOpen)) return;
    if (!InWorld()) return;
    if (g_state.active) {
        Close("toggle");
        return;
    }
    Open();
}

}  // namespace acc::examine_view
