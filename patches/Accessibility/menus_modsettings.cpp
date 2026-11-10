// virtual mod-settings submenu.
//
// See menus_modsettings.h for the design summary. This TU owns the
// sentinel pointer, the toggle bits, and the input router.

#include "menus_modsettings_internal.h"

#include "audio_bus.h"        // global cue-volume slider get/set
#include "engine_input.h"
#include "engine_manager.h"  // IsPanelInManager for close-time rebind guard
#include "engine_panels.h"
#include "intro_skip.h"      // SkipIntros toggle — filesystem-backed state
#include "log.h"
#include "menus_chain.h"
#include "menus_keybinds.h"  // nested keybind configurator submenu
#include "mod_settings_store.h"  // persist toggles across launches
#include "speech.h"

namespace acc::menus::modsettings {

namespace {

// Sentinel: address-of a never-deref'd static byte. Casting to void*
// gives a stable, unique pointer that we can use as the chain entry's
// `control` field without ever pointing at an engine-allocated control.
char s_rootSentinel = 0;

// Pull persisted toggle values into s_toggles once, on first access. The
// scaffold defaults in s_toggles[] act as the fallback for absent keys, so
// behaviour on a fresh install (no file) is unchanged.
void EnsureTogglesLoaded() {
    static bool done = false;
    if (done) return;
    done = true;
    for (int i = 0; i < k_optionCount; ++i) {
        const char* key = PersistKey(k_options[i].option);
        if (!key) continue;
        s_toggles[i] = acc::settings::GetBool(key, s_toggles[i]);
    }
}

const char* StateText(Option opt) {
    // SkipIntros: state is the filesystem (biologo.bik vs .disabled),
    // not s_toggles. Read the canonical state every call so the UI
    // reflects external changes (installer apply between sessions,
    // manual user rename, etc.).
    if (opt == Option::SkipIntros) {
        auto st = acc::intro_skip::CurrentState();
        // Unknown maps to Off so the toggle UI still works — the Enter
        // handler will report failure if the rename can't go through.
        bool on = (st == acc::intro_skip::State::Disabled);
        return acc::strings::Get(on ? acc::strings::Id::ModSettingStateOn
                                    : acc::strings::Id::ModSettingStateOff);
    }
    bool on = s_toggles[static_cast<int>(opt)];
    return acc::strings::Get(on ? acc::strings::Id::ModSettingStateOn
                                : acc::strings::Id::ModSettingStateOff);
}

// True iff the engine pushed a new panel on top of whatever was
// foreground when the submenu opened — typically a MessageBoxModal
// raised by Alt+F4 / quit-confirm, but also any other engine modal
// the user hits while the submenu is logically focused. Also true if
// the saved parent panel itself disappeared (engine teardown after
// Quit Game, save reload, etc.).
//
// Baseline is the snapshot taken at OpenSubMenu time, NOT s_parentPanel:
// the In-Game Options flow keeps the InGameMenu strip pinned at
// panels[top] so `GetForegroundPanel != parent` is the steady state
// inside that panel. We only care about the foreground CHANGING after
// the user entered the submenu (modal push, parent dismissed, etc.).
bool ForegroundDivergedFromParent() {
    if (!s_parentPanel) return false;
    if (!acc::engine::IsPanelInManager(s_parentPanel)) return true;
    void* mgr = *reinterpret_cast<void**>(kAddrGuiManagerPtr);
    if (!mgr) return false;
    void* fg = acc::engine::GetForegroundPanel(mgr);
    if (!fg) return false;
    if (s_fgAtOpen == nullptr) return false;  // no baseline yet
    return fg != s_fgAtOpen;
}

}  // namespace

// Spec for `opt`. Callers must have ruled out Option::Count first.
const OptionSpec& Spec(Option opt) {
    return k_options[static_cast<int>(opt)];
}

// Option under the cursor, or Option::Count when the index has gone out of
// range.
Option FocusedOption() {
    if (s_focused < 0 || s_focused >= k_rootRowCount) return Option::Count;
    return k_rootRows[s_focused];
}

// Stable acc_settings.ini keys for the toggles that persist across launches.
// SkipIntros is filesystem-backed (the .bik rename IS its state), and
// CueVolume / AudioGlossary aren't toggles — those return nullptr (not
// persisted through this path; CueVolume persists via audio_bus).
const char* PersistKey(Option opt) {
    switch (opt) {
        case Option::ExtendedCycling: return "ExtendedCycling";
        case Option::RoomShapes:      return "RoomShapes";
        case Option::HumanSubtitles:  return "HumanSubtitles";
        case Option::QEAllAround:     return "QEAllAround";
        case Option::TurretAutoAim:   return "TurretAutoAim";
        default:                      return nullptr;  // not persisted here
    }
}

// Speak the current focused option. Toggle rows read as "Name: value";
// Submenu and Link rows read just "Name" (nothing to compose with).
// `interrupt` is false only on the two open paths, where the view's title
// has just been spoken and the row should queue behind it.
void SpeakFocusedOption(bool interrupt) {
    const Option opt = FocusedOption();
    if (opt == Option::Count) return;
    const auto& row = Spec(opt);
    const char* name = acc::strings::Get(row.label);
    char line[160];
    if (row.kind == RowKind::Toggle) {
        snprintf(line, sizeof(line),
                 acc::strings::Get(acc::strings::Id::FmtModSettingOption),
                 name, StateText(opt));
    } else if (row.kind == RowKind::Slider) {
        snprintf(line, sizeof(line),
                 acc::strings::Get(acc::strings::Id::FmtModSettingSlider),
                 name, acc::audio::GetGlobalCueVolumePercent());
    } else {
        snprintf(line, sizeof(line), "%s", name);
    }
    acc::speech::Speak(line, interrupt);
    acclog::Write("ModSettings", "speak focused idx=%d \"%s\"", s_focused, line);
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
    // sortCy = a very large synthetic y. The chain's insertion sort is
    // by cy ascending; placing the virtual entry at y=9000 lands it at
    // the end of the chain — below every real sub-screen button and
    // (on InGameOptions) below the bottom-row "Close" / "Quit
    // Game" buttons. User hears the existing options first on
    // Down-arrow, then "Mod settings" as the last stop. Top-of-chain
    // wrap (Home) still works.
    //
    // sortCx = 180 (matches the left-aligned column the engine paints
    // every other Options button in — observed in the chain dump at
    // patch-20260520-115529.log). Keeps the cursor warp on chain step
    // visually consistent for a sighted observer.
    callback(&s_rootSentinel, /*sortCx=*/180, /*sortCy=*/9000, userData);
}

bool ExtractRootLabel(char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize == 0) return false;
    const char* label =
        acc::strings::Get(acc::strings::Id::ModSettingsRootLabel);
    if (!label) {
        outBuf[0] = '\0';
        return false;
    }
    size_t i = 0;
    for (; i + 1 < bufSize && label[i]; ++i) outBuf[i] = label[i];
    outBuf[i] = '\0';
    return i > 0;
}

bool IsOpen() {
    return s_open;
}

void OpenSubMenu(void* parentPanel) {
    EnsureTogglesLoaded();
    s_open         = true;
    s_focused      = 0;
    s_parentPanel  = parentPanel;
    // Snapshot the engine-foreground panel right now. Subsequent
    // HandleInput calls compare against this baseline so we only
    // auto-close on a real foreground push (modal popup, parent
    // teardown) and not on the stable strip-on-top arrangement the
    // In-Game Options panel runs under.
    void* mgr = *reinterpret_cast<void**>(kAddrGuiManagerPtr);
    s_fgAtOpen = mgr ? acc::engine::GetForegroundPanel(mgr) : nullptr;
    acclog::Write("ModSettings",
                  "submenu opened (parent=%p fgAtOpen=%p)",
                  parentPanel, s_fgAtOpen);
    // Two-part open speech: "Mod settings opened" then the first
    // option. The title interrupts the chain-step tail; the option
    // queues right after.
    acc::speech::Speak(
        acc::strings::Get(acc::strings::Id::ModSettingsOpened),
        /*interrupt=*/true);
    SpeakFocusedOption(/*interrupt=*/false);
}

void Close() {
    if (!s_open) return;
    acclog::Write("ModSettings", "submenu closed (parent=%p)",
                  s_parentPanel);
    s_open            = false;
    s_focused         = 0;
    s_glossaryOpen    = false;
    s_glossaryFocused = 0;
    if (s_keybindsOpen) { acc::menus::keybinds::Reset(); s_keybindsOpen = false; }
    CancelPendingGlossaryCue();
    void* parent = s_parentPanel;
    s_parentPanel = nullptr;
    s_fgAtOpen    = nullptr;
    acc::speech::Speak(
        acc::strings::Get(acc::strings::Id::ModSettingsClosed),
        /*interrupt=*/true);
    // Rebind the chain on the parent panel so the next arrow press
    // walks the real options again. ValidateChainPanel-style guard:
    // skip the rebind if the parent has been freed under us (extremely
    // unlikely while the submenu is open, but cheap to check).
    if (parent && acc::engine::IsPanelInManager(parent)) {
        acc::menus::chain::RebindChain(parent);
    }
}

// Silent auto-close used when the engine pushes a new panel
// (MessageBoxModal etc.) in front of our parent. Unlike Close(), this
// path doesn't speak the "closed" cue — the surfacing panel will
// announce itself through the existing menu-monitor speech path, and
// we don't want to step on its cue. Also skips the parent-rebind
// because the chain rebuilder is going to fire anyway when the new
// panel becomes foreground.
void AutoCloseSilent() {
    if (!s_open) return;
    acclog::Write("ModSettings",
                  "auto-close (foreground diverged from parent=%p)",
                  s_parentPanel);
    s_open            = false;
    s_focused         = 0;
    s_glossaryOpen    = false;
    s_glossaryFocused = 0;
    if (s_keybindsOpen) { acc::menus::keybinds::Reset(); s_keybindsOpen = false; }
    CancelPendingGlossaryCue();
    s_parentPanel = nullptr;
    s_fgAtOpen    = nullptr;
}

bool HandleInput(int keyCode) {
    if (!s_open) return false;
    // Auto-close on foreground divergence — the engine pushed a new
    // panel (MessageBoxModal from Alt+F4 quit-confirm, etc.) in front
    // of our parent while we held input control. Releasing now lets
    // the press flow into the modal's own dispatcher; we'd otherwise
    // strand the user inside the submenu with no way to reach the
    // popup without Esc-closing us first. Verified scenario:
    // user-reported Alt+F4 popup unreachable until Esc.
    if (ForegroundDivergedFromParent()) {
        AutoCloseSilent();
        return false;
    }

    bool handled;
    if (s_keybindsOpen) {
        // Route to the keybind configurator. When the user Esc's out of its
        // top (category) level it flips IsOpen() to false — we then drop the
        // routing and re-announce the Key bindings row so the user is
        // anchored back at the Mod-settings root.
        handled = acc::menus::keybinds::HandleInput(keyCode);
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

    // Block every other GUI-key press from reaching the parent panel
    // while either submenu is logically focused. Without this, e.g.
    // Left / Right would dispatch through the chain's cycle-arrow path
    // and mutate state on the parent (sub-screen-button hover etc.)
    // while the user thinks they're inside Mod settings.
    switch (keyCode) {
    case kInputNavLeft:
    case kInputNavRight:
    case kInputHome:
    case kInputEnd:
    case kInputActivate:
        return true;
    default:
        // Unmapped scancodes (in-world hotkeys etc.) fall through. The
        // submenu is a UI-only state; we don't want to break the user's
        // ability to e.g. cycle volume bus or any other global hotkey
        // while it happens to be open.
        return false;
    }
}

void Tick() {
    // Drive the keybind configurator's capture poll while it's open. Cheap
    // (single early-out) when no capture is armed.
    if (s_keybindsOpen) acc::menus::keybinds::Tick();
    if (!s_pendingValid) return;
    DWORD now = GetTickCount();
    // GetTickCount wraps every ~49 days. Use signed-subtract semantics
    // so the comparison is wrap-safe.
    if (static_cast<int32_t>(now - s_pendingFireAt) < 0) return;
    acc::audio::NavCue cue = s_pendingCue;
    s_pendingValid = false;
    const char* resref = acc::audio::GetNavCueResref(cue);
    // Audition through a self-managed CExoSoundSource (non-looping, 2D)
    // rather than CExoSound::PlayOneShotSound. 2D + non-spatial so the
    // sample plays centred, matching how the main-menu glossary sounds —
    // the user is auditioning the SOUND, not a positioned cue, and may want
    // to hear e.g. "Wall" even with the wall-sound toggle off (no per-fire
    // IsCueEnabled gating here).
    //
    // Priority group 0xb is the lever that makes it audible in-game: the
    // in-game menu calls SetSoundMode(4) on open, whose PauseAllSounds pass
    // mutes every source EXCEPT priority groups 1/2/0xb. 0xb is the group
    // the engine's own GUI click sounds use (CSWGuiManager::LoadGuiSounds),
    // so the audition rides the same exemption and stays audible under the
    // pause. (The default group 0x17 is muted — that was the silent bug.)
    const Vector kCentre = { 0.0f, 0.0f, 0.0f };
    constexpr int kGuiPriorityGroup = 0xb;
    bool ok = s_glossaryPreview.Start(resref, kCentre,
                                      /*looping=*/false, /*spatial=*/false,
                                      /*priorityGroup=*/kGuiPriorityGroup);
    acclog::Write("ModSettings",
                  "glossary fire cue=%d resref=\"%s\" played=%d",
                  static_cast<int>(cue), resref, ok ? 1 : 0);
}

bool GetToggle(Option option) {
    EnsureTogglesLoaded();
    int idx = static_cast<int>(option);
    if (idx < 0 || idx >= k_optionCount) return false;
    // AudioGlossary is a submenu pivot, not a toggle. Return false
    // unconditionally so a misuse from a downstream consumer doesn't
    // read the unused s_toggles slot as meaningful state.
    if (k_options[idx].kind != RowKind::Toggle) return false;
    return s_toggles[idx];
}

}  // namespace acc::menus::modsettings
