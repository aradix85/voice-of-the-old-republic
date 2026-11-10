// The unified action menu: one keyboard menu over the engine's target rows (Shift+1..3) and personal columns (Shift+4..7). Here: its categories and selection, arming and disarming, and the suspend while an engine panel sits on top.

#include "unified_action_menu_internal.h"

#include "engine_area.h"        // ResolveServerObjectHandle, kInvalidObjectId
#include "engine_offsets.h"      // kInvalidObjectId
#include "engine_options.h"     // GetActionMenuAutoPause (vanilla pause parity)
#include "engine_panels.h"      // IsForegroundUiBlocking (arm-time panel gate)
#include "engine_subscreen.h"   // Begin/EndOverlayPause
#include "log.h"
#include "narrated_target.h"
#include "strings.h"

namespace acc::unified_menu {

namespace {

// Decide whether the target block's three rows are the hostile-creature
// Attacks / Force-Powers / Items layout (→ named categories) vs a
// door/placeable/trigger's per-object actions (→ announce by label).
//
// We do NOT rely solely on resolving the target's client handle: an
// extended-cycled FAR target often isn't in the client object array, so
// the vtable downcast (IsCreatureClientTarget) returns false even for a
// real creature (observed 2026-06-07, "mangled menu" bug). The robust
// signal is the action content itself — only hostile-creature rows carry
// the tagged action_ids (force power 0x1000…, feat 0x2000…, item 0x4000…);
// doors/placeables expose only small interface-action ids (0x3ea, 0x3f2…,
// 0x404). So: tagged action present in any row ⇒ creature layout.
bool TargetRowsLookHostileCreature(void* tam) {
    if (!tam) return false;
    for (int r = 0; r < kRowCount; ++r) {
        if (acc::engine_radial::RowActionCount(tam, r) <= 0) continue;
        if (acc::engine_radial::ReadSelectedRowActionId(tam, r) >= 0x10000000u) {
            return true;
        }
    }
    return false;
}

// ---- arm / disarm --------------------------------------------------------

// Vanilla parity: the radial / personal action menus auto-pause the world only
// when the "Action Menu" auto-pause option is on (CClientOptions bit 0x8000) —
// off by default. Confirmed by decompiling OnTargetUpArrowPressed /
// OnActionUpArrowPressed, which gate their SetAutoPaused(1,7) call on that bit.
// On read failure we default to NOT pausing: matches the vanilla out-of-box
// default and never freezes the world for a user who didn't opt in.
bool ActionMenuAutoPauseEnabled() {
    bool on = false;
    return acc::engine::GetActionMenuAutoPause(on) && on;
}

}  // namespace

int ClampInt(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// True when a real engine panel / modal / dialog owns the foreground. Our
// menu has no GUI panel of its own, so it must never arm over a blocking
// panel: the panel owns input + its own pause, and a menu armed underneath
// would double-consume the shared nav keys (the quit-confirm double-speak
// in patch-20260609-111933.log). The matching auto-disarm — for a blocker
// that appears AFTER we armed (e.g. a hotkey-opened sub-screen) — lives in
// interact_hotkey's per-tick poll. Broader than the old
// HasActiveDialogPanel-only gate: also covers MessageBox, TutorialBox,
// Container / Store, and hotkey-opened sub-screens (the InGameMenu strip
// stays foreground while any sub-screen is drilled).
bool ForegroundPanelBlocks() {
    return acc::engine::IsForegroundUiBlocking();
}

// ---- per-kind primitive dispatch -----------------------------------------

int CountForCat(void* tam, void* mi, const Cat& c) {
    return c.kind == CatKind::Target
        ? acc::engine_radial::RowActionCount(tam, c.slot)
        : acc::engine_actionbar::VariantCount(mi, c.slot);
}

int& ShadowFor(const Cat& c) {
    return c.kind == CatKind::Target ? g_targetSel[c.slot]
                                     : g_personalSel[c.slot];
}

// Stamp the engine's per-slot selection to `idx` so reads + dispatch land on
// the user's chosen entry (target: field1 via SelectActionInRow; personal:
// the +0x1bac selected-action-id via SelectVariant).
void ApplySelection(void* tam, void* mi, const Cat& c, int idx) {
    if (c.kind == CatKind::Target) {
        (void)acc::engine_radial::SelectActionInRow(tam, c.slot, idx);
    } else {
        (void)acc::engine_actionbar::SelectVariant(mi, c.slot, idx);
    }
}

// Read the label of the selected entry. Target reads via the field1-selected
// descriptor (ApplySelection must run first); personal reads by explicit idx.
void ReadLabel(void* tam, void* mi, const Cat& c, int idx,
               char* out, size_t n) {
    out[0] = '\0';
    if (c.kind == CatKind::Target) {
        acc::engine_radial::ReadRowActionLabel(tam, c.slot, out, n);
    } else {
        acc::engine_actionbar::ReadVariantLabel(mi, c.slot, idx, out, n);
    }
}

uint32_t ReadActionId(void* tam, void* mi, const Cat& c, int idx) {
    return c.kind == CatKind::Target
        ? acc::engine_radial::ReadSelectedRowActionId(tam, c.slot)
        : acc::engine_actionbar::ReadVariantActionId(mi, c.slot, idx);
}

bool Dispatch(void* tam, void* mi, const Cat& c) {
    return c.kind == CatKind::Target
        ? acc::engine_radial::DispatchRowAction(tam, c.slot)
        : acc::engine_actionbar::FireSelectedVariant(mi, c.slot);
}

// Localised category name, or nullptr for an unnamed category (target rows
// on a non-creature target — door / placeable / trigger — whose rows carry
// per-object actions like Security / Bash and are announced by label).
const char* CategoryName(const Cat& c) {
    using S = acc::strings::Id;
    if (c.kind == CatKind::Personal) {
        switch (c.slot) {
            case 0: return acc::strings::Get(S::MenuCatSelfPowers);
            case 1: return acc::strings::Get(S::MenuCatMedical);
            case 2: return acc::strings::Get(S::MenuCatMisc);
            case 3: return acc::strings::Get(S::MenuCatExplosives);
            // KOTOR 2 only. Its main interface fills a fifth column the first
            // game does not have — the party-AI combat stance, Aggressive and
            // three siblings (confirmed live: UnifiedMenu.cols read
            // [4]=4 with "Aggressive" at index 0). KOTOR 1 never populates it,
            // so BuildCategoryList drops it there and this name is unreachable.
            case 4: return acc::strings::Get(S::MenuCatCombatBehaviour);
            // KOTOR 2 only, and only once the character has learned a form:
            // the lightsaber and Force forms (decompile-confirmed — K2's
            // GetPersonalActions fills slot 5 from the spell rows 258..268
            // alone). Empty for everyone else, so BuildCategoryList drops it.
            case 5: return acc::strings::Get(S::MenuCatForms);
            default: return nullptr;
        }
    }
    if (!g.creature) return nullptr;  // door/placeable target → label only
    switch (c.slot) {
        case 0: return acc::strings::Get(S::MenuCatAttacks);
        case 1: return acc::strings::Get(S::MenuCatForcePowers);
        case 2: return acc::strings::Get(S::MenuCatItems);
        default: return nullptr;
    }
}

// ---- category list -------------------------------------------------------

void BuildCategoryList(void* tam, void* mi) {
    g.catCount = 0;
    if (tam) {
        for (int r = 0; r < kRowCount && g.catCount < kMaxCats; ++r) {
            if (acc::engine_radial::RowActionCount(tam, r) > 0) {
                g.cats[g.catCount++] = {CatKind::Target, r};
            }
        }
    }
    if (mi) {
        int n[kColumnCount] = {0};
        for (int col = 0; col < kColumnCount && g.catCount < kMaxCats; ++col) {
            n[col] = acc::engine_actionbar::VariantCount(mi, col);
            if (n[col] > 0) {
                g.cats[g.catCount++] = {CatKind::Personal, col};
            }
        }
        // Column census, deduped. KOTOR 1 populates four personal columns and
        // binds them to keys 4..7; KOTOR 2 adds column 4 (combat behaviour,
        // key 8) and column 5 (forms, key 9 — empty until a form is learned).
        // Trace collapses it to one line per distinct shape.
        acclog::Trace("UnifiedMenu.cols", "personal variant counts "
                      "[0]=%d [1]=%d [2]=%d [3]=%d [4]=%d [5]=%d",
                      n[0], n[1], n[2], n[3], n[4], n[5]);
    }
}

int LocateCat(CatKind kind, int slot) {
    for (int i = 0; i < g.catCount; ++i) {
        if (g.cats[i].kind == kind && g.cats[i].slot == slot) return i;
    }
    return -1;
}

int FirstPopulatedTargetRow(void* tam) {
    for (int r = 0; r < kRowCount; ++r) {
        if (acc::engine_radial::RowActionCount(tam, r) > 0) return r;
    }
    return -1;
}

// Combined creature signal: trust the clean vtable downcast for near
// targets, fall back to action-content for far / unresolvable ones.
bool DetectCreature(void* tam, uint32_t handle) {
    return acc::engine_radial::IsCreatureClientTarget(handle) ||
           TargetRowsLookHostileCreature(tam);
}

// ---- target resolution ---------------------------------------------------

// Server-side handle of the currently narrated target, or 0 if none / map
// pin / stale. Mirrors target_action_menu's old resolver.
uint32_t ResolveNarratedServerHandle() {
    acc::narrated_target::Slot slot{};
    if (!acc::narrated_target::TryGet(slot)) return 0;
    if (slot.isMapPin) return 0;
    if (slot.handle == 0u || slot.handle == kInvalidObjectId) return 0;
    if (!acc::engine::ResolveServerObjectHandle(slot.handle)) return 0;
    return slot.handle;
}

void Arm() {
    g.suspended = false;
    if (!g.active) {
        g.active = true;
        // One rule for every opener: the player's "Action Menu" auto-pause
        // option decides. Pausing is what lets the user stack several actions
        // into one round (ApplyPostFireClosePolicy's stack mode).
        g.pausedOnOpen = ActionMenuAutoPauseEnabled();
        if (g.pausedOnOpen) {
            acc::engine::BeginOverlayPause(
                acc::engine::OverlayPauseOwner::UnifiedMenu);
        } else {
            acclog::Write("UnifiedMenu", "open without pause — Action Menu "
                "auto-pause option off");
        }
    }
}

int PersonalSelection(int col) {
    if (col < 0 || col >= kColumnCount) return 0;
    return g_personalSel[col] < 0 ? 0 : g_personalSel[col];
}

int TargetSelection(int row) {
    if (row < 0 || row >= kRowCount) return 0;
    return g_targetSel[row] < 0 ? 0 : g_targetSel[row];
}

bool IsActive() { return g.active; }

bool IsSuspended() { return g.active && g.suspended; }

// Re-speak the current category against the live menus. Used when a stacked
// overlay (the combat queue) closes back onto this menu: the world stayed
// paused (owner-tracked overlay pause), so the user needs to hear they have
// landed back here at the same position. Mirrors the resume re-speak in
// SetForegroundBlocked, minus the suspend bookkeeping. No-op when inactive or
// suspended (a blocking engine panel still owns input in that case).
void ReannounceCurrent() {
    if (!g.active || g.suspended) return;
    void* tam = acc::engine_radial::ResolveTargetActionMenu();
    void* mi  = acc::engine_actionbar::ResolveMainInterface();
    CatKind savedKind = g.cats[g.curCat].kind;
    int     savedSlot = g.cats[g.curCat].slot;
    BuildCategoryList(g.hasTargetBlock ? tam : nullptr, mi);
    if (g.catCount == 0) {
        acclog::Write("UnifiedMenu",
                      "reannounce — all categories empty; disarming");
        ForceDisarm("reannounce-empty");
        return;
    }
    int loc = LocateCat(savedKind, savedSlot);
    g.curCat = (loc >= 0) ? loc : ClampInt(g.curCat, 0, g.catCount - 1);
    acclog::Write("UnifiedMenu", "reannounce cat=%d/%d", g.curCat, g.catCount);
    SpeakCategory(tam, mi, /*prefix=*/nullptr);
}

// Panel-stack suspend / resume. The menu owns no engine GUI panel, so when
// the engine pushes a blocking panel over it (a MessageBox, a hotkey-opened
// sub-screen) it must stop owning input — otherwise both the panel and the
// menu consume the same nav keys (the quit-confirm double-speak in
// patch-20260609-111933.log). Unlike a disarm, we keep the menu's state and
// our world pause so the user returns to the same category/entry when the
// panel closes — matching how native engine menus restore focus under a
// dismissed popup (the regression the disarm-only first cut caused). Driven
// per-tick by interact_hotkey with the current foreground-blocked state; only
// the edges do work.
void SetForegroundBlocked(bool blocked) {
    if (!g.active) { g.suspended = false; return; }
    if (blocked == g.suspended) return;  // no edge
    g.suspended = blocked;

    if (blocked) {
        acclog::Write("UnifiedMenu", "suspended — blocking panel over menu");
        // Leave our overlay pause held; the blocking panel manages its own
        // pause on top. We re-assert on resume in case its close churned it.
        return;
    }

    // Resume. The closing panel's own cleanup can clear the world pause
    // (TickInputClassReassert's modal-edge unpause runs for us because we are
    // not a sub-screen), so re-assert our overlay pause — BeginOverlayPause is
    // idempotent, so this is a no-op when the pause survived. Only when we own
    // the pause (Action Menu auto-pause on); otherwise the menu runs live and
    // there is nothing to re-assert.
    if (g.pausedOnOpen)
        acc::engine::BeginOverlayPause(
            acc::engine::OverlayPauseOwner::UnifiedMenu);

    // Rebuild against the live menus (the engine may have re-populated
    // action_lists while the panel was up) and re-locate the cursor on the
    // same category, mirroring HandleInputEvent's entry rebuild. Then re-speak
    // so the user hears they are back in the menu at the same position.
    void* tam = acc::engine_radial::ResolveTargetActionMenu();
    void* mi  = acc::engine_actionbar::ResolveMainInterface();
    CatKind savedKind = g.cats[g.curCat].kind;
    int     savedSlot = g.cats[g.curCat].slot;
    BuildCategoryList(g.hasTargetBlock ? tam : nullptr, mi);
    if (g.catCount == 0) {
        // Everything drained while the panel was up — close cleanly rather
        // than resuming onto an empty menu.
        acclog::Write("UnifiedMenu", "resume — all categories empty; disarming");
        ForceDisarm("resume-empty");
        return;
    }
    int loc = LocateCat(savedKind, savedSlot);
    g.curCat = (loc >= 0) ? loc : ClampInt(g.curCat, 0, g.catCount - 1);
    acclog::Write("UnifiedMenu", "resumed — blocking panel closed; re-speaking "
        "cat=%d/%d", g.curCat, g.catCount);
    SpeakCategory(tam, mi, /*prefix=*/nullptr);
}

void ForceDisarm(const char* reason) {
    if (!g.active) return;
    acclog::Write("UnifiedMenu", "disarm — reason=%s", reason ? reason : "?");
    g.active = false;
    g.suspended = false;
    g.med.active = false;
    if (g.pausedOnOpen)
        acc::engine::EndOverlayPause(
            acc::engine::OverlayPauseOwner::UnifiedMenu);
    g.pausedOnOpen = false;
    g.catCount = 0;
    g.curCat = 0;
    g.targetHandle = 0;
    g.creature = false;
    g.hasTargetBlock = false;
    g.reqSlot = 0;
    g.targetName[0] = '\0';
    g.unfoldDeclined = 0;
    // Shadows persist intentionally — keep the user's per-slot variant.
}

}  // namespace acc::unified_menu
