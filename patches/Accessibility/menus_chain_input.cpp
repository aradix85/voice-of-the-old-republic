// Input against an already-built chain (menus_chain.cpp builds it): the
// Up/Down/Home/End step with its announcement and cursor warp, and the child
// walker. Enter is in menus_chain_input_enter.cpp, Left/Right and Esc in
// menus_chain_input_keys.cpp; menus_dispatch.cpp decides the handler order.

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
#include "menus_nav.h"
#include "menus_pending.h"
#include "menus_store.h"
#include "speech.h"
#include "strings.h"

#include <cstdio>

namespace acc::menus::chain {

namespace {

// A panel with nothing to navigate is left to the engine, but walked into the
// log once so its structure can be studied (the engine never focuses these, so
// the usual panel walk does not fire).
void OnEmptyChain(void* panel, bool& consumed) {
    const acc::engine::PanelKind kind = acc::engine::IdentifyPanel(panel);
    acclog::Write("Menus.Chain", "empty panel=%p kind=%s has no navigable "
                  "controls; input not consumed",
                  panel, acc::engine::PanelKindName(kind));

    static void* s_walked[16];
    static int   s_walkedCount = 0;
    bool walked = false;
    for (int i = 0; i < s_walkedCount; ++i) {
        if (s_walked[i] == panel) { walked = true; break; }
    }
    if (!walked && s_walkedCount < 16) {
        s_walked[s_walkedCount++] = panel;
        acclog::Write("Menus.EmptyChain", "walk panel=%p kind=%s",
                      panel, acc::engine::PanelKindName(kind));
        WalkChildren("Menus.EmptyChain", panel, kPanelControlsOffset);
    }

    // The map is the exception: with its buttons hidden the chain is empty, and
    // the engine would answer Up/Down by stepping its map-note list, a second
    // hint reader the chain deliberately drops.
    if (kind == acc::engine::PanelKind::InGameMap) {
        consumed = true;
        acclog::Trace("Menus.Chain", "empty map panel — nav key consumed so "
                                     "the engine's note stepper stays out");
    }
}

void SpeakCountSuffix(acc::strings::Id fmt, int n) {
    char suffix[64];
    snprintf(suffix, sizeof(suffix), acc::strings::Get(fmt), n);
    acc::speech::Speak(suffix, /*interrupt=*/false);
}

// Speak the newly focused entry, then the per-panel extras. The chargen and
// crafting helpers no-op on every panel but their own.
void AnnounceStep(const ChainEntry& e) {
    // Chargen skill descriptions run for seconds: without this the user hears
    // the description of the row they already left.
    if (acc::menus::chargen_skills::IsChargenSkillsPanel(g_chainPanel)) {
        acc::speech::Silence();
    }
    acc::menus::monitors::AnnounceControl(e.control);
    // Mirror the chain focus into the engine's own selection, which Left/Right
    // (chargen) and "Create Item" (crafting) act on.
    acc::menus::chargen_attr::SyncSelectedAbilityFromChainFocus();
    acc::menus::chargen_skills::SyncSelectedSkillFromChainFocus();
    acc::menus::crafting::SyncSelectedRowFromChainFocus();
    acc::menus::chargen_attr::AnnounceChainStepSuffix(g_chainPanel, e.control);
    acc::menus::chargen_attr::AnnounceChainStepDescription(g_chainPanel, e.control);
    acc::menus::chargen_skills::AnnounceChainStepSuffix(g_chainPanel, e.control);
    acc::menus::chargen_skills::AnnounceChainStepDescription(g_chainPanel, e.control);
    acc::menus::store::AnnounceChainStepSuffix(g_chainPanel, e.control);
    acc::menus::crafting::AnnounceChainStepSuffix(g_chainPanel, e.control);
    // Inventory and loot rows: a stack size above 1, or the charges left (a
    // charged item never stacks). Store rows have their own suffix above.
    if (acc::engine::IsInventoryItemRow(e.control)) {
        const int stack = acc::engine::ReadItemRowStackCount(e.control);
        if (stack > 1) SpeakCountSuffix(acc::strings::Id::FmtItemStackSuffix, stack);
        const int charges = acc::engine::ReadItemRowCharges(e.control);
        if (charges >= 0) SpeakCountSuffix(acc::strings::Id::FmtItemChargeSuffix, charges);
    }
}

// Warp the cursor onto the entry so the engine's hover handler follows the
// focus. Not for text-only entries (nothing to hover), and never onto a panel
// whose identity no longer holds: that hands engine code a freed object.
// The engine's focus echo of the warp is deduplicated by AnnounceControl.
void WarpCursor(const ChainEntry& e, bool safeToDrive, int* x, int* y) {
    *x = e.cx;
    *y = e.cy;
    if (e.textOnly || !safeToDrive) return;
    if (IsTabButton(e.control)) *y += ComputeTabClickOffsetY(g_chainPanel);
    if (acc::menus::detail::IsClassSelectionIcon(g_chainPanel, e.control) &&
        g_classIconClickOffsetX > 0) {
        *x += g_classIconClickOffsetX;
    }
    // The chargen attribute and skill rows hit-test one row up; without this
    // the engine describes the row above.
    const int abilityPitch =
        acc::menus::chargen_attr::RowPitchForCursorWarp(g_chainPanel, e.control);
    if (abilityPitch > 0) *y += abilityPitch;
    const int skillPitch =
        acc::menus::chargen_skills::RowPitchForCursorWarp(g_chainPanel, e.control);
    if (skillPitch > 0) *y += skillPitch;
    acc::menus::pending::QueueMoveCursor(*x, *y, e.control);
}

}  // namespace

void WalkChildren(const char* label, void* parent, size_t offset,
                  const char* kindName) {
    if (!parent) return;
    // One BlockLog per walk. Key() leaves out the heap pointers, so a walk of a
    // panel recreated at a new address folds into "(repeated Nx)"; the vtable
    // addresses stay, they identify the class.
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
        // `parent` lets the per-kind fallback name InGameMenu's icon controls,
        // which carry no text of their own.
        const char* source = acc::menus::extract::FromControl(
            child, text, sizeof(text), parent);
        if (source) {
            block.Line("  [%d] %p id=%d src=%s text=\"%s\"",
                       i, child, id, source, text);
            block.Key("  [%d] id=%d src=%s text=\"%s\"", i, id, source, text);
        } else {
            char vtbl[160];
            acc::engine::DumpControlVtable(child, vtbl, sizeof(vtbl));
            block.Line("  [%d] %p id=%d src=none %s", i, child, id, vtbl);
            block.Key("  [%d] id=%d src=none %s", i, id, vtbl);
        }
    }
}

void HandleNavStep(void* activePanel, int code, int val, bool& consumed) {
    if (val == 0) return;
    const bool home = (code == kInputHome);
    const bool end  = (code == kInputEnd);
    const bool up   = (code == kInputNavUp);
    const bool down = (code == kInputNavDown);
    if (!(up || down || home || end)) return;
    if (activePanel == nullptr) return;

    // Rebind on another panel, and when the bound panel's address now holds a
    // different object: stepping a chain of a dead panel crashed K2 chargen.
    if (activePanel != g_chainPanel || !ChainPanelIdentityHolds()) {
        RebindChain(activePanel);
    }
    if (g_chainCount <= 0) {
        OnEmptyChain(activePanel, consumed);
        return;
    }

    g_chainIndex = acc::menus::nav::ClampedCursorStep(g_chainIndex, g_chainCount,
                                                      up, down, home, end);
    const ChainEntry& e = g_chain[g_chainIndex];
    AnnounceStep(e);

    const bool safeToDrive = ChainPanelIdentityHolds();
    int cursorX = 0;
    int cursorY = 0;
    WarpCursor(e, safeToDrive, &cursorX, &cursorY);
    const char* dirTag = home ? "HOME" : end ? "END" : down ? "DOWN" : "UP";
    acclog::Write("Menus.Chain",
                  "step panel=%p index=%d/%d target=%p center=(%d,%d) cursor=(%d,%d)%s %s",
                  g_chainPanel, g_chainIndex, g_chainCount,
                  e.control, e.cx, e.cy, cursorX, cursorY,
                  e.textOnly ? " text-only" : (safeToDrive ? "" : " no-warp-stale"),
                  dirTag);
    consumed = true;
}

}  // namespace acc::menus::chain
