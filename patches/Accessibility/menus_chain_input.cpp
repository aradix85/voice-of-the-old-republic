// chain-navigation input handlers.
//
// Split out of menus_chain.cpp by the Phase-1 structure pass (refactoring
// candidate 2). menus_chain.cpp builds and maintains the chain (RebindChain
// and its validation/helper cluster); this file consumes input against an
// already-built chain. All five entry points were already declared
// individually in menus_chain.h, so the move is verbatim - no header
// change, no call-site change.
//
//   HandleEnterActivation  Enter / Shift+Enter on the chain cursor
//   WalkChildren           child-list walker (also the empty-chain probe)
//   HandleNavStep          Up / Down chain step + cursor warp
//   HandleLeftRight        Left / Right (cycle widgets, sliders, tabs)
//   HandleEsc              Esc back-out / modal close
//
// Handler ORDER is decided by the caller in menus_dispatch.cpp, not here.

#include "menus_chain.h"

#include "engine_input.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "log.h"
#include "menus_chargen_attr.h"
#include "menus_chargen_skills.h"
#include "menus_crafting.h"
#include "menus_extract.h"
#include "menus_internal.h"
#include "menus_monitors.h"
#include "menus_pending.h"
#include "menus_store.h"
#include "speech.h"
#include "strings.h"

#include <cstdio>

namespace acc::menus::chain {

void WalkChildren(const char* label, void* parent, size_t offset,
                  const char* kindName) {
    if (!parent) return;
    // One BlockLog per walk. Line() is the full display (with pointers); Key()
    // is the same content with the volatile heap pointers stripped, so a walk
    // of a panel the engine recreated at a fresh address still hashes equal and
    // folds to a "(repeated Nx)" summary. The block emits-or-folds when `block`
    // leaves scope on any return path. Stable vtable code-addresses stay in the
    // key (they identify the control class).
    acclog::BlockLog block(label);
    if (kindName && *kindName) {
        block.Line("panel=%p kind=%s", parent, kindName);
        block.Key("kind=%s", kindName);
    }

    auto* list = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(parent) + offset);
    if (!list->data || list->size <= 0) {
        block.Line("walk parent=%p children=0", parent);
        block.Key("children=0");
        return;
    }
    int count = list->size;
    if (count > 256) {
        block.Line("walk parent=%p size_oob=%d (capped)", parent, count);
        block.Key("size_oob=%d", count);
        count = 256;
    }
    block.Line("walk parent=%p children=%d", parent, list->size);
    block.Key("children=%d", list->size);
    for (int i = 0; i < count; ++i) {
        void* child = list->data[i];
        if (!child) {
            block.Line("  [%d]=NULL", i);
            block.Key("  [%d]=NULL", i);
            continue;
        }
        int id = *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(child) + kControlIdOffset);
        char text[256];
        // Pass `parent` so the perkind fallback resolves correctly when
        // walking InGameMenu's children — the icon labels/buttons have empty
        // CExoString/strref/text_object/gui_string and only resolve via the
        // panel-keyed perkind table.
        const char* source = acc::menus::extract::FromControl(
            child, text, sizeof(text), parent);
        if (source) {
            block.Line("  [%d] %p id=%d src=%s text=\"%s\"",
                       i, child, id, source, text);
            block.Key("  [%d] id=%d src=%s text=\"%s\"", i, id, source, text);
        } else {
            char vtbl[160];
            acc::engine::DumpControlVtable(child, vtbl, sizeof(vtbl));
            block.Line("  [%d] %p id=%d src=none %s",
                       i, child, id, vtbl);
            block.Key("  [%d] id=%d src=none %s", i, id, vtbl);
        }
    }
}

void HandleNavStep(void* activePanel, int code, int val, bool& consumed) {
    if (val == 0) return;
    bool navKeyIsHome = (code == kInputHome);
    bool navKeyIsEnd  = (code == kInputEnd);
    bool navKeyIsUp   = (code == kInputNavUp);
    bool navKeyIsDown = (code == kInputNavDown);
    if (!(navKeyIsUp || navKeyIsDown || navKeyIsHome || navKeyIsEnd)) return;
    if (activePanel == nullptr) return;

    // Rebind on a different panel, and also when the panel we are bound to
    // is no longer the object we bound (its address reused by another
    // panel — see ChainPanelIdentityHolds). Stepping a chain whose entries
    // describe a dead object is how KOTOR 2 chargen crashed.
    if (activePanel != g_chainPanel || !ChainPanelIdentityHolds()) {
        RebindChain(activePanel);
    }
    if (g_chainCount == 0) {
        // Foreground panel has no navigable controls. Log so we can see
        // which panels are routing-only (e.g. the recurring 074FE618
        // overlay and the dialog routing target 0FDEE418 observed in
        // the in-game session) and decide whether to add a fallback
        // strategy (walk down the modal stack to the next chain-eligible
        // panel, or surface the panel's content via the title/listbox
        // path). For now: log only, leave the input unconsumed so the
        // engine sees it.
        acc::engine::PanelKind emptyKind = acc::engine::IdentifyPanel(activePanel);
        acclog::Write("Menus.Chain", "empty panel=%p kind=%s has no navigable "
                      "controls; input not consumed",
                      activePanel, acc::engine::PanelKindName(emptyKind));

        // Walk the panel ONCE so we can see what's actually in it.
        // OnSetActiveControl's panel-walk gate (s_lastPanel) doesn't
        // fire on these panels because the engine never sets focus on
        // them. Without a walk we never learn their structure — log-only
        // diagnostics give us nothing actionable.
        static void* s_walkedEmptyPanels[16];
        static int   s_walkedEmptyCount = 0;
        bool walked = false;
        for (int i = 0; i < s_walkedEmptyCount; ++i) {
            if (s_walkedEmptyPanels[i] == activePanel) { walked = true; break; }
        }
        if (!walked && s_walkedEmptyCount < 16) {
            s_walkedEmptyPanels[s_walkedEmptyCount++] = activePanel;
            acclog::Write("Menus.EmptyChain", "walk panel=%p kind=%s",
                          activePanel, acc::engine::PanelKindName(emptyKind));
            WalkChildren("Menus.EmptyChain", activePanel, kPanelControlsOffset);
        }

        // The map is the exception to "leave it to the engine". Its buttons
        // (party selection, return to base) are hidden in most areas, which
        // empties the chain — and the engine's own map panel answers an
        // unconsumed Up / Down by stepping its map-note list, the very
        // stepper IsDecorativeControl drops from the chain, a second,
        // unwanted hint reader. Nothing is there to navigate, so the press is
        // spent here.
        if (emptyKind == acc::engine::PanelKind::InGameMap) {
            consumed = true;
            acclog::Trace("Menus.Chain", "empty map panel — nav key consumed so "
                                         "the engine's note stepper stays out");
        }
    }
    if (g_chainCount <= 0) return;

    int newIndex;
    if (navKeyIsHome) {
        newIndex = 0;
    } else if (navKeyIsEnd) {
        newIndex = g_chainCount - 1;
    } else {
        int delta = navKeyIsDown ? +1 : -1;
        newIndex = g_chainIndex + delta;
        if (newIndex < 0)              newIndex = 0;
        if (newIndex >= g_chainCount)  newIndex = g_chainCount - 1;
    }
    g_chainIndex = newIndex;

    ChainEntry& e = g_chain[g_chainIndex];
    // Chargen Skills descriptions are long (~10s of speech each) but the
    // user navigates Up/Down faster than they can read. With interrupt=false
    // (our default), each step queues "label, suffix, description" behind the
    // previous step's still-playing description — the user hears descriptions
    // one row behind their focus. Silence any in-flight speech before
    // announcing the new row, so each chain step starts fresh and the
    // just-arrived focus wins the speech channel. No-op on every other panel
    // (their descriptions are short enough to drain naturally).
    if (acc::menus::chargen_skills::IsChargenSkillsPanel(g_chainPanel)) {
        acc::speech::Silence();
    }
    acc::menus::monitors::AnnounceControl(e.control);
    // Mirror chain focus into the chargen Attributes panel's selected_ability
    // so the next Left/Right press routes OnPlusButton / OnMinusButton to the
    // focused ability rather than the default top row (STR). No-op on every
    // other panel.
    acc::menus::chargen_attr::SyncSelectedAbilityFromChainFocus();
    // Same for the chargen Skills panel — different field
    // (selected_skill_index) on a different panel, same mechanism.
    acc::menus::chargen_skills::SyncSelectedSkillFromChainFocus();
    // Same for the K2 workbench / lab-station crafting screens — the engine's
    // listbox selection_index is what "Create Item" builds from.
    acc::menus::crafting::SyncSelectedRowFromChainFocus();
    // Per-row info suffixes / descriptions across panels that need them.
    // Each helper no-ops on every panel except its own.
    acc::menus::chargen_attr::AnnounceChainStepSuffix(g_chainPanel, e.control);
    acc::menus::chargen_attr::AnnounceChainStepDescription(g_chainPanel, e.control);
    acc::menus::chargen_skills::AnnounceChainStepSuffix(g_chainPanel, e.control);
    acc::menus::chargen_skills::AnnounceChainStepDescription(g_chainPanel, e.control);
    acc::menus::store::AnnounceChainStepSuffix(g_chainPanel, e.control);
    acc::menus::crafting::AnnounceChainStepSuffix(g_chainPanel, e.control);
    // Inventory rows (CSWGuiInGameInventory / Container loot listbox): append
    // "N in stack" when stack_size > 1. Store rows are deliberately excluded —
    // the store suffix above already speaks the stock and owned counts.
    // Silent on stack_size == 1 so weapons / armour stay quiet.
    if (acc::engine::IsInventoryItemRow(e.control)) {
        int stack = acc::engine::ReadItemRowStackCount(e.control);
        if (stack > 1) {
            char suffix[64];
            snprintf(suffix, sizeof(suffix),
                     acc::strings::Get(acc::strings::Id::FmtItemStackSuffix),
                     stack);
            acc::speech::Speak(suffix, /*interrupt=*/false);
        }
        // Charged consumables can't stack, so this is mutually exclusive with
        // the suffix above. charges == 0 is still meaningful (depleted item).
        int charges = acc::engine::ReadItemRowCharges(e.control);
        if (charges >= 0) {
            char suffix[64];
            snprintf(suffix, sizeof(suffix),
                     acc::strings::Get(acc::strings::Id::FmtItemChargeSuffix),
                     charges);
            acc::speech::Speak(suffix, /*interrupt=*/false);
        }
    }
    int cursorX = e.cx;
    int cursorY = e.cy;
    // The warp exists to make the engine's own hover handler follow our
    // focus, so it must never point the engine at a panel whose identity
    // no longer holds — that hands engine code a freed object.
    bool safeToDrive = ChainPanelIdentityHolds();
    if (!e.textOnly && safeToDrive) {
        // Cursor warp + suppress-budget exist to make hover-to-focus work for
        // activatable controls. Text-only entries (modal body listboxes) have
        // no hover semantics worth chasing — skipping keeps the cursor stable
        // on whatever button the user just left, and avoids spurious
        // engine-side SetActiveControl echoes from the listbox under the cursor.
        if (IsTabButton(e.control)) {
            cursorY += ComputeTabClickOffsetY(g_chainPanel);
        }
        if (acc::menus::detail::IsClassSelectionIcon(g_chainPanel, e.control) &&
            g_classIconClickOffsetX > 0) {
            cursorX += g_classIconClickOffsetX;
        }
        // Chargen Attribute / Skills hit-test-shifts-up-one-row compensation.
        // Without it the cursor lands on the row above and the engine's
        // OnEnterPointsButton populates description_listbox for the wrong row.
        {
            int abilityPitch = acc::menus::chargen_attr::RowPitchForCursorWarp(
                g_chainPanel, e.control);
            if (abilityPitch > 0) cursorY += abilityPitch;
        }
        {
            int skillPitch = acc::menus::chargen_skills::RowPitchForCursorWarp(
                g_chainPanel, e.control);
            if (skillPitch > 0) cursorY += skillPitch;
        }
        acc::menus::pending::QueueMoveCursor(cursorX, cursorY, e.control);
        // No explicit suppress needed for the engine-side focus echo:
        // AnnounceControl above primed channel-0 dedup via MarkSpoken, so
        // DrainPendingAnnounce will short-circuit when the cursor-warp's
        // SetActive echo arrives with the same text.
    }
    const char* dirTag =
        navKeyIsHome ? "HOME" :
        navKeyIsEnd  ? "END"  :
        navKeyIsDown ? "DOWN" : "UP";
    acclog::Write("Menus.Chain",
                  "step panel=%p index=%d/%d target=%p center=(%d,%d) cursor=(%d,%d)%s %s",
                  g_chainPanel, g_chainIndex, g_chainCount,
                  e.control, e.cx, e.cy, cursorX, cursorY,
                  e.textOnly ? " text-only" :
                               (safeToDrive ? "" : " no-warp-stale"),
                  dirTag);
    // Always consume nav keys on a panel with a non-empty chain.
    consumed = true;
}

}  // namespace acc::menus::chain
