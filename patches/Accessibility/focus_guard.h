// Diagnostics for the "menu loaded but unresponsive" / "intro movie plays
// twice" / "alt-tab needed to wake the engine" class of reports.
//
// StartFocusProbe spins up a background polling thread that periodically
// re-enumerates the process's top-level windows and subclasses every
// game window it hasn't seen before. Subclassed WindowProc logs every
// WM_ACTIVATE / WM_ACTIVATEAPP / WM_SETFOCUS / WM_KILLFOCUS with the
// source HWND + class name tag. Polls every 100ms so transient windows
// (SWMovieWindow recreated between intros) get caught.
//
// Idempotent — repeat calls are no-ops. Safe to call from OnRulesInit
// (early, before windows exist) and from MainMenu first-sight (late,
// belt-and-braces) without worrying about double-subclassing.

#pragma once

namespace acc::focus_guard {

// Start the focus-probe polling thread. Idempotent. First call spawns
// the worker; subsequent calls return immediately.
void StartFocusProbe();

// True when the foreground window belongs to our process — i.e. the game owns
// the foreground. Used to gate the cold-start input acquire: input must mirror
// foreground, so we only acquire the keyboard at main-menu first-sight if we
// actually own the foreground (acquiring while an overlay/other window is in
// front would bleed keys into the background game). Cheap; no allocation.
bool GameOwnsForeground();

// Drain a pending "input blocked by Steam Big Picture" warning and speak it
// (no-op if none queued). The focus-probe poll thread flags this when the user
// presses a key while Steam Big Picture Mode holds the foreground — in
// windowed mode those keystrokes go to Big Picture, not the game, so menus
// look dead. Edge- and cooldown-throttled by the poll thread. Call once per
// tick on the engine's main thread (speech path is COM-safe there).
void DrainInputBlockedWarning();

}  // namespace acc::focus_guard
