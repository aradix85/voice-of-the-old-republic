// The keys while the unified action menu is open.

#include "unified_action_menu_internal.h"

#include "engine_input.h"       // kInputNav*, kInputEnter*, kInputEsc*,
                                // kInputHome/End, kInputCatFirst/Last
#include "hotkeys.h"            // ShiftHeld
#include "log.h"
#include "strings.h"
#include "speech.h"

namespace acc::unified_menu {

bool HandleInputEvent(int code, int value) {
    if (!g.active) return false;
    if (value == 0) return false;

    // The medical party picker is modal while up — it owns every key,
    // including Esc (which backs out to the entry, not out of the menu).
    if (g.med.active) return HandleMedPickerInput(code);

    if (code == kInputEsc1 || code == kInputEsc2) {
        CloseFromEsc();
        return true;
    }

    void* tam = acc::engine_radial::ResolveTargetActionMenu();
    void* mi  = acc::engine_actionbar::ResolveMainInterface();

    // Follow-cycling: the narrated target may have changed while the menu
    // sat open. Re-anchor onto it (menu already had a target block) or
    // unfold one in (menu was personal-only). rowCarried[] records, per
    // target row, whether the user's selected action_id survived the move —
    // it gates Enter into orientation mode below.
    bool targetChanged = false;
    bool selectionCarried = true;
    bool rowCarried[kRowCount] = {true, true, true};
    if (tam) {
        uint32_t narrated = ResolveNarratedServerHandle();
        const bool haveBlock = g.hasTargetBlock && g.targetHandle != 0;
        if (narrated != 0 && haveBlock &&
            (narrated & ~0x80000000u) != (g.targetHandle & ~0x80000000u)) {
            targetChanged = ReanchorToNewTarget(tam, narrated, rowCarried);
        } else if (narrated != 0 && !haveBlock &&
                   narrated != g.unfoldDeclined) {
            targetChanged = UnfoldTargetBlock(tam, narrated, rowCarried);
        }
    }

    MaybeRestoreDrainedRows(tam, targetChanged);

    // Rebuild the category list (target rows may have changed on re-anchor)
    // and re-locate the cursor on the same category, clamping if it drained.
    // Personal-only menus pass tam=nullptr so the target rows never get
    // folded back in mid-navigation.
    CatKind savedKind = g.cats[g.curCat].kind;
    int     savedSlot = g.cats[g.curCat].slot;
    BuildCategoryList(g.hasTargetBlock ? tam : nullptr, mi);
    if (g.catCount == 0) {
        char msg[160];
        std::snprintf(msg, sizeof(msg),
                      acc::strings::Get(acc::strings::Id::FmtInteractNoActions),
                      g.targetName);
        acc::speech::Speak(msg, /*interrupt=*/true);
        acclog::Write("UnifiedMenu", "all categories empty after reanchor; disarming");
        ForceDisarm("empty");
        return true;
    }
    int loc = LocateCat(savedKind, savedSlot);
    g.curCat = (loc >= 0) ? loc : ClampInt(g.curCat, 0, g.catCount - 1);

    // Selection survives a follow-cycle re-anchor only when the category the
    // user was on still exists AND (for target rows) its action_id was found
    // on the new target. Not-carried gates Enter into orientation mode.
    if (targetChanged) {
        const Cat& c = g.cats[g.curCat];
        selectionCarried = (loc >= 0) &&
            (c.kind != CatKind::Target || rowCarried[c.slot]);
    }

    Cat cur = g.cats[g.curCat];
    int count = CountForCat(tam, mi, cur);
    int& sel = ShadowFor(cur);
    sel = ClampInt(sel, 0, count > 0 ? count - 1 : 0);
    ApplySelection(tam, mi, cur, sel);

    if ((code == kInputNavUp || code == kInputNavDown ||
         code == kInputNavLeft || code == kInputNavRight) &&
        acc::hotkeys::ShiftHeld()) {
        DescribeSelectedEntry(tam, mi, cur, sel);
        return true;
    }

    switch (code) {
        case kInputNavLeft:
        case kInputNavRight: {
            int dir = (code == kInputNavRight) ? +1 : -1;
            int nx = g.curCat + dir;
            if (nx < 0 || nx >= g.catCount) {
                // Clamp — re-speak the current category as the edge cue.
                acclog::Write("UnifiedMenu", "%s at edge cat=%d/%d",
                    dir > 0 ? "Right" : "Left", g.curCat, g.catCount);
                SpeakCategory(tam, mi, /*prefix=*/nullptr);
                return true;
            }
            g.curCat = nx;
            SpeakCategory(tam, mi, /*prefix=*/nullptr);
            return true;
        }
        case kInputCatFirst:
        case kInputCatLast: {
            g.curCat = (code == kInputCatFirst) ? 0 : g.catCount - 1;
            SpeakCategory(tam, mi, /*prefix=*/nullptr);
            return true;
        }
        case kInputNavUp:
        case kInputNavDown: {
            // Up = previous entry (toward Home), Down = next (toward End).
            // Clamp at both ends; the repeated label is the edge cue.
            if (count > 1) {
                int dir = (code == kInputNavDown) ? +1 : -1;
                sel = ClampInt(sel + dir, 0, count - 1);
                ApplySelection(tam, mi, cur, sel);
            }
            SpeakEntry(tam, mi);
            return true;
        }
        case kInputHome:
        case kInputEnd: {
            if (count > 0) {
                sel = (code == kInputHome) ? 0 : count - 1;
                ApplySelection(tam, mi, cur, sel);
            }
            SpeakEntry(tam, mi);
            return true;
        }
        case kInputEnter1:
        case kInputEnter2:
            return HandleEnter(tam, mi, cur, sel, targetChanged,
                               selectionCarried);
        default:
            return false;
    }
}

}  // namespace acc::unified_menu
