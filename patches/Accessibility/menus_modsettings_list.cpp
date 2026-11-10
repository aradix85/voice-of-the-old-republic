// Input on the option list: toggles, the cue-volume slider, the submenu pivots and the link rows.

#include "menus_modsettings_internal.h"

#include "audio_bus.h"        // global cue-volume slider get/set
#include "engine_input.h"
#include "intro_skip.h"      // SkipIntros toggle — filesystem-backed state
#include "log.h"
#include "menus_keybinds.h"  // nested keybind configurator submenu
#include "menus_nav.h"       // ClampedCursorStep — shared Up/Down/Home/End ladder
#include "mod_settings_store.h"  // persist toggles across launches
#include "speech.h"

namespace acc::menus::modsettings {

namespace {

// Link-row targets: the original author's Ko-fi page (README.md's support
// section) and this fork's changelog.
constexpr const char* kSupportModderUrl = "https://ko-fi.com/jeanstiletto";
constexpr const char* kLatestChangesUrl =
    "https://github.com/aradix85/voice-of-the-old-republic/blob/bridge/docs/CHANGELOG.md";

// The README of this fork (branch bridge). Same as Config.ReadmeUrl in the
// installer.
constexpr const char* kReadmeUrl =
    "https://github.com/aradix85/voice-of-the-old-republic/blob/bridge/README.md";

// Cue-volume slider step (percent per Left/Right press) and the cue used
// for the audible preview. The preview rides priority group 0xb so it
// survives the in-game Options pause (SetSoundMode mutes everything else),
// and plays at the slider-scaled per-source volume so the user hears the
// level they just dialled in. The swoop race's shift chirp is short and
// clean.
// The only slider is the cue volume; it steps 10 percent per press.
constexpr int kSliderStep               = 10;
constexpr int kCueVolumePreviewGroup    = 0x0b;
constexpr acc::audio::NavCue kCueVolumePreviewCue = acc::audio::NavCue::SwoopShiftReady;

// Hand `url` to the default browser. Returns false if the shell refused
// (no browser association, blocked protocol handler) so the caller can
// speak the failure instead of leaving the user waiting for a page that
// never came.
//
// Deliberately nothing beyond the handoff. The first cut also minimised
// the game window (ShowWindow SW_MINIMIZE) on the theory that a fullscreen
// game would bury the browser; in testing the browser came up fine on its
// own, and the forced minimise made alt-tabbing BACK into the game
// misbehave badly. Letting the shell do the window management is both
// simpler and what actually works — do not reintroduce the minimise.
bool OpenExternalUrl(const char* url) {
    HINSTANCE rc = ShellExecuteA(nullptr, "open", url, nullptr, nullptr,
                                 SW_SHOWNORMAL);
    // ShellExecute's success contract is "> 32"; anything at or below is an
    // error code in the return value itself.
    const bool ok = reinterpret_cast<INT_PTR>(rc) > 32;
    acclog::Write("ModSettings", "open url \"%s\" rc=%d ok=%d",
                  url, static_cast<int>(reinterpret_cast<INT_PTR>(rc)),
                  ok ? 1 : 0);
    return ok;
}

// Enter on a Link row: open the URL, then speak the outcome. The submenu
// stays open, so the user is still on the same row when they come back
// from the browser.
void ActivateLinkRow(Option opt) {
    const char* url = kLatestChangesUrl;
    if (opt == Option::SupportModder) {
        url = kSupportModderUrl;
    } else if (opt == Option::OpenReadme) {
        url = kReadmeUrl;
    }
    const bool ok = OpenExternalUrl(url);
    acc::speech::Speak(
        acc::strings::Get(ok ? acc::strings::Id::ModSettingLinkOpened
                             : acc::strings::Id::ModSettingLinkFailed),
        /*interrupt=*/true);
}

// Adjust the cue-volume slider by delta (clamped to 0..100), re-announce it,
// and play a hint-sound cue at the new level on priority group 0xb (survives
// the in-game Options pause), reusing the glossary preview handle.
void AdjustSlider(int delta) {
    int next = acc::audio::GetGlobalCueVolumePercent() + delta;
    if (next < 0)   next = 0;
    if (next > 100) next = 100;
    acc::audio::SetGlobalCueVolumePercent(next);
    acclog::Write("ModSettings", "cue volume adjust delta=%+d -> %d", delta, next);
    SpeakFocusedOption();
    if (next > 0) {
        const Vector kCentre = { 0.0f, 0.0f, 0.0f };
        s_glossaryPreview.Start(
            acc::audio::GetNavCueResref(kCueVolumePreviewCue), kCentre,
            /*looping=*/false, /*spatial=*/false,
            /*priorityGroup=*/kCueVolumePreviewGroup,
            /*volumeByte=*/127 * next / 100);
    } else {
        s_glossaryPreview.Stop();  // muted - silence any in-flight tail
    }
}

}  // namespace

// Input for the option list.
bool HandleInputList(int keyCode) {
    // Left / Right: adjust the focused Slider row;
    // ignored on every other kind (they fall through to the parent-input
    // block in HandleInput, which consumes them so they don't leak to the
    // engine).
    if (keyCode == kInputNavLeft || keyCode == kInputNavRight) {
        const Option opt = FocusedOption();
        if (opt != Option::Count) {
            const RowKind kind = Spec(opt).kind;
            if (kind == RowKind::Slider) {
                AdjustSlider(keyCode == kInputNavRight ? kSliderStep : -kSliderStep);
                return true;
            }
        }
        return false;
    }
    // Up / Down / Home / End: step or jump focus with end-clamp (no wrap —
    // sighted "list box" semantics match Options panels above and below).
    // Home / End used to be swallowed by the block-everything switch in
    // HandleInput without moving anything, which read as a dead key.
    {
        const bool up   = (keyCode == kInputNavUp);
        const bool down = (keyCode == kInputNavDown);
        const bool home = (keyCode == kInputHome);
        const bool end  = (keyCode == kInputEnd);
        if (up || down || home || end) {
            s_focused = acc::menus::nav::ClampedCursorStep(
                s_focused, k_rootRowCount, up, down, home, end);
            SpeakFocusedOption();
            return true;
        }
    }
    // Enter: Toggle rows flip + re-announce; Submenu rows pivot to the
    // nested view; Link rows hand their URL to the browser.
    if (keyCode == kInputEnter1 || keyCode == kInputEnter2) {
        const Option opt = FocusedOption();
        if (opt == Option::Count) return true;
        const auto& row = Spec(opt);
        if (row.kind == RowKind::Submenu) {
            if (row.option == Option::AudioGlossary) {
                OpenGlossarySubMenu();
            } else if (row.option == Option::Keybindings) {
                s_keybindsOpen = true;
                acc::menus::keybinds::Open();
            }
            return true;
        }
        if (row.kind == RowKind::Link) {
            ActivateLinkRow(row.option);
            return true;
        }
        // Slider: Enter replays the preview at the current level (Left/
        // Right do the actual adjusting). delta=0 keeps the value put.
        if (row.kind == RowKind::Slider) {
            AdjustSlider(0);
            return true;
        }
        // SkipIntros: filesystem rename instead of s_toggles flip.
        // Speak "Name: state" first (matches other toggles' rhythm)
        // and then a side-effect cue clarifying "takes effect on next
        // launch". Failure path speaks the toggle-failed string.
        if (row.option == Option::SkipIntros) {
            auto before = acc::intro_skip::CurrentState();
            bool wantDisable =
                (before != acc::intro_skip::State::Disabled);
            bool ok = acc::intro_skip::SetDisabled(wantDisable);
            acclog::Write("ModSettings",
                "toggle SkipIntros: target=%s ok=%d",
                wantDisable ? "disabled" : "enabled", ok ? 1 : 0);
            SpeakFocusedOption();
            const char* followup = !ok
                ? acc::strings::Get(acc::strings::Id::ModSettingSkipIntrosToggleFailed)
                : (wantDisable
                    ? acc::strings::Get(acc::strings::Id::ModSettingSkipIntrosOnNextLaunch)
                    : acc::strings::Get(acc::strings::Id::ModSettingPlayIntrosOnNextLaunch));
            acc::speech::Speak(followup, /*interrupt=*/false);
            return true;
        }
        const int idx = static_cast<int>(opt);
        s_toggles[idx] = !s_toggles[idx];
        if (const char* key = PersistKey(opt)) {
            acc::settings::SetBool(key, s_toggles[idx]);
        }
        acclog::Write("ModSettings", "toggle idx=%d new=%d",
                      idx, s_toggles[idx] ? 1 : 0);
        SpeakFocusedOption();
        return true;
    }
    // Esc: back to the parent panel.
    if (keyCode == kInputEsc1 || keyCode == kInputEsc2) {
        Close();
        return true;
    }
    return false;
}

}  // namespace acc::menus::modsettings
