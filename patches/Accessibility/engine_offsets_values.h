// Engine constants that are neither addresses nor struct field offsets: vtable
// slot indices, TLK strrefs, object-handle sentinels, enum bytes and
// panel-internal input codes.
//
// Part of the engine_offsets.h family - see engine_offsets_types.h for the
// split rationale and the full file list.
//
// None of these are in the upstream AddressDatabase, and that is the point.
// Strrefs and .gui control IDs are resource-derived: they come from dialog.tlk
// and the .gui layout files, so they vary with the game's content rather than
// with its executable - a re-pack of the same build can move them while every
// address stays put, and vice versa. On a KOTOR 2 port they need re-deriving
// from K2's own resources, independently of any address or offset work.

#pragma once

#include <cstdint>

// GuiControlMethods vtable indices for RTTI-style downcasts.
//
// Each accessor returns the same `this` cast to the concrete subclass, or
// nullptr if the control isn't of that subclass. They are trivial
// implementations (no engine state mutation, no allocation), so calling them
// from inside our hook is safe.
//
// Verified against the SARIF GuiControlMethods struct (offset 80/84/88/92).
constexpr int kVtableAsLabel        = 20;
constexpr int kVtableAsLabelHilight = 21;
constexpr int kVtableAsButton       = 22;
constexpr int kVtableAsButtonToggle = 23;

// The engine's reusable "close/back" caption. Every standalone dismiss
// button across the sub-screen .gui files — BTN_EXIT (abilities, character,
// inventory, journal, map, messages, optionsingame), BTN_BACK (equip,
// questitem, upgrade*, all options sub-tabs, titlemovie), BTN_CANCEL
// (container) and BTN_Cancel (store) — points its CAPTION at this single
// strref (verified by dumping CONTROLS across all 21 panels with gffdump).
// It renders as "Close" in the English game, so a
// match on the engine's *resolved* text for this strref is fully
// locale-agnostic. Confirm/Yes-No popups deliberately use 1580 (OK/Yes) and
// 1581 (Cancel/No), NOT 1582, so matching 1582 never strips the only
// actionable button from a choice dialog. Used by RebindChain's
// isDecorative filter to drop the redundant close button (Esc already
// dismisses every such panel via HandleEsc → FindCancelButton/Close).
constexpr uint32_t kCloseButtonStrRef = 1582;

// Engine sentinel for "no object" in AI-queue object-id slots. Distinct
// from CGameObjectArray's removed-handle sentinel (0xFFFFFFFF).
constexpr uint32_t kInvalidObjectId = 0x7F000000u;

// A TLK strref that names no string.
constexpr uint32_t kNoStrref = 0xFFFFFFFFu;

// Row of Security in skills.2da, the same in both games. The door code
// passes it to GetCanUseSkill as the literal 6.
constexpr int kSkillSecurity = 6;

// Inferred action_type byte mapping. The engine's AddX adders on
// CSWSCombatRound are declared in this order; matches typical enum-by-
// declaration patterns. Validate via DumpBytes on each AddX path.
constexpr unsigned char kActionTypeAttack          = 0;
constexpr unsigned char kActionTypeSpellCast       = 1;
constexpr unsigned char kActionTypeItemCast        = 2;
constexpr unsigned char kActionTypeEquip           = 3;
constexpr unsigned char kActionTypeUnequip         = 4;
constexpr unsigned char kActionTypeMove            = 5;
constexpr unsigned char kActionTypeUseTalent       = 6;
constexpr unsigned char kActionTypeHeal            = 7;
constexpr unsigned char kActionTypeCutscene        = 8;

// CSWGuiInGameAbilities panel-internal input codes, consumed by that panel's
// own HandleInputEvent (engine_offsets_addresses.h). These are panel-internal
// codes, distinct from the manager kInput* codes.
constexpr int       kAbilitiesPanelCodeCycleTab       = 0x29;
// Chart-nav codes consumed by HandleInputEvent on the Feats/Powers tabs and
// routed to CSWGuiSkillFlowChart::HandleInput (@0x006cdd80): 0x31/0x32 step the
// feat-chain rows (up/down, skipping empty cells) and trigger OnEnterFeat/
// OnEnterPower to repaint the detail + description.
// 0x2f/0x30 step columns (ranks within a chain). We do NOT use them: every
// column code path in HandleInputEvent calls PlayGuiSound first, so walking to
// the highest owned rank would fire up to two spurious clicks per row move.
// menus_abilities drives the column via SetSelectedSkill + OnEnterFeat/
// OnEnterPower instead, which is silent and lands in one step.
constexpr int       kAbilitiesPanelCodeChartUp        = 0x31;
constexpr int       kAbilitiesPanelCodeChartDown      = 0x32;

// The minigame object array has 255 slots, and CSWMiniGameObject's downcasts
// are at these vtable slots, for CallDowncast (byte offsets 0x1c and 0x20;
// kMgoArrayObjectsOffset has the witnesses).
constexpr int kMgoArraySlotCount    = 255;
constexpr int kVtableSlotAsEnemy    = 7;
constexpr int kVtableSlotAsObstacle = 8;

// HandleInputEvent's vtable slot on every GUI class, control and panel, in
// both games.
constexpr int kVtableSlotHandleInputEvent = 15;

// The GetPosition thunk of a track follower's model, a vtable byte offset
// (kTrackFollowerModelsDataOffset has the witnesses).
constexpr size_t kModelVtableSlotGetPosition = 0x64;

// Gob (render object) vtable slots, identical in both games — the RTTI slot map
// (docs/llm-docs/re/k2/k2-vtable-slots.csv, class "Gob") pairs them exactly:
//   slot  3 GetModelName()                       K1 0x0043F1C0 / K2 0x004615D0
//   slot 59 GetBoundingBox(Vector* mn, Vector* mx) K1 0x0043E8C0 / K2 0x00459FB0
//           (copies the member box — the model header's, MODEL-LOCAL, so the
//           world box is gob position + local box for an unrotated object)
//   slot 67 GetPart(const char* name) -> Part*   K1 0x004402F0 / K2 0x004587F0
//           ("root" is the root part; anything else is FindPart by name)
// Part vtable slot 29 GetMinimumBoundingBox(Vector* mn, Vector* mx, float scale)
//           K1 0x00440960 / K2 0x0045C810 — walks the part's trimesh vertices
//           and folds them into the CALLER-INITIALISED min/max, in the gob's
//           local frame (it subtracts the gob position and un-rotates).
// K1's CSWTrackFollower::DoObstacleHitCheck asks the obstacle's gob for its
// "hitbump" part and hit-tests against that (the string is literal in the
// decompile), so the part box is the true collision extent; the K2 swoop
// tracks' obstacle models carry exactly one such mesh (offline atlas,
// docs/llm-docs/swoop-k2-controls-and-jump.md section 7).
constexpr size_t kGobVtableSlotGetBoundingBox = 59 * sizeof(void*);
constexpr size_t kGobVtableSlotGetPart        = 67 * sizeof(void*);
constexpr size_t kPartVtableSlotGetMinBBox    = 29 * sizeof(void*);
