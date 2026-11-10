// Text of the plain GUI controls: button, toggle button, label, slider
// (with its sibling label), edit box, single-row list box, item entry row,
// and a label beside a cycle control's arrows.

#include "menus_extract_internal.h"

#include <cstdlib>

#include "log.h"
#include "strings.h"

namespace acc::menus::extract {

namespace {

using acc::strings::Get;
using acc::strings::Id;

// The panel's controls array, walked at most 256 entries deep.
struct Controls {
    void** data;
    int    count;
};

Controls PanelControls(void* panel) {
    if (!panel) return {nullptr, 0};
    auto* list = reinterpret_cast<CExoArrayList*>(
        static_cast<unsigned char*>(panel) + kPanelControlsOffset);
    if (!list->data || list->size <= 0) return {nullptr, 0};
    return {list->data, list->size > 256 ? 256 : list->size};
}

// True for an image-only button flanking a cycle's value button on the
// same row (the arrows around "Normal" in the Difficulty cycle): no text
// of its own, and a button with text within FindAdjacentArrow's reach.
// Such an arrow must not borrow the nearest label, which IS the value
// display: that would paint both flankers and defeat the chain squash and
// FindAdjacentArrow.
bool IsCycleFlankerArrow(void* panel, void* control) {
    char text[64];
    if (!CallDowncast(control, kVtableAsButton) ||
        ReadButtonText(control, text, sizeof(text))) {
        return false;
    }
    int focusCx, focusCy;
    if (!GetControlCenter(control, focusCx, focusCy)) return false;

    // .gui authoring units, see ScaleGuiThresholdPx.
    const int kSameRowDyTol = ScaleGuiThresholdPx(5);
    const int kMaxDx        = ScaleGuiThresholdPx(80);  // FindAdjacentArrow's reach

    const Controls all = PanelControls(panel);
    for (int i = 0; i < all.count; ++i) {
        void* c = all.data[i];
        if (!c || c == control || !CallDowncast(c, kVtableAsButton)) continue;
        int cx, cy;
        if (!GetControlCenter(c, cx, cy)) continue;
        const int adx = std::abs(cx - focusCx);
        if (std::abs(cy - focusCy) > kSameRowDyTol || adx == 0 || adx > kMaxDx) {
            continue;
        }
        if (ReadButtonText(c, text, sizeof(text))) return true;
    }
    return false;
}

// The text of the closest captioned label beside `control`: on the same
// row (left preferred, right as a fallback for trailing captions) or
// above or below it with a similar x (the Sound panel's label over each
// slider, the in-game menu's caption under each icon), scored by Manhattan
// distance. Empty labels are skipped, so a closer decorative frame cannot
// hide the real caption (K2 optsound_p.gui: LBL_BAR4 next to SLI_FX).
//
// The box is in .gui authoring units (ScaleGuiThresholdPx): too small and
// a control speaks as "control N", too big and the panel title pairs.
bool FindSiblingLabel(void* panel, void* control, char* outBuf, size_t bufSize) {
    int targetCx, targetCy;
    if (bufSize < 2 || !GetControlCenter(control, targetCx, targetCy)) return false;

    const int kSameRowDyTol = ScaleGuiThresholdPx(5);
    const int kVertDyMax    = ScaleGuiThresholdPx(50);
    const int kVertDxMax    = ScaleGuiThresholdPx(80);

    int  bestScore = 0x7fffffff;
    bool found = false;
    const Controls all = PanelControls(panel);
    for (int i = 0; i < all.count; ++i) {
        void* c = all.data[i];
        if (!c || c == control) continue;
        if (!CallDowncast(c, kVtableAsLabel) &&
            !CallDowncast(c, kVtableAsLabelHilight)) {
            continue;
        }
        int cx, cy;
        if (!GetControlCenter(c, cx, cy)) continue;
        const int absDx = std::abs(cx - targetCx);
        const int absDy = std::abs(cy - targetCy);
        int score;
        if (absDy <= kSameRowDyTol) {
            score = cx < targetCx ? absDx : absDx + 8;  // left wins ties
        } else if (absDy <= kVertDyMax && absDx <= kVertDxMax) {
            score = absDx + absDy;
        } else {
            continue;
        }
        if (score >= bestScore) continue;
        char candidate[256];  // the largest caller buffer: no truncation here
        if (!ReadLabelText(c, candidate, sizeof(candidate))) continue;
        bestScore = score;
        found = true;
        strncpy_s(outBuf, bufSize, candidate, _TRUNCATE);
    }
    return found;
}

// The movie-volume slider on KOTOR 1's optionssound.gui, whose stock label
// repeats the music slider's ("music volume"). Fingerprint: sliders with
// .gui ids 1, 4, 7 and 8 (music, voice, SFX, movie; stable across
// locales), `control` being id 8. KOTOR 2 numbers them 2, 5, 8, 9 and
// labels the movie slider correctly, so this never matches there.
bool IsSoundOptionsMovieSlider(void* panel, void* control) {
    if (!IsSlider(control) || ReadControlId(control) != 8) return false;
    constexpr unsigned kAll = (1u << 1) | (1u << 4) | (1u << 7) | (1u << 8);
    unsigned seen = 0;
    const Controls all = PanelControls(panel);
    for (int i = 0; i < all.count; ++i) {
        void* c = all.data[i];
        if (!c || !IsSlider(c)) continue;
        const int id = ReadControlId(c);
        if (id >= 0 && id < 32) seen |= (1u << id) & kAll;
    }
    return seen == kAll;
}

// `tag` when `read` finds text on the control's downcast at `vtableIndex`.
const char* TryAs(void* control, int vtableIndex,
                  bool (*read)(void*, char*, size_t), const char* tag,
                  char* outBuf, size_t bufSize) {
    void* typed = CallDowncast(control, vtableIndex);
    return typed && read(typed, outBuf, bufSize) ? tag : nullptr;
}

// A list-box row's own text: the inline string or the strref of the row
// as a button, label or highlight label.
bool ReadRowText(void* row, char* outBuf, size_t bufSize) {
    struct Kind {
        int    vtableIndex;
        size_t textOffset;
        size_t strRefOffset;
    };
    const Kind kKinds[] = {
        {kVtableAsButton, kButtonTextOffset, kButtonStrRefOffset},
        {kVtableAsLabel, kLabelTextOffset, kLabelStrRefOffset},
        {kVtableAsLabelHilight, kLabelTextOffset, kLabelStrRefOffset},
    };
    for (const Kind& k : kKinds) {
        void* typed = CallDowncast(row, k.vtableIndex);
        if (typed && ExtractTextOrStrRef(typed, k.textOffset, k.strRefOffset,
                                         outBuf, bufSize)) {
            return true;
        }
    }
    return false;
}

}  // namespace

// CSWGuiButton, which also covers CharButton, ActivatedButton and
// ButtonToggle: they embed Button at offset 0 and AsButton returns them.
const char* TryButton(void* control, char* outBuf, size_t bufSize) {
    return TryAs(control, kVtableAsButton, ReadButtonText, "button", outBuf, bufSize);
}

// CSWGuiButtonToggle, in case AsButton misses one (same text offsets).
const char* TryButtonToggle(void* control, char* outBuf, size_t bufSize) {
    return TryAs(control, kVtableAsButtonToggle, ReadButtonText, "buttontoggle",
                 outBuf, bufSize);
}

const char* TryLabel(void* control, char* outBuf, size_t bufSize) {
    return TryAs(control, kVtableAsLabel, ReadLabelText, "label", outBuf, bufSize);
}

// CSWGuiLabelHilight embeds Label at offset 0: same text offsets.
const char* TryLabelHilight(void* control, char* outBuf, size_t bufSize) {
    return TryAs(control, kVtableAsLabelHilight, ReadLabelText, "labelhilight",
                 outBuf, bufSize);
}

// CSWGuiSlider, found by vtable (there is no AsSlider). Its own string is
// the rendered "X of Y"; the category name is a sibling label. `owner` is
// FromControl's ResolveOwnerPanel result, not g_currentPanel: a modal
// pushed without SetActiveControl (KOTOR 2's sound options over the
// options screen) leaves that global on the panel behind, whose controls
// hold no label near the slider.
const char* TrySlider(void* control, void* owner, char* outBuf, size_t bufSize) {
    if (!IsSlider(control)) return nullptr;
    const uint32_t cur = ReadU32(control, kSliderCurValueOffset);
    const uint32_t max = ReadU32(control, kSliderMaxValueOffset);
    char label[128];
    const char* labelText = nullptr;
    if (owner && IsSoundOptionsMovieSlider(owner, control)) {
        labelText = Get(Id::SoundOptionsMovieVolume);
    } else if (owner && FindSiblingLabel(owner, control, label, sizeof(label))) {
        labelText = label;
    }
    if (labelText) {
        snprintf(outBuf, bufSize, Get(Id::FmtSliderValueLabeled), labelText, cur, max);
    } else {
        snprintf(outBuf, bufSize, Get(Id::FmtSliderValue), cur, max);
    }
    return "slider";
}

// CSWGuiEditbox (chargen Name and the save-name popup), found by vtable:
// "{role}. {value}" on focus. menus_editbox's per-tick poll speaks the
// edits after that.
const char* TryEditbox(void* control, char* outBuf, size_t bufSize) {
    if (!IsEditbox(control)) return nullptr;
    char text[128];
    uint32_t len = 0;
    ReadEditboxText(control, text, sizeof(text), len);
    snprintf(outBuf, bufSize, "%s. %s", Get(Id::EditboxRole),
             len > 0 ? text : Get(Id::EditboxEmpty));
    return "editbox";
}

// A list box with exactly one row: many modals (OK/Cancel boxes, the quit
// confirmation, tutorial popups) put their message there. Multi-row list
// boxes speak nothing here: their focus would read every row as one blob,
// and row navigation already reads them one at a time.
const char* TrySingleRowListBox(void* control, char* outBuf, size_t bufSize) {
    if (!IsListBox(control)) return nullptr;
    auto* rows = reinterpret_cast<CExoArrayList*>(
        static_cast<unsigned char*>(control) + kListBoxControlsOffset);
    if (!rows->data || rows->size != 1 || !rows->data[0]) return nullptr;
    // kAnnounceTextMax: the row is the whole message, and tutorial popups
    // run far past 256 bytes. A cut one also stops matching its full TLK
    // copy in tutorial_hints and loses its keyboard hint.
    char rowText[kAnnounceTextMax];
    if (!ReadRowText(rows->data[0], rowText, sizeof(rowText))) return nullptr;
    const size_t len = strnlen(rowText, sizeof(rowText));
    if (len == 0 || len + 1 >= bufSize) return nullptr;
    memcpy(outBuf, rowText, len + 1);
    return "listbox";
}

// An inventory, container or store row named from its ITEM when the row's
// own string is empty. Runs before the sibling-label fallback on purpose:
// a row's extent is list-box-local, so a spatial search can only pair it
// with an unrelated label (K2 captioned an item "Inventory" that way).
const char* TryItemEntryRow(void* control, char* outBuf, size_t bufSize) {
    if (!acc::engine::ReadItemRowName(control, outBuf, bufSize)) {
        return nullptr;
    }
    if (outBuf[0] == '\0') return nullptr;
    acclog::Trace("Menus.ItemRow",
                  "control=%p named from item: \"%s\"", control, outBuf);
    return "itemrow-name";
}

// Last resort for a chain-navigable control without text, such as the
// in-game menu's image-only icons: speak the nearest label as its name.
// - `owner` is the panel that holds the control, not g_currentPanel: KOTOR
//   2's chargen wizard steps are pushed without SetActiveControl, and the
//   step list behind them once captioned a stepper with a step number.
// - Only panel-direct children: a list-box row's extent is list-box-local,
//   so matching it against panel labels pairs whatever sits top-left.
// - Not for cycle flanker arrows (IsCycleFlankerArrow).
const char* TrySiblingLabel(void* control, void* owner, char* outBuf, size_t bufSize) {
    if (!owner || !IsChainNavigable(control) ||
        !IsPanelDirectChild(owner, control) || IsCycleFlankerArrow(owner, control)) {
        return nullptr;
    }
    char label[256];
    if (!FindSiblingLabel(owner, control, label, sizeof(label)) ||
        !EmitText(outBuf, bufSize, label)) {
        return nullptr;
    }
    // Trace: fires for every chain entry on every arrow press.
    acclog::Trace("Menus.SiblingFallback", "control=%p label=\"%s\"", control, outBuf);
    return "siblinglabel-fallback";
}

}  // namespace acc::menus::extract
