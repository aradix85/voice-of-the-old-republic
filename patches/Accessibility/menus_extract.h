// Control text extraction (the announce ladder): FromControl.
//
// The cycle-category cache (used by the cycle-button announce path to
// recover the category prefix that the engine overwrites with the value
// on each activation) lives in this TU but is populated by
// the panel walk in menus_focus.cpp. Hence the public
// setter pair below — read happens internally in FromControl,
// write crosses the seam.
//
// Helpers used only by FromControl (FindSiblingLabel, IsCycleFlankerArrow,
// LookupCycleCategory) are file-statics in menus_extract.cpp. Helpers
// shared with chain navigation (IsChainNavigable, IsClassSelectionIcon,
// ClassLabelCache*) live in menus_internal*.cpp and cross via
// menus_internal.h.

#pragma once

#include <cstddef>

namespace acc::menus::extract {

// Pull the announceable text of a control (tooltip → button → label →
// labelhilight → slider → listbox → vtable-overrides → per-kind fallbacks
// → sibling label → cycle category prefix → toggle state suffix).
//
// Returns a static C-string tag identifying which step succeeded
// ("button", "label", "perkind-tlk", "siblinglabel-fallback", …) or
// nullptr if every path returned no text. Tags are diagnostic only —
// caller compares against nullptr, not against specific tag strings.
//
// `ownerPanel` is the panel this control belongs to. Caller passes it
// when known (chain code, panel-walk, monitor pollers); when null, the
// function resolves via FindOwningPanel + g_currentPanel fallback.
//
// `outBuf` must be at least 2 bytes; result is null-terminated.
const char* FromControl(void* control,
                        char* outBuf, size_t bufSize,
                        void* ownerPanel = nullptr);

// Cycle-category cache management. Cycle widgets render as
// `[◀] value [▶]`, with the engine rewriting the middle button's
// CExoString to the new value on each activation — losing the category
// name. We capture (control → category) at panel-walk time, BEFORE any
// activation has run, then prepend the cached category in FromControl.
//
// OnSetActiveControl drives the capture: clear the cache on first
// focus into a panel, then add one entry per cycle-display button it
// finds. Both functions are no-ops on overflow (cache is sized for
// realistic panel widget counts).
void ResetCycleCategoryCache();
void CaptureCycleCategory(void* control, const char* category);

// Resolve the equipped item's display name for one equip-screen slot
// button (matched by its .gui id against the slot table the per-kind
// announce uses). Reads the panel-cached slot handle and resolves it via
// the engine's universal name accessor. Returns false — with outBuf
// empty — when the slot is empty, the control is no known slot button,
// or a read faults. Used by the picker monitor's refusal follow-up.
bool ReadEquipSlotItemName(void* panel, void* slotBtn,
                           char* outBuf, size_t bufSize);

// Pazaak wager popup: register a single virtual top-of-chain "row" (live
// wager + maximum + credits) anchored on the maximum_label. No-op for any
// non-PazaakWager panel. Mirrors menus_credits::ForEachCreditsRowAnchor;
// the row's text is produced by FromControl's PazaakWager branch.
void ForEachWagerRowAnchor(void* panel,
                           bool (*callback)(void* labelControl, int sortCy,
                                            void* userData),
                           void* userData);

}  // namespace acc::menus::extract
