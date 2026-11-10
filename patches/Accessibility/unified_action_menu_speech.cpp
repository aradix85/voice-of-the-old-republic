// What the unified action menu says: a category with its selected entry, an
// entry, an item's quantity, and an entry's description.

#include "unified_action_menu_internal.h"

#include "engine_reads.h"       // ResolveActionDescriptionFromActionId
#include "log.h"
#include "menus_speak.h"
#include "strings.h"
#include "speech.h"

namespace acc::unified_menu {

namespace {

// Append ", N in stack" / ", N charges" to an item entry's label, matching the
// inventory / store suffixes. Resolves the CSWSItem behind the entry's
// item-tagged action_id; no-op for non-item entries (attacks, force powers)
// and single non-charged items. Charged items can't stack, so the two
// suffixes never both fire.
void AppendItemQuantity(void* tam, void* mi, const Cat& c, int idx,
                        char* label, size_t n) {
    void* item = acc::engine::ItemFromActionId(ReadActionId(tam, mi, c, idx));
    if (!item) return;
    char suffix[64] = "";
    int charges = acc::engine::ReadItemCharges(item);
    if (charges >= 0) {
        std::snprintf(suffix, sizeof(suffix),
                      acc::strings::Get(acc::strings::Id::FmtItemChargeSuffix),
                      charges);
    } else {
        int stack = acc::engine::ReadItemStack(item);
        if (stack > 1) {
            std::snprintf(suffix, sizeof(suffix),
                          acc::strings::Get(acc::strings::Id::FmtItemStackSuffix),
                          stack);
        }
    }
    if (!suffix[0]) return;
    size_t len = strnlen(label, n);
    if (len < n) std::snprintf(label + len, n - len, ", %s", suffix);
}

// Build the full category announce ("Name: label, N options" / "label, N
// options" for unnamed rows) into `out`. ApplySelection must have run.
void FormatCategory(void* tam, void* mi, const Cat& c, int count, int idx,
                    char* out, size_t n) {
    char label[128] = "";
    ReadLabel(tam, mi, c, idx, label, sizeof(label));
    AppendItemQuantity(tam, mi, c, idx, label, sizeof(label));
    const char* lbl = label[0] ? label : "?";
    const char* name = CategoryName(c);
    using S = acc::strings::Id;
    if (name && name[0]) {
        if (count > 1) {
            std::snprintf(out, n, acc::strings::Get(S::FmtMenuCatMulti),
                          name, lbl, count);
        } else {
            std::snprintf(out, n, acc::strings::Get(S::FmtMenuCatSingle),
                          name, lbl);
        }
    } else {
        if (count > 1) {
            std::snprintf(out, n, acc::strings::Get(S::FmtMenuPlainMulti),
                          lbl, count);
        } else {
            std::snprintf(out, n, "%s", lbl);
        }
    }
}

}  // namespace

// Speak the current category in full. `prefix` (e.g. "action menu, X") is
// prepended with ". " when non-null/non-empty (Shift+Enter open only).
void SpeakCategory(void* tam, void* mi, const char* prefix) {
    Cat& c = g.cats[g.curCat];
    int count = CountForCat(tam, mi, c);
    int& sel = ShadowFor(c);
    sel = ClampInt(sel, 0, count > 0 ? count - 1 : 0);
    ApplySelection(tam, mi, c, sel);

    char cat[256] = "";
    FormatCategory(tam, mi, c, count, sel, cat, sizeof(cat));

    char msg[320];
    if (prefix && prefix[0]) {
        std::snprintf(msg, sizeof(msg), "%s. %s", prefix, cat);
    } else {
        std::snprintf(msg, sizeof(msg), "%s", cat);
    }
    acc::speech::Speak(msg, /*interrupt=*/true);
    acclog::Write("UnifiedMenu", "speak cat=%d kind=%s slot=%d count=%d idx=%d "
        "-> [%s]", g.curCat,
        c.kind == CatKind::Target ? "target" : "personal",
        c.slot, count, sel, msg);
}

// Speak only the current entry label — used after Up/Down/Home/End (the
// category didn't change, so its name would be redundant noise).
void SpeakEntry(void* tam, void* mi) {
    Cat& c = g.cats[g.curCat];
    int& sel = ShadowFor(c);
    char label[128] = "";
    ReadLabel(tam, mi, c, sel, label, sizeof(label));
    AppendItemQuantity(tam, mi, c, sel, label, sizeof(label));
    acc::menus::speak::SpeakChoice("UnifiedMenu", label,
                                 "entry cat=%d slot=%d idx=%d",
                                 g.curCat, c.slot, sel);
}

// Shift + any arrow — describe the selected entry, no movement.
void DescribeSelectedEntry(void* tam, void* mi, const Cat& cur, int sel) {
    uint32_t actionId = ReadActionId(tam, mi, cur, sel);
    char text[8192];
    if (actionId &&
        acc::engine::ResolveActionDescriptionFromActionId(
            actionId, text, sizeof(text))) {
        acc::speech::Speak(text, /*interrupt=*/true);
        acclog::Write("UnifiedMenu", "Shift+nav action_id=0x%x desc=\"%s\"",
            actionId, text);
    } else {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::NoTooltipAvailable),
                     /*interrupt=*/true);
        acclog::Write("UnifiedMenu", "Shift+nav action_id=0x%x no desc",
            actionId);
    }
}

}  // namespace acc::unified_menu
