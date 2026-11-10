// Building the chain's entries: controls, list-box rows, the virtual rows for
// value labels (credits, stats, wager, equipment stats, mod settings), the
// reading order, and the click offsets for slots and class icons.

#include "menus_chain_internal.h"

#include "engine_game.h"     // IsKotor2: filter-strip demotion, class icons
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "log.h"
#include "menus_charsheet.h"
#include "menus_credits.h"
#include "menus_equipstats.h"
#include "menus_extract.h"
#include "menus_crafting.h"  // IsHiddenCraftingListBox — K2 crafting view filter
#include "menus_store.h"

namespace acc::menus::chain {

namespace {

// Synthetic sortCy base for a filter strip demoted below the list. Real
// control cy tops out near 1800 at 2880x1800 and the mod-settings root sits
// at 9000, so the whole 8000..8999 band is free for "after every real
// control, still before mod settings".
constexpr int kFilterStripSortBase = 8000;

void AppendChainTextOnly(void* control, void* panel) {
    if (g_chainCount >= kMaxChainEntries) return;
    if (!control) return;
    char tmp[512];
    if (!acc::menus::extract::FromControl(control, tmp, sizeof(tmp), panel)) return;
    int cx, cy;
    if (!GetControlCenter(control, cx, cy)) return;
    g_chain[g_chainCount++] = {
        control, cx, cy, cy,
        /*textOnly=*/true, /*virtualKind=*/0, /*geometricOrder=*/true
    };
}

// Shared body of the four virtual-row anchors below (credits, charsheet
// stat rows, pazaak wager, equip stats). All four register a label control
// the engine renders for sighted players but that IsChainNavigable rejects,
// so the chain walker would otherwise skip it.
//
// cx comes from the real label position so a cursor warp on chain step
// lands the mouse on the actual on-screen text; cy is overridden to the
// caller's synthetic sortCy so the y-sort produces the logical reading
// order instead of interleaving with the real buttons.
//
// `probe` is the row's own extractor, called only to answer "does this row
// have text yet". A label can be present but empty — a mid-frame race
// during a re-snapshot, or a field the engine has not populated. Those are
// skipped silently and reappear on the next rebind.
typedef bool (*RowProbe)(void* panel, void* labelControl);

bool AppendVirtualRow(void* labelControl, int sortCy, void* panel,
                      RowProbe probe) {
    if (g_chainCount >= kMaxChainEntries) return false;
    int cx, cy;
    if (!GetControlCenter(labelControl, cx, cy)) {
        cx = 0;
    }
    if (!probe(panel, labelControl)) return true;
    g_chain[g_chainCount++] = {
        labelControl, cx, sortCy, sortCy, /*textOnly=*/true
    };
    return true;
}

bool ProbeCreditsRow(void* panel, void* labelControl) {
    char probe[8];
    return acc::menus::credits::ExtractCreditsRow(
        panel, labelControl, probe, sizeof(probe));
}

bool ProbeStatRow(void* panel, void* labelControl) {
    char probe[8];
    return acc::menus::charsheet::ExtractStatRow(
        panel, labelControl, probe, sizeof(probe));
}

bool ProbeWagerRow(void* panel, void* labelControl) {
    char probe[8];
    return acc::menus::extract::FromControl(
        labelControl, probe, sizeof(probe), panel) != nullptr;
}

bool ProbeEquipStatRow(void* panel, void* labelControl) {
    char probe[8];
    return acc::menus::equipstats::ExtractEquipStatRow(
        panel, labelControl, probe, sizeof(probe));
}

// Reading-order comparator: y first, then x — but the x tiebreak applies
// only when BOTH entries are geometrically ordered (see
// ChainEntry::geometricOrder). Listbox rows and virtual rows share a single
// sortCy by design and keep their insertion order, so mixing an x compare
// into their block would scramble it.
bool ChainEntryPrecedes(const ChainEntry& a, const ChainEntry& b) {
    if (a.sortCy != b.sortCy) return a.sortCy < b.sortCy;
    if (!a.geometricOrder || !b.geometricOrder) return false;
    return a.cx < b.cx;
}

}  // namespace

// `sortCy` overrides only the reading-order key, never the real cy the cursor
// warp uses. Listbox rows pass their listbox's extent top here so the whole
// listbox stays one contiguous block — see the ChainEntry::sortCy comment.
void AppendChainEntry(void* control, int sortCy) {
    if (g_chainCount >= kMaxChainEntries) return;
    if (!IsChainNavigable(control))       return;
    int cx, cy;
    if (!GetControlCenter(control, cx, cy)) return;
    bool ownCy = (sortCy == kSortByOwnCy);
    g_chain[g_chainCount++] = {
        control, cx, cy,
        ownCy ? cy : sortCy,
        /*textOnly=*/false, /*virtualKind=*/0,
        // Only panel-direct controls (the default sortCy) are ordered
        // geometrically; a caller-supplied sortCy means this is a listbox
        // row whose block order must survive the sort untouched.
        /*geometricOrder=*/ownCy
    };
}

// A listbox among the panel's controls:
//   * size > 1  — recurse one level into button children (sub-dialog
//     settings list).
//   * size == 1 in a modal popup — promote the listbox to a text-only
//     chain entry so arrow keys can land for re-announce.
//   * size == 1 elsewhere — descriptive label blob; skipped.
void AppendListBoxChildren(void* panel, void* c, void* equipPickerLb,
                           void* workbenchUpgradeLb, bool modalText,
                           int& listBlockTop) {
    void** vt = *reinterpret_cast<void***>(c);
    if (reinterpret_cast<uintptr_t>(vt) != kVtableListBox) return;
    // The engine keeps BOTH store listboxes (shop and inventory) in
    // panel.controls whichever is visible; the hidden one's rows can be
    // neither examined nor traded. The K2 crafting screens flip visibility
    // the same way. The two pickers have their own specs.
    if (acc::menus::store::IsHiddenStoreListBox(panel, c) ||
        acc::menus::crafting::IsHiddenCraftingListBox(panel, c) ||
        c == equipPickerLb || c == workbenchUpgradeLb) {
        return;
    }
    // The journal's quest list (items_listbox @ panel+0x5c4) must expose its
    // rows even when only ONE quest is active — the norm in the Endar Spire
    // tutorial — or the user hears no journal content. Its rows carry the
    // journal-entry Enter handler, so descend exactly like the size>1 path.
    // The description listbox (+0x1a4) is a different control and stays
    // skipped. The inventory's item list needs the same exemption: one item
    // carried is a real state (the opening hour of KOTOR 2), and the size==1
    // rule was written for description blobs, not for a short list.
    const PanelKind pk = IdentifyPanel(panel);
    auto* pb = reinterpret_cast<unsigned char*>(panel);
    const bool shortListOk =
        (pk == PanelKind::InGameJournal && c == pb + kJournalItemsListBoxOffset) ||
        (pk == PanelKind::InGameInventory && c == pb + kInventoryItemListBoxOffset);
    // Sort anchor for every row this listbox contributes. Row extents are
    // listbox-local content coordinates that keep climbing one row-pitch at a
    // time whether or not the row is inside the viewport, so they are not
    // comparable with the panel buttons' screen coordinates. The listbox
    // itself IS a panel-direct child with a screen-absolute extent, so its top
    // places the whole block where the list actually sits. Falls back to
    // per-row cy if the extent is degenerate.
    int blockSortCy = kSortByOwnCy;
    int lbCx, lbCy;
    if (GetControlCenter(c, lbCx, lbCy)) {
        blockSortCy = *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(c) +
            kControlExtentOffset + sizeof(int));   // extent.top
    }
    auto* lbList = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(c) + kListBoxControlsOffset);
    if (!lbList || !lbList->data) return;
    if (lbList->size > 1 || (shortListOk && lbList->size == 1)) {
        // AppendChainEntry stops at the chain bound anyway; the same bound
        // here keeps the two limits in lockstep.
        int lbN = lbList->size > kMaxChainEntries ? kMaxChainEntries
                                                  : lbList->size;
        for (int j = 0; j < lbN; ++j) {
            AppendChainEntry(lbList->data[j], blockSortCy);
        }
        // Report the block anchor so DemoteFilterStripAboveList knows where
        // the list content starts; only a block that contributed rows and had
        // a usable extent counts.
        if (lbN > 0 && blockSortCy != kSortByOwnCy &&
            blockSortCy < listBlockTop) {
            listBlockTop = blockSortCy;
        }
    } else if (lbList->size == 1 && modalText) {
        AppendChainTextOnly(c, panel);
    }
}

// Virtual credits row for Inventory + Store. credits_value_label isn't
// IsChainNavigable, so without this the user can't reach the gold display
// the engine renders for sighted players. ForEachCreditsRowAnchor is a
// no-op for unsupported panel kinds, so we call it unconditionally.
//
// RebindChain registers it BEFORE the control/listbox walk, unlike the other
// virtual rows, because Inventory's item listbox can fill the chain to
// kMaxChainEntries and a credits row queued afterwards lost its slot to the
// cap. The cy-sort still lands credits at the top via its synthetic sortCy.
bool OnCreditsAnchor(void* labelControl, int sortCy, void* userData) {
    return AppendVirtualRow(labelControl, sortCy, userData, ProbeCreditsRow);
}

// Per-kind virtual chain entries. For InGameCharacter the panel
// hosts a dense value-label cluster (Class, Level, Experience, HP,
// FP, six attributes) that the snapshot announce already covers
// but the chain doesn't expose — labels aren't IsChainNavigable.
// ForEachStatRowAnchor visits each value-label control; we register
// it as a text-only chain entry at its real (cx, cy). The y-sort
// below then drops them into top-to-bottom reading order alongside
// the real buttons (Auto Level Up, Level Up, bottom-row
// navigation).
//
// Text-only flag means Enter re-announces (calls AnnounceControl
// again) instead of firing vtable[15] — safe for label controls
// that have no activate handler. FromControl routes through
// ExtractStatRow at section 0 so the user hears the composed phrase
// ("Strength 14, +2") rather than the bare label text ("14").
bool OnStatRowAnchor(void* labelControl, int sortCy, void* userData) {
    return AppendVirtualRow(labelControl, sortCy, userData, ProbeStatRow);
}

// Virtual wager row for the Pazaak wager popup. Same shape as credits:
// the maximum_label isn't IsChainNavigable, so without this the user
// can't reach the wager / max / credits readout. The anchor is a no-op
// for non-PazaakWager panels.
bool OnWagerAnchor(void* labelControl, int sortCy, void* userData) {
    return AppendVirtualRow(labelControl, sortCy, userData, ProbeWagerRow);
}

// Virtual stat rows for the Equip panel (Vitality, Defense, Attack,
// Damage). Same shape as the credits anchor — value labels live
// inline in CSWGuiInGameEquip but aren't IsChainNavigable, so the
// chain walker would skip them. ForEachEquipStatRowAnchor self-gates
// on InGameEquip (no-op elsewhere) and emits sortCy values above
// every real button so stats land at the END of the chain after the
// slots + Back / Change* buttons.
bool OnEquipStatAnchor(void* labelControl, int sortCy, void* userData) {
    return AppendVirtualRow(labelControl, sortCy, userData, ProbeEquipStatRow);
}

// Virtual "Mod settings" entry for InGameOptions + MainMenuOptions.
// Same shape as the credits / stat-row anchors, but the control pointer is
// a static sentinel from menus_modsettings rather than an engine label: it
// is never read or dispatched as a real CSWGuiControl. Position is
// synthetic: sortCy=9000 lands the entry at the end of the chain, after
// every real button on the Options strip.
bool OnModSettingsAnchor(void* sentinel, int sortCx, int sortCy,
                         void* /*userData*/) {
    if (g_chainCount >= kMaxChainEntries) return false;
    g_chain[g_chainCount++] = {
        sentinel, sortCx, sortCy, sortCy,
        /*textOnly=*/false,
        /*virtualKind=*/kVirtualMod_SettingsRoot
    };
    return true;
}

// Per-kind virtual chain entries for StatusSummary — the engine's quest-
// progress / journal-entry info popup. Its body is a cluster of label
// controls, one per notification type (journal entry, credits, XP, DS/LS
// points, items gained/lost, stealth XP), NOT a listbox, so the listbox
// text-only path above never exposed it and the user only reached OK.
// The engine shows exactly the row(s) that apply and leaves the rest as
// hidden templates whose text still reads "<CUSTOM0>"; only the shown
// rows carry the CSWGuiControl visible bit (kControlVisibleBit at +0x44).
// Expose just those as text-only entries so the user can arrow back and
// re-read, and so the substituted value ("Experience gained: 45")
// is what's read rather than the placeholder. The y-sort below drops them
// above OK (cy ~25 vs ~51) into natural reading order.
void AppendStatusSummaryRows(void* panel, CExoArrayList* list, int n) {
    if (IdentifyPanel(panel) != PanelKind::StatusSummary) return;
    for (int i = 0; i < n; ++i) {
        void* c = list->data[i];
        if (!c) continue;
        if (CallDowncast(c, kVtableAsLabel) == nullptr) continue;
        uint32_t bitFlags = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(c) + kControlBitFlagsOffset);
        if ((bitFlags & kControlVisibleBit) == 0) continue;
        AppendChainTextOnly(c, panel);  // self-skips empty text / no extent
    }
}

// House rule: on a list-driven screen the list is read first and its filter /
// toggle strip after it. KOTOR 2 lays several of those strips out ABOVE the
// list — the journal's three sort-mode buttons (Time / Name / Planet) sit over
// the quest list, the load/save screen's cloud-save toggle sits over the
// save slots — and this chain sorts geometrically, so the strip became the
// first thing Up/Down reached AND the panel's initial focus (RebindChain
// anchors at index 0 when the engine's active control is the listbox itself,
// which is exactly what saveload does). Opening "Load Game" therefore spoke
// the first save slot and then the focused Cloud toggle, one Enter away from
// flipping a setting the user never went looking for.
//
// Demote by geometry rather than by a per-screen button list: any panel-direct
// entry sitting above the topmost list block gets kFilterStripSortBase + its
// own cy. That lands it after every real control while preserving the strip's
// own top-to-bottom order, and the entries stay geometric so the comparator's
// x tiebreak still walks a horizontal filter row left to right.
//
// KOTOR 2 only. KOTOR 1 already puts these strips below the list, so the
// comparison would never fire there — gating on the game means a shipped
// screen's order can't be rearranged by a geometry accident.
//
// listBlockTop is INT_MAX when no listbox contributed rows to this chain
// (every non-list panel, plus the modal popups whose single-row listbox
// becomes a text-only entry): nothing to sit above, so nothing moves.
void DemoteFilterStripAboveList(void* panel, int listBlockTop) {
    if (!acc::game::IsKotor2()) return;
    if (listBlockTop == INT_MAX) return;
    for (int i = 0; i < g_chainCount; ++i) {
        if (!g_chain[i].geometricOrder) continue;
        if (g_chain[i].cy >= listBlockTop) continue;
        g_chain[i].sortCy = kFilterStripSortBase + g_chain[i].cy;
        char text[128];
        const char* src = acc::menus::extract::FromControl(
            g_chain[i].control, text, sizeof(text), panel);
        acclog::Write("Menus.Chain",
                      "demote above-list panel=%p ctrl=%p cy=%d listTop=%d "
                      "-> sort=%d text=\"%s\"",
                      panel, g_chain[i].control, g_chain[i].cy, listBlockTop,
                      g_chain[i].sortCy, src ? text : "");
    }
}

// Insertion sort in reading order (ChainEntryPrecedes). Stable; the n^2 is
// cheap int compares and runs once per rebind (panel open / content change),
// not per tick, so it stays well within budget even at a full
// kMaxChainEntries chain.
//
// Stability is load-bearing, not incidental: every row of one listbox shares
// a single sortCy AND is flagged non-geometric, so the comparator never
// separates them and their relative order is decided purely by the order
// AppendListBoxChildren walked them — i.e. the engine's own row order.
void SortChainBySortCy() {
    for (int i = 1; i < g_chainCount; ++i) {
        for (int j = i; j > 0 && ChainEntryPrecedes(g_chain[j], g_chain[j - 1]); --j) {
            ChainEntry tmp = g_chain[j];
            g_chain[j]     = g_chain[j - 1];
            g_chain[j - 1] = tmp;
        }
    }
}

// Squash cycle-arrow flankers from the chain. Empty-text navigable
// entries that share a y-row with a NEARBY text-bearing entry are
// cycle arrows; the user reaches them via Left/Right cycle dispatch
// on the value-display entry. Lone empty-text entries with no nearby
// text-bearing same-row neighbour are kept.
void SquashCycleFlankers(void* panel) {
    // Title-screen Options sub-screens (Advanced Sound EAX, Advanced
    // Graphics AA/texture/anisotropy, Game Settings difficulty) lay their
    // spinner rows out as a centred value button flanked by empty
    // left/right arrow buttons at x=72 / x=288 — ~108 px from the value at
    // x=180, beyond the 80 px reach that catches chargen's tighter
    // spinners. Widen the squash for just these kinds so the redundant
    // arrows (the user cycles the value with Left/Right) drop out of Up/Down
    // nav instead of speaking "control N". Gated by kind so chargen and
    // every other panel keep the conservative 80 px reach.
    //
    // Both reaches, and the same-row tolerance below, are quoted in .gui
    // authoring units — they were measured off KOTOR 1's .gui files — so
    // they go through ScaleGuiThresholdPx to survive KOTOR 2's stretched
    // extents. Without that the chargen steppers (27 authored units apart,
    // 97 at 2880x1800) and the options spinners (108 authored, ~389) both
    // fall outside the reach and announce as "control N".
    const int kSquashDxMax = ScaleGuiThresholdPx(
        acc::engine::IsMainMenuOptionsSubScreen(IdentifyPanel(panel))
            ? 130 : 80);
    const int kSquashDyMax = ScaleGuiThresholdPx(5);
    int writeIdx = 0;
    for (int i = 0; i < g_chainCount; ++i) {
        char tmp[64];
        bool hasText = acc::menus::extract::FromControl(g_chain[i].control,
                                               tmp, sizeof(tmp),
                                               panel) != nullptr;
        if (hasText) {
            g_chain[writeIdx++] = g_chain[i];
            continue;
        }
        bool sameRowWithText = false;
        for (int j = 0; j < g_chainCount; ++j) {
            if (j == i) continue;
            int dy = g_chain[j].cy - g_chain[i].cy;
            if (dy < 0) dy = -dy;
            if (dy > kSquashDyMax) continue;
            int dx = g_chain[j].cx - g_chain[i].cx;
            if (dx < 0) dx = -dx;
            if (dx == 0 || dx > kSquashDxMax) continue;
            char tmp2[64];
            if (acc::menus::extract::FromControl(g_chain[j].control,
                                        tmp2, sizeof(tmp2),
                                        panel) != nullptr) {
                sameRowWithText = true;
                break;
            }
        }
        if (!sameRowWithText) {
            g_chain[writeIdx++] = g_chain[i];
        }
    }
    g_chainCount = writeIdx;
}

// Row pitch between InGameEquip slot buttons, used to compensate the
// simulated click position. Zero when the panel is not InGameEquip or
// fewer than two slots made it into the chain.
void ComputeEquipSlotClickOffset(void* panel) {
    g_equipSlotClickOffsetY = 0;
    if (IdentifyPanel(panel) != PanelKind::InGameEquip) return;
    int firstSlotIdx = -1;
    int firstSlotY   = 0;
    for (int i = 0; i < g_chainCount; ++i) {
        if (acc::menus::detail::EquipSlotIndexFromButton(
                panel, g_chain[i].control) < 0) continue;
        if (firstSlotIdx < 0) {
            firstSlotIdx = i;
            firstSlotY   = g_chain[i].cy;
        } else if (g_chain[i].cy != firstSlotY) {
            int spacing = g_chain[i].cy - firstSlotY;
            g_equipSlotClickOffsetY = spacing < 0 ? -spacing : spacing;
            break;
        }
    }
}

// Column pitch between chargen class-selection icons, same purpose as
// ComputeEquipSlotClickOffset above.
//
// KOTOR 1 ONLY. The compensation exists because KOTOR 1's classsel.gui
// hit-test resolves one column to the RIGHT of each icon's own extent:
// warping to BTN_SEL2's centre (x=189) hovers BTN_SEL1, so the warp has to
// aim at x=276 (centre + 87) to land on BTN_SEL2. KOTOR 2 has no such shift:
// the K1 compensation overshoots by exactly one column there, which skipped
// icons, needed repeated presses, and left the focused icon silent.
void ComputeClassIconClickOffset(void* panel) {
    g_classIconClickOffsetX = 0;
    if (acc::game::IsKotor2() || !panel) return;
    void** pVt = *reinterpret_cast<void***>(panel);
    if (reinterpret_cast<uintptr_t>(pVt) != kVtableCSWGuiClassSelection) return;
    int firstIconIdx = -1;
    for (int i = 0; i < g_chainCount; ++i) {
        if (!IsClassSelectionIcon(panel, g_chain[i].control)) continue;
        if (firstIconIdx < 0) {
            firstIconIdx = i;
        } else if (g_chain[i].cy == g_chain[firstIconIdx].cy) {
            int spacing = g_chain[i].cx - g_chain[firstIconIdx].cx;
            if (spacing < 0) spacing = -spacing;
            if (spacing > 0) g_classIconClickOffsetX = spacing;
            break;
        }
    }
}

}  // namespace acc::menus::chain
