// Internal to the pad_input module (pad_input*.cpp); no other file includes
// it. The public interface is pad_input.h. Here: what the files share.

#pragma once

#include "pad_input.h"

#include <windows.h>
#include <cmath>
#include <cstdint>


namespace acc::pad {

// XInput's own button bits. Mirrored rather than included for the same reason
// the structures above are.
constexpr WORD kBtnDpadUp    = 0x0001;
constexpr WORD kBtnDpadDown  = 0x0002;
constexpr WORD kBtnDpadLeft  = 0x0004;
constexpr WORD kBtnDpadRight = 0x0008;
constexpr WORD kBtnStart     = 0x0010;
constexpr WORD kBtnBack      = 0x0020;
constexpr WORD kBtnThumbL    = 0x0040;
constexpr WORD kBtnThumbR    = 0x0080;
constexpr WORD kBtnShoulderL = 0x0100;
constexpr WORD kBtnShoulderR = 0x0200;
constexpr WORD kBtnA         = 0x1000;
constexpr WORD kBtnB         = 0x2000;
constexpr WORD kBtnX         = 0x4000;
constexpr WORD kBtnY         = 0x8000;

extern bool g_ltHeld;
extern bool g_rtHeld;

// A trigger does two jobs: alone it is a button, held it is a modifier for the
// shoulder next to it (and for the other trigger). Both cannot fire on the
// PRESS — LT+LB would then also switch the D-Pad mode on the way in. So each
// trigger's solo job fires on RELEASE, and any chord taken during the hold
// cancels it. There is no timing rule and nothing for the user to learn: LT
// switches the D-Pad, LT+LB speaks the route, and the two never collide.
extern bool g_ltChorded;
extern bool g_rtChorded;

// The three trigger jobs, claimed by Tick() and drained by PollTriggers().
// Tick() runs in OnUpdate, and LT now OPENS the unified action menu — which
// the menu's hard rule forbids from that context (arming from a tick made the
// engine synthesise a phantom confirm one tick later that fired the menu's
// first entry). So Tick() only decides that the job is owed; PollTriggers()
// pays it from the input-poll slot, the same one the keyboard's Shift+Enter
// and the pad's own A button open the menu from.
extern bool g_pendingLtSolo;
extern bool g_pendingRtSolo;
extern bool g_pendingHelp;

// ---------------------------------------------------------------------------
// Is the ENGINE's own gamepad path alive?
// ---------------------------------------------------------------------------
// KOTOR 2 ships gamepad support, but it enumerates game controllers exactly
// ONCE, at input init. A pad switched on a few seconds after the game starts is
// invisible to the engine for the rest of the session: its D-Pad, its face
// buttons, its shoulders, the stick that walks and the stick that turns the
// camera are all dead — while the mod's own trigger layer, read from XInput
// which rescans every 2 s, keeps working perfectly. Half a controller, and
// nothing in the game says which half or why.
//
// KOTOR 1 has no engine pad path at all, so the mod already IS the pad driver
// there. This is what lets KOTOR 2 fall back to that same driver when its
// engine's half turns out to be missing, and it is why "plug the pad in before
// launching" stops being a rule the player has to know.
//
// Deciding it without double-firing on a healthy session
// ------------------------------------------------------
// We cannot simply drive from XInput and see: on a session where the engine IS
// delivering, every press would then fire twice. And we cannot wait for
// evidence of absence, because absence produces no events to wait for.
//
// So the first D-Pad press of the session is the probe. XInput sees it at once;
// the engine, if it is alive, delivers its own event for the same press within
// a frame or two. Hold the press for a beat — if the engine's event arrives,
// its pad is live and we never ask again; if the beat passes in silence, its
// pad is dead, we dispatch the press ourselves and latch the fallback for every
// binding.
//
// The D-Pad is the probe because it is the one surface that reaches the manager
// hook from EVERY screen on a healthy KOTOR 2 (in the world it drives the
// engine's own action menu, in a menu it navigates), and because its codes are
// among the two event shapes NoteSeen can tell apart from a real keyboard
// press. The whole cost is one barely perceptible delay, once, on the first
// D-Pad press of a session whose engine pad is dead.
//
// Dead is not permanent. An engine pad event arriving after we latched Dead is
// proof the probe was wrong — a press swallowed by a loading screen would do
// it — and we flip back at once, because two dispatches of every press is a far
// worse failure than one delayed press.
enum class EnginePad { Unknown, Live, Dead };
extern EnginePad g_enginePad;

extern int       g_probeCode;  // D-Pad code the probe is holding; 0 = idle
extern int       g_probeSlot;  // its kDpadBits index
extern ULONGLONG g_probeDeadline;

// The button word, sampled by Tick() and edge-detected by PollButtons(). Two
// stages rather than one because they run at different points in the tick:
// Tick() has to be early (its trigger state is a modifier other consumers
// read), while the dispatch belongs next to the keyboard poll.
extern WORD g_btnNow;
extern WORD g_btnLast;

// Y, the peek modifier. Sampled as a STATE rather than tracked as an edge
// because that is the question peek_description asks of it.
extern bool g_yHeld;

// ---------------------------------------------------------------------------
// KOTOR 1 — making the ENGINE act
// ---------------------------------------------------------------------------
// Everything above this point is game-agnostic: it drives the MOD. What
// follows is the K1-only other half — the presses whose whole job is something
// only the engine can do (navigate one of its panels, cycle a target, switch
// party leader, pause). K2 gets those for free because the engine binds the
// pad itself; K1 has to be given the player's own key.
//
// The key goes in as a DirectInput scancode, which is all the engine's
// keyboard layer can see (a plain-VK SendInput is invisible to it). Every
// caller here is already behind the foreground gate in PollButtons.
//
// [Keymapping] action ids are keymap.2da row + 200. The defaults below are
// that file's own key for the row, used when the ini has no line for it — or
// when the bound key is one the mod refuses to inject (Caps Lock; see
// engine_keymap::InputIndexToScancode).
constexpr int kActSelectPrev = 204;   // row 4  SelectPrev   — default Q
constexpr int kActSelectNext = 205;   // row 5  SelectNext   — default E
constexpr int kActChangeChar = 206;   // row 6  ChangeChar   — default Tab
constexpr int kActOptions    = 216;   // row 16 Options      — default O
constexpr int kActFlourish   = 242;   // row 42 Flourish     — default X
constexpr int kActPrevMenu   = 243;   // row 43 PrevMenu     — default Q
constexpr int kActNextMenu   = 244;   // row 44 NextMenu     — default E
constexpr int kActPauseSpace = 241;   // row 41 Pause        — default Space

// DIK for those defaults, plus the three keys the pad plays as itself in a
// menu. The arrow cluster is behind the E0 prefix: DirectInput reports it as
// DIK_UP = 0xC8, but SendInput wants the BASE scancode with the extended flag,
// so 0x48 and `extended = true`.
constexpr int kDikQ      = 0x10;
constexpr int kDikE      = 0x12;
constexpr int kDikO      = 0x18;
constexpr int kDikX      = 0x2D;
constexpr int kDikTab    = 0x0F;
constexpr int kDikSpace  = 0x39;
constexpr int kDikReturn = 0x1C;
constexpr int kDikEscape = 0x01;
constexpr int kDikUp     = 0x48;   // extended
constexpr int kDikDown   = 0x50;   // extended
constexpr int kDikLeft   = 0x4B;   // extended
constexpr int kDikRight  = 0x4D;   // extended
constexpr int kDikHome   = 0x47;   // extended
constexpr int kDikEnd    = 0x4F;   // extended

extern ULONGLONG g_dpadHoldSince[4];
extern bool      g_dpadJumped[4];

// Helpers shared across the files, by the file that defines them.

// pad_input.cpp
void NoteSeen();
bool InWorldSurface();
void NoteNavPress();

// pad_input_bindings.cpp
bool RouteToOverlay(int logical);
void ToggleActionMenu();
bool ClaimQueueClearAll();
bool DispatchFaceChord(WORD button);
bool DispatchWorldDpad(int code);
bool DispatchMapDpad(int code);
bool DispatchShoulderChord(bool right);

// pad_input_dispatch.cpp
void FireGameAction(int actionId, int defaultDik, const char* what);
int LogicalForDirection(int code);
void DispatchDpad(int code);
void DispatchA();
void DispatchB();
void DispatchShoulder(bool right);
bool DpadHoldSpent(int code);
void DispatchDpadHold(int code);

}  // namespace acc::pad
