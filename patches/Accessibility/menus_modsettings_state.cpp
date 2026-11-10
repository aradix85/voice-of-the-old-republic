// The state the menus_modsettings files share, defined in one place; each item is
// described where it is declared, in menus_modsettings_internal.h.

#include "menus_modsettings_internal.h"

namespace acc::menus::modsettings {

bool s_toggles[static_cast<int>(Option::Count)] = {
    /* ExtendedCycling */ false,
    /* RoomShapes      */ true,
    /* HumanSubtitles  */ false,  // default off — humans AND droids have VO,
                                  // so TTS over the same line clashes (humans,
                                  // HK-47) or reads meaningless beep-text (T3).
                                  // Genuine aliens still read because the
                                  // suppression filter gates on appearance /
                                  // race (see IsSuppressibleSpeaker), not on
                                  // this toggle alone.
    /* QEAllAround */ false,      // default off. ON: Q/E cycle every object
                                  // around the player within the engine's
                                  // target range, clockwise from straight
                                  // ahead, instead of only what the camera
                                  // shows (qe_ring). Camera-independent.
    /* TurretAutoAim   */ false,  // "Autoaiming" — OFF by default. When ON, the
                                  // turret PERMANENTLY locks aim onto the
                                  // selected fighter (full auto-track), so the
                                  // player just selects + fires — for players
                                  // who want no challenge or have stronger
                                  // hearing impairments. The DEFAULT (toggle
                                  // off) is the always-on slight aim-magnetism
                                  // in minigame_turret.cpp, which keeps the minigame
                                  // skill-based but playable by ear.
    /* SkipIntros      */ false,  // unused — state lives in filesystem
                                  // (biologo.bik vs biologo.bik.disabled);
                                  // StateText + Enter both special-case
                                  // this index and call into intro_skip.
    /* CueVolume       */ false,  // unused — RowKind::Slider; value lives in
                                  // audio_bus (Get/SetGlobalCueVolumePercent).
    /* Keybindings     */ false,  // unused — RowKind::Submenu (configurator pivot)
    /* AudioGlossary   */ false,  // unused — RowKind::Submenu
    /* SupportModder   */ false,  // unused — RowKind::Link
    /* LatestChanges   */ false,  // unused — RowKind::Link
    /* OpenReadme      */ false,  // unused — RowKind::Link
};
bool   s_open            = false;
int    s_focused         = 0;
void*  s_parentPanel     = nullptr;
void*  s_fgAtOpen        = nullptr;
bool   s_glossaryOpen    = false;
int    s_glossaryFocused = 0;
bool   s_keybindsOpen    = false;
bool                s_pendingValid  = false;
DWORD               s_pendingFireAt = 0;
acc::audio::NavCue  s_pendingCue    = acc::audio::NavCue::SwoopShiftReady;
acc::audio::LoopSource s_glossaryPreview;

}  // namespace acc::menus::modsettings
