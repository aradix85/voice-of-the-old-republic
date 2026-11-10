// Input on the option list: toggles, the cue-volume slider, the submenu pivots
// and the link rows.

#include "menus_modsettings_internal.h"

#include "audio_bus.h"
#include "audio_cues.h"
#include "engine_input.h"
#include "intro_skip.h"
#include "log.h"
#include "menus_keybinds.h"
#include "mod_settings_store.h"
#include "speech.h"

#include <windows.h>
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")

namespace acc::menus::modsettings {

namespace {

constexpr const char* kSupportModderUrl = "https://ko-fi.com/jeanstiletto";  // the original author
constexpr const char* kLatestChangesUrl =
    "https://github.com/aradix85/voice-of-the-old-republic/blob/bridge/docs/CHANGELOG.md";
// Same as Config.ReadmeUrl in the installer.
constexpr const char* kReadmeUrl =
    "https://github.com/aradix85/voice-of-the-old-republic/blob/bridge/README.md";

constexpr int kSliderStep = 10;  // percent per Left/Right press
constexpr acc::audio::NavCue kCueVolumePreviewCue = acc::audio::NavCue::SwoopShiftReady;

// Hand `url` to the default browser; false when the shell refused. Nothing
// more: also minimising the game broke alt-tabbing back into it.
bool OpenExternalUrl(const char* url) {
    HINSTANCE rc = ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
    const bool ok = reinterpret_cast<INT_PTR>(rc) > 32;  // ShellExecute's success contract
    acclog::Write("ModSettings", "open url \"%s\" rc=%d ok=%d",
                  url, static_cast<int>(reinterpret_cast<INT_PTR>(rc)), ok ? 1 : 0);
    return ok;
}

// The submenu stays open, so the user is on the same row after the browser.
void ActivateLinkRow(Option opt) {
    const char* url = (opt == Option::SupportModder) ? kSupportModderUrl
                    : (opt == Option::OpenReadme)    ? kReadmeUrl
                                                     : kLatestChangesUrl;
    const bool ok = OpenExternalUrl(url);
    acc::speech::Speak(acc::strings::Get(ok ? Id::ModSettingLinkOpened
                                            : Id::ModSettingLinkFailed),
                       /*interrupt=*/true);
}

// Step the cue volume (clamped to 0..100), say it, and preview the new level.
void AdjustSlider(int delta) {
    int next = acc::audio::GetGlobalCueVolumePercent() + delta;
    if (next < 0)   next = 0;
    if (next > 100) next = 100;
    acc::audio::SetGlobalCueVolumePercent(next);
    acclog::Write("ModSettings", "cue volume adjust delta=%+d -> %d", delta, next);
    SpeakFocusedOption();
    if (next > 0) {
        const Vector kCentre = { 0.0f, 0.0f, 0.0f };
        s_preview.Start(acc::audio::GetNavCueResref(kCueVolumePreviewCue), kCentre,
                        /*looping=*/false, /*spatial=*/false,
                        /*priorityGroup=*/kGuiPriorityGroup,
                        /*volumeByte=*/127 * next / 100);
    } else {
        s_preview.Stop();
    }
}

// SkipIntros renames the intro movie instead of flipping a bit; the change
// applies on the next launch.
void ToggleSkipIntros() {
    const bool wantDisable =
        acc::intro_skip::CurrentState() != acc::intro_skip::State::Disabled;
    const bool ok = acc::intro_skip::SetDisabled(wantDisable);
    acclog::Write("ModSettings", "toggle SkipIntros: target=%s ok=%d",
                  wantDisable ? "disabled" : "enabled", ok ? 1 : 0);
    SpeakFocusedOption();
    const Id followup = !ok         ? Id::ModSettingSkipIntrosToggleFailed
                      : wantDisable ? Id::ModSettingSkipIntrosOnNextLaunch
                                    : Id::ModSettingPlayIntrosOnNextLaunch;
    acc::speech::Speak(acc::strings::Get(followup), /*interrupt=*/false);
}

void FlipToggle(Option opt) {
    const int idx = static_cast<int>(opt);
    s_toggles[idx] = !s_toggles[idx];
    if (const char* key = Spec(opt).persistKey) {
        acc::settings::SetBool(key, s_toggles[idx]);
    }
    acclog::Write("ModSettings", "toggle idx=%d new=%d", idx, s_toggles[idx] ? 1 : 0);
    SpeakFocusedOption();
}

void Activate(Option opt) {
    switch (Spec(opt).kind) {
    case RowKind::Submenu:
        if (opt == Option::AudioGlossary) {
            OpenGlossarySubMenu();
        } else if (opt == Option::Keybindings) {
            s_keybindsOpen = true;
            acc::menus::keybinds::Open();
        }
        return;
    case RowKind::Link:
        ActivateLinkRow(opt);
        return;
    case RowKind::Slider:
        AdjustSlider(0);  // Enter replays the preview at the current level
        return;
    case RowKind::Toggle:
        if (opt == Option::SkipIntros) {
            ToggleSkipIntros();
        } else {
            FlipToggle(opt);
        }
        return;
    }
}

}  // namespace

bool HandleInputList(int keyCode) {
    // Left/Right adjust the slider; on other rows HandleInput swallows them.
    if (keyCode == kInputNavLeft || keyCode == kInputNavRight) {
        const Option opt = FocusedOption();
        if (opt == Option::Count || Spec(opt).kind != RowKind::Slider) return false;
        AdjustSlider(keyCode == kInputNavRight ? kSliderStep : -kSliderStep);
        return true;
    }
    if (StepListCursor(keyCode, k_rootRowCount, &s_focused)) {
        SpeakFocusedOption();
        return true;
    }
    if (keyCode == kInputEnter1 || keyCode == kInputEnter2) {
        const Option opt = FocusedOption();
        if (opt != Option::Count) Activate(opt);
        return true;
    }
    if (keyCode == kInputEsc1 || keyCode == kInputEsc2) {
        Close();
        return true;
    }
    return false;
}

}  // namespace acc::menus::modsettings
