// Internal to the unified_action_menu module (unified_action_menu*.cpp); no other file includes
// it. The public interface is unified_action_menu.h. Here: what the files share.

#pragma once

#include "unified_action_menu.h"

#include <cstdio>
#include <cstring>

#include "engine_actionbar.h"   // personal block read + primitives
#include "engine_radial.h"      // target block read + primitives

namespace acc::unified_menu {

constexpr int kRowCount    = acc::engine_radial::kRowCount;     // 3 target rows
constexpr int kColumnCount = acc::engine_actionbar::kColumnCount; // 6 (K1 0..3, K2 0..5)
constexpr int kMaxCats     = kRowCount + kColumnCount;

enum class CatKind { Target, Personal };

struct Cat {
    CatKind kind;
    int     slot;   // Target: row 0..2 ; Personal: col 0..3
};

// Per-row / per-column selected-entry shadow, persisted across menu
// sessions AND across the engine's PopulateMenus rebuilds (which reassign
// action_ids). Kept in lock-step with the engine selection by pairing every
// index change with an ApplySelection() (SelectActionInRow / SelectVariant).
// Read by the bare-key announce path so 1..7 reports what the engine fires.
extern int g_targetSel[kRowCount];
extern int g_personalSel[kColumnCount];

// KOTOR 2 medical-item party picker — the keyboard's version of the
// portrait click that completes K2's two-step medical use (see
// PrimeMedicalPickK2). Modal over the menu while active.
struct MedPicker {
    bool active = false;
    int  count  = 0;              // populated rows (party members present)
    int  sel    = 0;              // focused row
    uint32_t handles[3] = {0, 0, 0};  // client handle per row (preselect
                                      // matching; Enter fallback resolve)
    int  slots[3]      = {0, 0, 0};   // engine party slot per row — Enter
                                      // re-resolves through the slot, not
                                      // the handle: the PC's client handle
                                      // is a floating pseudo-id the object
                                      // array won't look up (the "cannot
                                      // aim at the PC" round, 2026-08-12)
    char names[3][64]  = {"", "", ""};
    Cat  itemCat{};               // the Medicine-row entry to fire
    int  itemSel = 0;
    char itemLabel[128] = "";     // re-spoken when Esc backs out
};

struct State {
    bool     active         = false;
    bool     suspended      = false;// a blocking engine panel (MessageBox,
                                    // hotkey-opened sub-screen) sits over the
                                    // menu. We stay armed + keep our state +
                                    // keep our world pause, but stop owning
                                    // input so the panel handles its own keys.
                                    // On the panel's close we resume at the
                                    // same position (parity with native menus
                                    // restoring focus under a dismissed popup).
    Cat      cats[kMaxCats];
    int      catCount       = 0;
    int      curCat         = 0;
    uint32_t targetHandle   = 0;    // server/client handle; 0 = personal-only
    bool     creature       = false;// target is a hostile creature → named rows
    bool     hasTargetBlock = false;// include the 3 target rows in the menu?
    int      reqSlot        = 0;    // category the open shortcut asked for
                                    // (Shift+1..3 row / Shift+4..7 column).
                                    // Held in State, not a local: reading it
                                    // back after the engine-call chain returned
                                    // corrupted values (observed 2026-06-07 —
                                    // the local logged as a pointer/handle while
                                    // globals stayed intact, a stack imbalance
                                    // somewhere in the engine calls). Capturing
                                    // it before the first engine call sidesteps
                                    // the corrupted stack slot.
    char     targetName[64] = "";
    uint32_t unfoldDeclined = 0;    // last narrated handle whose mid-menu
                                    // target-block unfold found no populated
                                    // rows. Suppresses re-running the populate
                                    // chain for that same handle on every
                                    // subsequent keypress; any other narrated
                                    // handle retries. Reset on disarm.
    bool     pausedOnOpen   = false;// did we BeginOverlayPause when arming? Set
                                    // from the "Action Menu" auto-pause option
                                    // (CClientOptions bit 0x8000) at Arm time so
                                    // close/resume only touch the pause we own.
                                    // Off → menu runs over the live world, the
                                    // vanilla behaviour when that option is
                                    // unset (decompile-confirmed; see the
                                    // auto-pause note in action-menu-and-combat).
    MedPicker med;                  // K2 medical party picker sub-state
};
extern State g;

// Helpers shared across the files, by the file that defines them.

// unified_action_menu.cpp
int ClampInt(int v, int lo, int hi);
bool ForegroundPanelBlocks();
int CountForCat(void* tam, void* mi, const Cat& c);
int& ShadowFor(const Cat& c);
void ApplySelection(void* tam, void* mi, const Cat& c, int idx);
void ReadLabel(void* tam, void* mi, const Cat& c, int idx, char* out, size_t n);
uint32_t ReadActionId(void* tam, void* mi, const Cat& c, int idx);
bool Dispatch(void* tam, void* mi, const Cat& c);
const char* CategoryName(const Cat& c);
void BuildCategoryList(void* tam, void* mi);
int LocateCat(CatKind kind, int slot);
int FirstPopulatedTargetRow(void* tam);
bool DetectCreature(void* tam, uint32_t handle);
uint32_t ResolveNarratedServerHandle();
void Arm();

// unified_action_menu_speech.cpp
void SpeakCategory(void* tam, void* mi, const char* prefix);
void SpeakEntry(void* tam, void* mi);
void DescribeSelectedEntry(void* tam, void* mi, const Cat& cur, int sel);

// unified_action_menu_open.cpp
void CloseFromEsc();
bool ReanchorToNewTarget(void* tam, uint32_t narrated, bool rowCarried[kRowCount]);
bool UnfoldTargetBlock(void* tam, uint32_t narrated, bool rowCarried[kRowCount]);
void MaybeRestoreDrainedRows(void*& tam, bool targetChanged);

// unified_action_menu_fire.cpp
void ApplyPostFireClosePolicy();
bool HandleEnter(void* tam, void* mi, const Cat& cur, int sel, bool targetChanged, bool selectionCarried);

// unified_action_menu_medpicker.cpp
bool FireMedicalDirect(void* mi, const Cat& c, int sel, void* targetCre, const char* how);
bool OpenMedPicker(void* mi, const Cat& c, int sel, const char* itemLabel);
bool HandleMedPickerInput(int code);

}  // namespace acc::unified_menu
