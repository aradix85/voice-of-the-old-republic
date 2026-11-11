// The mod-settings submenu: the toggle state, open and close, the virtual
// root entry on the Options panels, and the input router.

#include "menus_modsettings_internal.h"

#include "audio_bus.h"
#include "engine_input.h"
#include "engine_manager.h"
#include "engine_panels.h"
#include "intro_skip.h"
#include "log.h"
#include "menus_chain.h"
#include "menus_keybinds.h"
#include "menus_nav.h"
#include "mod_settings_store.h"
#include "speech.h"

#include <cstdio>

namespace acc::menus::modsettings {

// Defaults; the persisted toggles are read over them on first access. Only the
// Toggle rows use their slot, and SkipIntros' state is the filesystem.
bool s_toggles[static_cast<int>(Option::Count)] = {
    /* ExtendedCycling */ false,  // ON: , / . cycle the whole area, not only what was found
    /* RoomShapes      */ true,
    /* HumanSubtitles  */ false,  // humans and droids have voice-over that TTS would clash with
    /* QEAllAround     */ false,  // ON: Q/E cycle everything around, not only the camera view
    /* TurretAutoAim   */ false,  // ON: the turret locks onto the selected fighter
    /* SwoopAutopilot  */ false,  // ON: the swoop bike steers and shifts by itself
    /* SkipIntros      */ false,
    /* CueVolume       */ false,
    /* Keybindings     */ false,
    /* AudioGlossary   */ false,
    /* SupportModder   */ false,
    /* LatestChanges   */ false,
    /* OpenReadme      */ false,
};
int  s_focused      = 0;
bool s_keybindsOpen = false;

namespace {

// The chain entry's `control`: a unique pointer that is never dereferenced.
char s_rootSentinel = 0;

bool  s_open        = false;
void* s_parentPanel = nullptr;  // the Options panel to rebind on close
// The foreground panel when the submenu opened. In-game Options keeps the
// InGameMenu strip on top, so "foreground != parent" is normal there; only a
// change since opening (a modal pushed, the parent gone) closes the submenu.
void* s_fgAtOpen    = nullptr;

void EnsureTogglesLoaded() {
    static bool done = false;
    if (done) return;
    done = true;
    for (int i = 0; i < k_optionCount; ++i) {
        if (const char* key = k_options[i].persistKey) {
            s_toggles[i] = acc::settings::GetBool(key, s_toggles[i]);
        }
    }
}

const char* StateText(Option opt) {
    // SkipIntros reads the filesystem every time so outside renames show;
    // Unknown counts as off and the Enter handler reports a failed rename.
    const bool on = (opt == Option::SkipIntros)
        ? acc::intro_skip::CurrentState() == acc::intro_skip::State::Disabled
        : s_toggles[static_cast<int>(opt)];
    return acc::strings::Get(on ? Id::ModSettingStateOn : Id::ModSettingStateOff);
}

void* ForegroundPanel() {
    void* mgr = *reinterpret_cast<void**>(kAddrGuiManagerPtr);
    return mgr ? acc::engine::GetForegroundPanel(mgr) : nullptr;
}

bool ForegroundDivergedFromParent() {
    if (!s_parentPanel) return false;
    if (!acc::engine::IsPanelInManager(s_parentPanel)) return true;
    void* fg = ForegroundPanel();
    return fg && s_fgAtOpen && fg != s_fgAtOpen;
}

// Close and AutoCloseSilent differ only in what they say and the rebind.
void ResetState() {
    s_open    = false;
    s_focused = 0;
    ResetGlossary();
    if (s_keybindsOpen) {
        acc::menus::keybinds::Reset();
        s_keybindsOpen = false;
    }
    s_parentPanel = nullptr;
    s_fgAtOpen    = nullptr;
}

// A panel pushed in front of the parent announces itself and gets the chain,
// so this close says nothing and does not rebind.
void AutoCloseSilent() {
    acclog::Write("ModSettings", "auto-close (foreground diverged from parent=%p)",
                  s_parentPanel);
    ResetState();
}

}  // namespace

const OptionSpec& Spec(Option opt) {
    return k_options[static_cast<int>(opt)];
}

Option FocusedOption() {
    if (s_focused < 0 || s_focused >= k_rootRowCount) return Option::Count;
    return k_rootRows[s_focused];
}

// interrupt=false queues the row behind a title that was just spoken.
void SpeakFocusedOption(bool interrupt) {
    const Option opt = FocusedOption();
    if (opt == Option::Count) return;
    const auto& row = Spec(opt);
    const char* name = acc::strings::Get(row.label);
    char line[160];
    if (row.kind == RowKind::Toggle) {
        snprintf(line, sizeof(line), acc::strings::Get(Id::FmtModSettingOption),
                 name, StateText(opt));
    } else if (row.kind == RowKind::Slider) {
        snprintf(line, sizeof(line), acc::strings::Get(Id::FmtModSettingSlider),
                 name, acc::audio::GetGlobalCueVolumePercent());
    } else {
        snprintf(line, sizeof(line), "%s", name);
    }
    acc::speech::Speak(line, interrupt);
    acclog::Write("ModSettings", "speak focused idx=%d \"%s\"", s_focused, line);
}

bool StepListCursor(int keyCode, int count, int* cursor) {
    const bool up   = (keyCode == kInputNavUp);
    const bool down = (keyCode == kInputNavDown);
    const bool home = (keyCode == kInputHome);
    const bool end  = (keyCode == kInputEnd);
    if (!up && !down && !home && !end) return false;
    *cursor = acc::menus::nav::ClampedCursorStep(*cursor, count, up, down, home, end);
    return true;
}

bool IsRootAnchor(void* control) {
    return control == &s_rootSentinel;
}

void ForEachRootAnchor(void* panel,
                       bool (*callback)(void* sentinel, int sortCx, int sortCy,
                                        void* userData),
                       void* userData) {
    if (!panel || !callback) return;
    auto kind = acc::engine::IdentifyPanel(panel);
    if (kind != acc::engine::PanelKind::InGameOptions &&
        kind != acc::engine::PanelKind::MainMenuOptions) {
        return;
    }
    // The chain sorts by y: 9000 makes the entry the last stop, below Close and
    // Quit Game. x 180 is the column the engine paints the Options buttons in.
    callback(&s_rootSentinel, /*sortCx=*/180, /*sortCy=*/9000, userData);
}

bool ExtractRootLabel(char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize == 0) return false;
    const char* label = acc::strings::Get(Id::ModSettingsRootLabel);
    snprintf(outBuf, bufSize, "%s", label ? label : "");
    return outBuf[0] != '\0';
}

bool IsOpen() {
    return s_open;
}

void OpenSubMenu(void* parentPanel) {
    EnsureTogglesLoaded();
    s_open        = true;
    s_focused     = 0;
    s_parentPanel = parentPanel;
    s_fgAtOpen    = ForegroundPanel();
    acclog::Write("ModSettings", "submenu opened (parent=%p fgAtOpen=%p)",
                  parentPanel, s_fgAtOpen);
    acc::speech::Speak(acc::strings::Get(Id::ModSettingsOpened), /*interrupt=*/true);
    SpeakFocusedOption(/*interrupt=*/false);
}

void Close() {
    if (!s_open) return;
    acclog::Write("ModSettings", "submenu closed (parent=%p)", s_parentPanel);
    void* parent = s_parentPanel;
    ResetState();
    acc::speech::Speak(acc::strings::Get(Id::ModSettingsClosed), /*interrupt=*/true);
    // The next arrow walks the real options again.
    if (parent && acc::engine::IsPanelInManager(parent)) {
        acc::menus::chain::RebindChain(parent);
    }
}

bool HandleInput(int keyCode) {
    if (!s_open) return false;
    // A modal in front of the parent (Alt+F4's quit confirm, say) must get the
    // press, or the user is stranded in the submenu.
    if (ForegroundDivergedFromParent()) {
        AutoCloseSilent();
        return false;
    }

    bool handled;
    if (s_keybindsOpen) {
        handled = acc::menus::keybinds::HandleInput(keyCode);
        // Esc out of the configurator's top level: back on the Key bindings row.
        if (!acc::menus::keybinds::IsOpen()) {
            s_keybindsOpen = false;
            SpeakFocusedOption();
            return true;
        }
    } else if (s_glossaryOpen) {
        handled = HandleInputGlossary(keyCode);
    } else {
        handled = HandleInputList(keyCode);
    }
    if (handled) return true;

    // The other GUI keys must not reach the parent panel; anything else (the
    // global hotkeys) still passes.
    switch (keyCode) {
    case kInputNavLeft:
    case kInputNavRight:
    case kInputHome:
    case kInputEnd:
    case kInputActivate:
        return true;
    default:
        return false;
    }
}

void Tick() {
    if (s_keybindsOpen) acc::menus::keybinds::Tick();  // its key-capture poll
    TickGlossary();
}

bool GetToggle(Option option) {
    EnsureTogglesLoaded();
    const int idx = static_cast<int>(option);
    if (idx < 0 || idx >= k_optionCount) return false;
    if (k_options[idx].kind != RowKind::Toggle) return false;
    return s_toggles[idx];
}

}  // namespace acc::menus::modsettings
