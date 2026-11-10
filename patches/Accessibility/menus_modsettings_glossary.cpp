// The Audio glossary submenu: one row per swoop-race cue, auditioned a moment after Enter.

#include "menus_modsettings_internal.h"

#include "engine_input.h"
#include "log.h"
#include "menus_nav.h"       // ClampedCursorStep — shared Up/Down/Home/End ladder
#include "speech.h"

namespace acc::menus::modsettings {

namespace {

// Glossary entries — one row per swoop-race cue, in audio_cues.h order.
struct GlossaryEntry {
    acc::audio::NavCue  cue;
    acc::strings::Id    label;
};

constexpr GlossaryEntry k_glossary[] = {
    { acc::audio::NavCue::SwoopAccelpadBoost,       acc::strings::Id::GlossaryEntrySwoopAccelpadBoost },
    { acc::audio::NavCue::SwoopObstacleWarn,        acc::strings::Id::GlossaryEntrySwoopObstacleWarn  },
    { acc::audio::NavCue::SwoopWallImpact,          acc::strings::Id::GlossaryEntrySwoopWallImpact    },
    { acc::audio::NavCue::SwoopSteerAligned,        acc::strings::Id::GlossaryEntrySwoopAligned       },
    { acc::audio::NavCue::SwoopShiftReady,          acc::strings::Id::GlossaryEntrySwoopShiftReady    },
    { acc::audio::NavCue::SwoopJumpNow,             acc::strings::Id::GlossaryEntrySwoopJumpNow       },
};
constexpr int k_glossaryCount = static_cast<int>(
    sizeof(k_glossary) / sizeof(k_glossary[0]));

// Speak the current focused glossary entry — just the localised name.
// State / cue resref aren't spoken; the user hears the cue itself one
// second after Enter, which is the meaningful signal.
void SpeakFocusedGlossaryEntry() {
    if (s_glossaryFocused < 0 || s_glossaryFocused >= k_glossaryCount) return;
    const char* name =
        acc::strings::Get(k_glossary[s_glossaryFocused].label);
    acc::speech::Speak(name, /*interrupt=*/true);
    acclog::Write("ModSettings", "glossary speak focused idx=%d \"%s\"",
                  s_glossaryFocused, name);
}

// Close the glossary submenu, returning the user to the root. Speaks
// the root row they came from so they have an audible anchor after the
// nested Esc.
void CloseGlossarySubMenu() {
    if (!s_glossaryOpen) return;
    acclog::Write("ModSettings", "glossary closed");
    s_glossaryOpen    = false;
    s_glossaryFocused = 0;
    CancelPendingGlossaryCue();
    s_glossaryPreview.Stop();  // silence any in-flight audition on close
    // Re-announce the root focus (the AudioGlossary row) so the user
    // knows they're back at the outer level. The root focus is
    // unchanged from before — it was the AudioGlossary row that was
    // Enter'd in the first place.
    SpeakFocusedOption();
}

}  // namespace

// Cancel any pending delayed glossary playback. Called from every
// glossary navigation / close path so a queued cue doesn't fire after
// the user has moved on.
void CancelPendingGlossaryCue() {
    if (!s_pendingValid) return;
    acclog::Write("ModSettings", "glossary cancel pending cue=%d",
                  static_cast<int>(s_pendingCue));
    s_pendingValid  = false;
    s_pendingFireAt = 0;
}

// Open the nested Audio glossary submenu and announce its title + first
// row. Called from HandleInput when the user presses Enter on the
// AudioGlossary row in the root submenu.
void OpenGlossarySubMenu() {
    s_glossaryOpen    = true;
    s_glossaryFocused = 0;
    CancelPendingGlossaryCue();
    acclog::Write("ModSettings", "glossary opened");
    acc::speech::Speak(
        acc::strings::Get(acc::strings::Id::ModSettingsAudioGlossaryOpened),
        /*interrupt=*/true);
    if (k_glossaryCount > 0) {
        acc::speech::Speak(
            acc::strings::Get(k_glossary[s_glossaryFocused].label),
            /*interrupt=*/false);
    }
}

bool HandleInputGlossary(int keyCode) {
    // Same clamped ladder as the root list — Home / End jump to the first /
    // last cue entry.
    {
        const bool up   = (keyCode == kInputNavUp);
        const bool down = (keyCode == kInputNavDown);
        const bool home = (keyCode == kInputHome);
        const bool end  = (keyCode == kInputEnd);
        if (up || down || home || end) {
            if (k_glossaryCount <= 0) return true;
            s_glossaryFocused = acc::menus::nav::ClampedCursorStep(
                s_glossaryFocused, k_glossaryCount, up, down, home, end);
            CancelPendingGlossaryCue();
            SpeakFocusedGlossaryEntry();
            return true;
        }
    }
    // Enter: arm the 1 s delayed playback. Re-pressing Enter replaces
    // the pending deadline so a held / re-mashed key just shifts the
    // fire-at forward; the user always hears the cue exactly once,
    // 1 s after the most recent Enter.
    if (keyCode == kInputEnter1 || keyCode == kInputEnter2) {
        if (s_glossaryFocused < 0 || s_glossaryFocused >= k_glossaryCount) {
            return true;
        }
        s_pendingCue    = k_glossary[s_glossaryFocused].cue;
        s_pendingFireAt = GetTickCount() + kGlossaryDelayMs;
        s_pendingValid  = true;
        acclog::Write("ModSettings",
                      "glossary armed cue=%d delay=%lums (idx=%d)",
                      static_cast<int>(s_pendingCue),
                      (unsigned long)kGlossaryDelayMs,
                      s_glossaryFocused);
        return true;
    }
    // Esc: return to the root submenu (NOT the parent panel).
    if (keyCode == kInputEsc1 || keyCode == kInputEsc2) {
        CloseGlossarySubMenu();
        return true;
    }
    return false;
}

}  // namespace acc::menus::modsettings
