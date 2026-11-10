// The Audio glossary submenu: one row per swoop-race cue, auditioned a moment
// after Enter.

#include "menus_modsettings_internal.h"

#include "audio_cues.h"
#include "engine_input.h"
#include "log.h"
#include "speech.h"

#include <windows.h>

#include <cstdint>

namespace acc::menus::modsettings {

bool s_glossaryOpen = false;
acc::audio::LoopSource s_preview;

namespace {

struct GlossaryEntry {
    acc::audio::NavCue cue;
    Id                 label;
};

// In audio_cues.h order.
constexpr GlossaryEntry k_glossary[] = {
    { acc::audio::NavCue::SwoopAccelpadBoost, Id::GlossaryEntrySwoopAccelpadBoost },
    { acc::audio::NavCue::SwoopObstacleWarn,  Id::GlossaryEntrySwoopObstacleWarn },
    { acc::audio::NavCue::SwoopWallImpact,    Id::GlossaryEntrySwoopWallImpact },
    { acc::audio::NavCue::SwoopSteerAligned,  Id::GlossaryEntrySwoopAligned },
    { acc::audio::NavCue::SwoopShiftReady,    Id::GlossaryEntrySwoopShiftReady },
    { acc::audio::NavCue::SwoopJumpNow,       Id::GlossaryEntrySwoopJumpNow },
};
constexpr int k_glossaryCount = static_cast<int>(sizeof(k_glossary) / sizeof(k_glossary[0]));

int s_glossaryFocused = 0;  // apart from the list's cursor, which stays on this row

// Enter arms the audition and Tick plays it once the row name has been spoken,
// so speech and sample don't overlap. Another Enter moves the deadline.
bool               s_pendingValid   = false;
DWORD              s_pendingFireAt  = 0;
acc::audio::NavCue s_pendingCue     = acc::audio::NavCue::SwoopShiftReady;
constexpr DWORD    kGlossaryDelayMs = 750;

void SpeakFocusedGlossaryEntry() {
    const char* name = acc::strings::Get(k_glossary[s_glossaryFocused].label);
    acc::speech::Speak(name, /*interrupt=*/true);
    acclog::Write("ModSettings", "glossary speak focused idx=%d \"%s\"",
                  s_glossaryFocused, name);
}

// So a queued cue does not play after the user has moved on.
void CancelPendingGlossaryCue() {
    if (!s_pendingValid) return;
    acclog::Write("ModSettings", "glossary cancel pending cue=%d",
                  static_cast<int>(s_pendingCue));
    s_pendingValid  = false;
    s_pendingFireAt = 0;
}

void CloseGlossarySubMenu() {
    if (!s_glossaryOpen) return;
    acclog::Write("ModSettings", "glossary closed");
    ResetGlossary();
    s_preview.Stop();
    SpeakFocusedOption();  // back on the Audio glossary row
}

}  // namespace

void OpenGlossarySubMenu() {
    s_glossaryOpen    = true;
    s_glossaryFocused = 0;
    CancelPendingGlossaryCue();
    acclog::Write("ModSettings", "glossary opened");
    acc::speech::Speak(acc::strings::Get(Id::ModSettingsAudioGlossaryOpened),
                       /*interrupt=*/true);
    acc::speech::Speak(acc::strings::Get(k_glossary[0].label), /*interrupt=*/false);
}

void ResetGlossary() {
    s_glossaryOpen    = false;
    s_glossaryFocused = 0;
    CancelPendingGlossaryCue();
}

bool HandleInputGlossary(int keyCode) {
    if (StepListCursor(keyCode, k_glossaryCount, &s_glossaryFocused)) {
        CancelPendingGlossaryCue();
        SpeakFocusedGlossaryEntry();
        return true;
    }
    if (keyCode == kInputEnter1 || keyCode == kInputEnter2) {
        s_pendingCue    = k_glossary[s_glossaryFocused].cue;
        s_pendingFireAt = GetTickCount() + kGlossaryDelayMs;
        s_pendingValid  = true;
        acclog::Write("ModSettings", "glossary armed cue=%d delay=%lums (idx=%d)",
                      static_cast<int>(s_pendingCue), (unsigned long)kGlossaryDelayMs,
                      s_glossaryFocused);
        return true;
    }
    if (keyCode == kInputEsc1 || keyCode == kInputEsc2) {
        CloseGlossarySubMenu();  // back to the list, not the parent panel
        return true;
    }
    return false;
}

void TickGlossary() {
    if (!s_pendingValid) return;
    if (static_cast<int32_t>(GetTickCount() - s_pendingFireAt) < 0) return;  // wrap-safe
    s_pendingValid = false;
    const char* resref = acc::audio::GetNavCueResref(s_pendingCue);
    // 2D, and not gated on the cue's own toggle: the user auditions the sound.
    const Vector kCentre = { 0.0f, 0.0f, 0.0f };
    const bool ok = s_preview.Start(resref, kCentre, /*looping=*/false, /*spatial=*/false,
                                    /*priorityGroup=*/kGuiPriorityGroup);
    acclog::Write("ModSettings", "glossary fire cue=%d resref=\"%s\" played=%d",
                  static_cast<int>(s_pendingCue), resref, ok ? 1 : 0);
}

}  // namespace acc::menus::modsettings
