// Text of the plain GUI controls: tooltip, button, toggle button, label,
// slider (with its sibling label), edit box, single-row list box, item
// entry row, and a label beside a cycle control's arrows.

#include "menus_extract_internal.h"

#include "log.h"
#include "strings.h"

namespace acc::menus::extract {

namespace {

// True for image-only buttons that flank a cycle value-display button at
// the same y-row (e.g. the [◀]/[▶] arrows around `Normal` in the
// Difficulty cycle). Suppresses sibling-label fallback so the cycle
// mechanism's empty-text predicate engages — without this gate, the
// nearest plain label (which IS the value display) would paint both
// flankers, defeating the chain squash and FindAdjacentArrow.
//
// Heuristic:
//   - self has empty inline text (real cycle value buttons fail this);
//   - some sibling button at the same y-row within ±80 px on x has
//     non-empty text (the value-display);
//   - self is downcastable to AsButton (sliders / labels / listboxes
//     don't reach here — those announce paths run earlier in the ladder).
//
// Used only by FromControl; co-located here, not in any chain-side TU.
// CSWGuiClassSelection icons are positionally separate and handled by
// IsClassSelectionIcon; chargen wizard step decorations are CSWGuiLabel-
// Hilight, not buttons. CSWGuiSlider category resolution uses
// FindSiblingLabel against a slider control, never a button, so this
// predicate never fires there either.
bool IsCycleFlankerArrow(void* panel, void* control) {
    if (!panel || !control) return false;
    if (CallDowncast(control, kVtableAsButton) == nullptr) return false;

    // Self must have empty own text — real cycle value buttons (carrying
    // "Normal", "8", "Off", …) keep the sibling-label fallback off the
    // table anyway, so this check guards the "image-only" classification.
    char selfTxt[64];
    if (ExtractTextOrStrRefIndirect(control,
                                    kButtonTextOffset,
                                    kButtonStrRefOffset,
                                    kButtonTextObjectOffset,
                                    selfTxt, sizeof(selfTxt))) {
        return false;
    }

    int focusCx, focusCy;
    if (!GetControlCenter(control, focusCx, focusCy)) return false;

    auto* list = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
    if (!list || !list->data || list->size <= 0) return false;

    // .gui authoring units — see ScaleGuiThresholdPx in menus_internal.h
    // for why a bare pixel constant is a KOTOR 1 assumption.
    const int kSameRowDyTol = ScaleGuiThresholdPx(5);
    const int kMaxDxPx      = ScaleGuiThresholdPx(80);  // FindAdjacentArrow's reach

    int n = list->size > 256 ? 256 : list->size;
    for (int i = 0; i < n; ++i) {
        void* c = list->data[i];
        if (!c || c == control) continue;
        if (CallDowncast(c, kVtableAsButton) == nullptr) continue;

        int cx, cy;
        if (!GetControlCenter(c, cx, cy)) continue;
        int dy = cy - focusCy;
        if (dy < -kSameRowDyTol || dy > kSameRowDyTol) continue;
        int dx = cx - focusCx;
        int adx = dx < 0 ? -dx : dx;
        if (adx == 0 || adx > kMaxDxPx) continue;

        char nbrTxt[64];
        if (ExtractTextOrStrRefIndirect(c,
                                        kButtonTextOffset,
                                        kButtonStrRefOffset,
                                        kButtonTextObjectOffset,
                                        nbrTxt, sizeof(nbrTxt))) {
            return true;
        }
    }
    return false;
}

// Find the closest label-like sibling of `control` on the panel. Two
// candidate positions:
//
//   1. Same y-row, to the visual LEFT (horizontal layouts, e.g. labelled
//      buttons on a config row — `[Close]   Default`).
//   2. Directly ABOVE the control (vertical layouts, e.g. KOTOR's Sound
//      panel where each slider has a label rendered on the line above it
//      rather than to its left).
//
// Picks the candidate with the smallest scoring distance: Manhattan
// distance for "above" candidates so a label slightly off-axis but close
// in y wins over a same-row label that's far to the left. Strict
// thresholds keep us from matching the panel title (which is far above
// any individual widget) or unrelated controls in adjacent rows.
//
// Returns the source tag ("siblinglabel") and writes the label's text
// into outBuf, or nullptr if no suitable label was found. Pure read;
// doesn't recurse through FromControl so it's safe to call from inside
// extraction code.
const char* FindSiblingLabel(void* panel, void* control,
                             char* outBuf, size_t bufSize) {
    if (!panel || !control || bufSize < 2) return nullptr;

    auto* list = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
    if (!list->data || list->size <= 0) return nullptr;

    int targetCx, targetCy;
    if (!GetControlCenter(control, targetCx, targetCy)) return nullptr;

    int bestScore = 0x7fffffff;
    bool haveBest = false;

    // Tolerances. Same-row dy<=5; vertical offset (above OR below) up to
    // 50 px (typical KOTOR row height is ~30-55 px) with dx tolerance 80
    // px (label can be slightly offset from the widget center, e.g.
    // left-aligned label vs centered slider). Search expanded vs the
    // original same-row-left + above-only set so InGameMenu-style
    // captioned icons (label below the button) also match.
    //
    // All three are .gui authoring units and go through
    // ScaleGuiThresholdPx — see menus_internal.h. This is the site where
    // getting it wrong is most expensive: a label that falls outside the
    // box is a control that speaks as "control N" instead of by name, and
    // on KOTOR 2 an unscaled 50-unit vertical reach rejects any caption
    // more than ~14 authored units from its widget.
    const int kSameRowDyTol  = ScaleGuiThresholdPx(5);
    const int kVertDyMax     = ScaleGuiThresholdPx(50);
    const int kVertDxMax     = ScaleGuiThresholdPx(80);

    int n = list->size > 256 ? 256 : list->size;
    for (int i = 0; i < n; ++i) {
        void* c = list->data[i];
        if (!c || c == control) continue;

        if (CallDowncast(c, kVtableAsLabel) == nullptr &&
            CallDowncast(c, kVtableAsLabelHilight) == nullptr) {
            continue;
        }

        int cx, cy;
        if (!GetControlCenter(c, cx, cy)) continue;

        int dy    = cy - targetCy;          // negative = label is above
        int adx   = cx - targetCx;          // negative = label is to the left
        int absDx = adx < 0 ? -adx : adx;
        int absDy = dy  < 0 ? -dy  : dy;

        int score = 0x7fffffff;
        // Same row — favour LEFT siblings (existing behaviour for slider
        // labels) but also accept right-of-target as a fallback so we
        // don't reject all icon-with-trailing-caption layouts.
        if (absDy <= kSameRowDyTol) {
            // Left has score = absDx (lower is better);
            // right gets a small penalty so left wins on ties.
            score = (cx < targetCx) ? absDx : (absDx + 8);
        }
        // Vertically displaced (above OR below) with similar x. Below-target
        // captioned-icon layouts (label below the icon button) need the
        // dy>0 case too. Manhattan distance scoring keeps the closest
        // candidate winning when multiple labels could pair.
        else if (absDy <= kVertDyMax && absDx <= kVertDxMax) {
            score = absDx + absDy;
        }
        else {
            continue;
        }

        if (score >= bestScore) continue;

        // Read the candidate's text HERE rather than once at the end, and
        // only keep it if it actually carries some. Picking the geometric
        // winner first and reading it afterwards loses the whole lookup to
        // any closer-but-empty decorative label — KOTOR 2's optsound_p.gui
        // is exactly that case: LBL_BAR4 is a 318x329 empty-text frame whose
        // centre lands 21px from SLI_FX, beating LBL_FX at 75px, so the FX
        // slider came back unlabelled while its neighbours resolved fine.
        // Skipping empty candidates lets the next-best labelled one win.
        // Sized to the largest caller buffer (TrySiblingLabel's label[256])
        // so routing through the staging copy can't truncate what a direct
        // write into outBuf would have kept.
        char candidate[256];
        if (!ExtractTextOrStrRefIndirect(c,
                                         kLabelTextOffset,
                                         kLabelStrRefOffset,
                                         kLabelTextObjectOffset,
                                         candidate, sizeof(candidate)) ||
            candidate[0] == '\0') {
            continue;
        }

        bestScore = score;
        haveBest  = true;
        strncpy_s(outBuf, bufSize, candidate, _TRUNCATE);
    }

    return haveBest ? "siblinglabel" : nullptr;
}

// Identify the movie-volume slider on optionssound.gui. The stock German
// .gui mislabels it as "music volume" (same string as the music
// slider above it), so FindSiblingLabel returns a duplicate; we swap in
// a synthesised "Movie volume" instead. We don't
// ship a .gui override, so the fix lives in the screen-reader path only.
//
// Fingerprint: the panel must carry sliders at .gui control IDs
// {1, 4, 7, 8} (music / voice / SFX / movie). IDs come from the .gui
// file in data/gui.bif and are stable across locales — unlike the label
// strrefs that the engine renders. `control` must itself be the id=8
// slider.
//
// KOTOR 1 only, and the fingerprint enforces that on its own: K2's
// optsound_p.gui numbers its sliders {2,5,8,9}, so has1/has4/has7 can never
// all be true there. Nothing to port — K2 gives LBL_MOVIE its own strref
// ("Movie volume"), so the duplicate-label bug this works around simply
// doesn't exist in that game.
bool IsSoundOptionsMovieSlider(void* panel, void* control) {
    if (!panel || !control) return false;
    if (!IsSlider(control)) return false;

    if (ReadControlId(control) != 8) return false;

    auto* list = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
    if (!list || !list->data || list->size <= 0) return false;

    bool has1 = false, has4 = false, has7 = false, has8 = false;
    int n = list->size > 64 ? 64 : list->size;
    for (int i = 0; i < n; ++i) {
        void* c = list->data[i];
        if (!c || !IsSlider(c)) continue;
        switch (ReadControlId(c)) {
            case 1: has1 = true; break;
            case 4: has4 = true; break;
            case 7: has7 = true; break;
            case 8: has8 = true; break;
        }
    }
    return has1 && has4 && has7 && has8;
}

}  // namespace

// 1. Tooltip on the base class — works for any control that has one.
//    SEH-wrapped: the field at +0x28 holds a `char*` that on a stale
//    (freed-and-reused) control can be a bogus address; the memcpy
//    would then fault reading the source. CallDowncast already SEH-
//    protects steps 2-5; this covers the single read path that doesn't
//    go through it.
//
//    This step was UNGATED until Phase-3 B2 — it ran even when step 0 had
//    already produced a per-kind phrase, so a control carrying a tooltip
//    would lose its composed text ("Strength 14, +2" replaced by the raw
//    field), and a faulting read cleared step 0's result outright. Step 0
//    was added above this one years later, precisely so the formatted
//    phrase wins; the `!source` gate it needed was never added here.
//
//    Nothing observable changed when the gate went in: across every patch
//    log in the install, all 9202 Menus.FocusChange samples read
//    tip[0]="" and no chain entry has ever been tagged src=tooltip. K1
//    appears not to populate the field at all, so step 0 was safe by
//    accident of the data rather than by construction. It is safe by
//    construction now.
const char* TryTooltip(void* control, char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    __try {
        const char* tip;
        uint32_t    tipLen;
        int         id;
        if (ReadControlNameFields(control, tip, tipLen, id) &&
            tipLen > 0 && tipLen < bufSize) {
            memcpy(outBuf, tip, tipLen);
            outBuf[tipLen] = '\0';
            source = "tooltip";
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        source = nullptr;
    }
    return source;
}

// 2. CSWGuiButton (most common — also covers CharButton, ActivatedButton,
//    ButtonToggle since those embed Button at offset 0 AND the engine's
//    AsButton override returns `this` for them). Tries inline CExoString,
//    then TLK str_ref, then text_object indirection — the last covers
//    classes whose text routes through CSWGuiText.text_params.text_object
//    rather than the inline CExoString.
const char* TryButton(void* control, char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (void* btn = CallDowncast(control, kVtableAsButton)) {
        if (ExtractTextOrStrRefIndirect(btn,
                                        kButtonTextOffset,
                                        kButtonStrRefOffset,
                                        kButtonTextObjectOffset,
                                        outBuf, bufSize)) {
            source = "button";
        }
    }
    return source;
}

// 3. CSWGuiButtonToggle — defensive fallback if AsButton misses it.
//    Same offsets because ButtonToggle.button is at offset 0.
const char* TryButtonToggle(void* control, char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (void* tgl = CallDowncast(control, kVtableAsButtonToggle)) {
        if (ExtractTextOrStrRefIndirect(tgl,
                                        kButtonTextOffset,
                                        kButtonStrRefOffset,
                                        kButtonTextObjectOffset,
                                        outBuf, bufSize)) {
            source = "buttontoggle";
        }
    }
    return source;
}

// 4. CSWGuiLabel.
const char* TryLabel(void* control, char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (void* lbl = CallDowncast(control, kVtableAsLabel)) {
        if (ExtractTextOrStrRefIndirect(lbl,
                                        kLabelTextOffset,
                                        kLabelStrRefOffset,
                                        kLabelTextObjectOffset,
                                        outBuf, bufSize)) {
            source = "label";
        }
    }
    return source;
}

// 5. CSWGuiLabelHilight — same offsets (Label embedded at 0).
const char* TryLabelHilight(void* control, char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (void* hil = CallDowncast(control, kVtableAsLabelHilight)) {
        if (ExtractTextOrStrRefIndirect(hil,
                                        kLabelTextOffset,
                                        kLabelStrRefOffset,
                                        kLabelTextObjectOffset,
                                        outBuf, bufSize)) {
            source = "labelhilight";
        }
    }
    return source;
}

// 6. CSWGuiSlider — no AsSlider downcast accessor exists; detect by
//    vtable identity. cur_value / max_value are Lane-named uint32 fields.
//    The slider widget itself has no inline category text (its CExoString
//    is the rendered "X of Y"); the category name lives on a sibling
//    CSWGuiLabel rendered to the left of the slider. Look it up via
//    FindSiblingLabel and prepend.
//
//    Engine-text fix-up for the Sound options panel's movie-volume
//    slider (id=8): the stock German .gui labels both the music slider
//    and the movie slider as "music volume", so FindSiblingLabel
//    resolves a duplicate. When the slider sits on a panel whose ID
//    fingerprint matches optionssound.gui ({1,4,7,8} — locale-stable),
//    we substitute the localized "Video volume" string for the label.
//
//    `owner` is FromControl's ResolveOwnerPanel result, for the same reason
//    TrySiblingLabel (step 10) takes it: the g_currentPanel global this used
//    to read is the panel that last fired SetActiveControl, which is NOT the
//    panel holding the control whenever one is pushed on top without firing
//    it. KOTOR 2's Soundoptionen is exactly that — a modal over the options
//    screen, with g_currentPanel still on the InGameOptions panel behind it
//    (patch-20260804-191607.log: `Routing: fg=333D2C60 current=31EB85D8`).
//    FindSiblingLabel then walked the wrong panel's controls, found no label
//    near the slider, and all four volume sliders announced as a bare
//    "85 of 100".
const char* TrySlider(void* control, void* owner,
                      char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (IsSlider(control)) {
        uint32_t cur = ReadU32(control, kSliderCurValueOffset);
        uint32_t max = ReadU32(control, kSliderMaxValueOffset);
        char label[128];
        const char* labelText = nullptr;
        // Liveness-filtered by ResolveOwnerPanel — both callees walk
        // panel+kPanelControlsOffset, which a freed panel would fault on.
        void* panel = owner;
        if (panel && IsSoundOptionsMovieSlider(panel, control)) {
            labelText = acc::strings::Get(
                acc::strings::Id::SoundOptionsMovieVolume);
        } else if (panel &&
                   FindSiblingLabel(panel, control,
                                    label, sizeof(label))) {
            labelText = label;
        }
        if (labelText) {
            snprintf(outBuf, bufSize,
                     acc::strings::Get(acc::strings::Id::FmtSliderValueLabeled),
                     labelText, cur, max);
        } else {
            snprintf(outBuf, bufSize,
                     acc::strings::Get(acc::strings::Id::FmtSliderValue),
                     cur, max);
        }
        source = "slider";
    }
    return source;
}

// 6b. CSWGuiEditbox — same vtable-identity pattern as slider (no AsEditbox
//     accessor exists). Two vanilla editboxes match (IsEditbox accepts both
//     vtables): the chargen Name `name_editbox` and the save-name popup's
//     CSWGuiSaveGameEditBox. Speech format: "{role}. {value}" so
//     the screen-reader user immediately knows they've landed in an input
//     field plus the current contents. The per-tick poll in
//     menus_editbox.cpp owns subsequent text-change announcements (single
//     char on insert/delete, full re-read on Up/Down or Random-button
//     replacement) — this branch only handles the focus-enter announce.
const char* TryEditbox(void* control, char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (IsEditbox(control)) {
        const char* role  = acc::strings::Get(acc::strings::Id::EditboxRole);
        const char* empty = acc::strings::Get(acc::strings::Id::EditboxEmpty);
        const char* cstr =
            *reinterpret_cast<const char**>(
                static_cast<unsigned char*>(control) + kEditboxStringCStrOffset);
        uint32_t len = *reinterpret_cast<uint32_t*>(
            static_cast<unsigned char*>(control) + kEditboxStringLengthOffset);
        if (cstr && len > 0) {
            // Bound the copy so a corrupted length can't blow past outBuf.
            char text[128];
            uint32_t copyLen = len < sizeof(text) - 1 ? len : sizeof(text) - 1;
            memcpy(text, cstr, copyLen);
            text[copyLen] = '\0';
            snprintf(outBuf, bufSize, "%s. %s", role, text);
        } else {
            snprintf(outBuf, bufSize, "%s. %s", role, empty);
        }
        source = "editbox";
    }
    return source;
}

// 7. CSWGuiListBox content. STRICTLY single-row only. Many in-game
//    modals (CSWGuiMessageBox-style — the recurring 07434E40 OK/Cancel
//    in our log, and the quit-confirmation "Do you really want to
//    quit?") put their message text in a single listbox row
//    rather than directly in a panel label, so without this path the
//    modal appears as src=none.
//
//    For multi-row listboxes (skills, inventory, save list, journal
//    entries, etc.) we return nullptr — the engine's SetActiveControl
//    auto-focus on a multi-row listbox would read every row
//    concatenated into one blob ("Computer Use  Demolitions…"
//    on every Abilities open). Row-by-row navigation via the
//    ListBoxPanelSpec / chain handlers is the only correct way to
//    expose those contents; SetActiveControl on the container
//    deliberately speaks nothing now.
//
//    Recursion is bounded to one level — listbox rows are not
//    themselves listboxes in observed layouts, so we only try
//    button/label extraction on the single row, never re-enter the
//    listbox branch.
const char* TrySingleRowListBox(void* control, char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (IsListBox(control)) {
        auto* lb = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(control) + kListBoxControlsOffset);
        if (lb && lb->data && lb->size == 1) {
            int n = 1;
            outBuf[0] = '\0';
            size_t off = 0;
            for (int i = 0; i < n; ++i) {
                void* row = lb->data[i];
                if (!row) continue;
                // kAnnounceTextMax, not 256: the single row IS the whole
                // message on every modal that routes its text through a
                // listbox, and tutorial popups run well past 256 bytes
                // (K2's mine tutorial is ~850). At 256 they were cut
                // mid-word at exactly 255 characters — 19 times in one K1
                // session — and, worse, the truncated string no longer
                // matched the full-length TLK copy in tutorial_hints, so
                // long popups silently lost their keyboard hint as well.
                char rowText[kAnnounceTextMax];
                bool got = false;
                if (void* btn = CallDowncast(row, kVtableAsButton)) {
                    got = ExtractTextOrStrRef(btn, kButtonTextOffset,
                                              kButtonStrRefOffset,
                                              rowText, sizeof(rowText));
                }
                if (!got) {
                    if (void* lbl = CallDowncast(row, kVtableAsLabel)) {
                        got = ExtractTextOrStrRef(lbl, kLabelTextOffset,
                                                  kLabelStrRefOffset,
                                                  rowText, sizeof(rowText));
                    }
                }
                if (!got) {
                    if (void* hil = CallDowncast(row, kVtableAsLabelHilight)) {
                        got = ExtractTextOrStrRef(hil, kLabelTextOffset,
                                                  kLabelStrRefOffset,
                                                  rowText, sizeof(rowText));
                    }
                }
                if (!got) continue;
                size_t rowLen = strnlen(rowText, sizeof(rowText));
                if (rowLen == 0) continue;
                size_t needed = (off > 0 ? 2 : 0) + rowLen + 1;
                if (off + needed >= bufSize) break;
                if (off > 0) {
                    outBuf[off++] = ' ';
                    outBuf[off++] = ' ';
                }
                memcpy(outBuf + off, rowText, rowLen);
                off += rowLen;
                outBuf[off] = '\0';
            }
            if (off > 0) source = "listbox";
        }
    }
    return source;
}

// 9. Item-row name, read from the ITEM rather than the row control.
//
//    Inventory / container / store rows normally carry the localised name
//    in their own CExoString and TryButton (step 3) reads it there. When
//    that string comes up empty the row still knows which object it was
//    built for, so ask the engine for that object's name instead of
//    guessing from the panel behind it.
//
//    Placed immediately before the sibling-label fallback, and that
//    ordering is the point: a listbox row's extent is listbox-LOCAL
//    content space, so a spatial search across the panel's controls can
//    only ever match something unrelated. KOTOR 2 captioned an inventory
//    item with the screen title "Inventory" that way, on every visit
//    (userlogs/daninventoryequipmentbroken077).
const char* TryItemEntryRow(void* control, char* outBuf, size_t bufSize) {
    if (!acc::engine::ReadItemRowName(control, outBuf, bufSize)) {
        return nullptr;
    }
    if (outBuf[0] == '\0') return nullptr;
    acclog::Trace("Menus.ItemRow",
                  "control=%p named from item: \"%s\"", control, outBuf);
    return "itemrow-name";
}

// 10. Sibling-label fallback for chain-navigable controls with no text.
//    Image-only icon buttons (vtable=0x0073E658 in CSWGuiInGameMenu —
//    Equipment / Inventory / Character / Map / Abilities / Journal /
//    Options / Messages icons) genuinely have no inline text. Their
//    visible name lives on a separately-allocated CSWGuiLabelHilight
//    sibling at the same x-coord. FindSiblingLabel locates that sibling
//    spatially; we then announce its text as if it were our own. Same
//    pattern the slider extraction (step 6) uses for category labels.
//
//    Gated on IsChainNavigable(control) so we only fire for buttons
//    the user can actually focus — a label on its own (already
//    extractable elsewhere) wouldn't hit this path.
//
//    Cycle flanker arrows (chargen ± and Difficulty-style left/right)
//    are suppressed here so the cycle-mechanism's empty-text predicate
//    engages. Without the suppression the nearest plain label would
//    paint both flankers (e.g. "Strength" for the + and the -),
//    defeating both the chain squash and FindAdjacentArrow.
//
//    `owner` is FromControl's ResolveOwnerPanel result — the panel that
//    actually holds this control — NOT the g_currentPanel global.
//    Those two diverge whenever a panel is pushed without
//    firing CSWGuiPanel::SetActiveControl, which is the norm for KOTOR
//    2's chargen wizard steps: g_currentPanel stayed on custpnl_p (the
//    step list) while the user navigated abchrgen_p / skchrgen_p. Both
//    callees then answered about the WRONG panel — IsCycleFlankerArrow
//    found no value button beside a ± stepper so the suppression never
//    engaged, and FindSiblingLabel captioned the stepper with whichever
//    of custpnl_p's LBL_NUM1..6 step numbers happened to line up. That
//    is the stray "1" entry the KOTOR 2 chargen Attribute and
//    Abilities chains grew between their 4th, 5th and 6th rows
//    (patch-20260803-095222.log, chain indices 4 and 6 on both panels).
//
//    Gated on IsPanelDirectChild as well, and that gate is what keeps the
//    search meaningful at all: both callees compare GetControlCenter
//    values against the panel's own controls, and only a panel-direct
//    control has a screen-space extent to compare. A LISTBOX ROW's extent
//    is listbox-local content space that starts near the origin and
//    climbs one row-pitch at a time, so matching it against panel labels
//    is a coordinate-system error - it pairs whatever label happens to
//    sit near the top-left of the screen. That is exactly how a KOTOR 2
//    inventory row with an empty caption came out as the panel title
//    "Inventory"; step 9 above now names such a row from its item, and
//    this gate stops the spatial search from firing on rows at all.
const char* TrySiblingLabel(void* control, void* owner,
                            char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    // Liveness-filtered — both callees walk panel+kPanelControlsOffset, and a
    // module load leaves g_currentPanel pointing at a freed panel (opening
    // Equipment right after an area transition crashed on data[0] of the
    // reused block).
    // ResolveOwnerPanel already applied that filter to `owner`.
    void* panel = owner;
    if (panel && IsChainNavigable(control) &&
        IsPanelDirectChild(panel, control) &&
        !IsCycleFlankerArrow(panel, control)) {
        char label[256];
        if (FindSiblingLabel(panel, control,
                             label, sizeof(label))) {
            if (EmitText(outBuf, bufSize, label)) {
                source = "siblinglabel-fallback";
                // Trace: fired for every chain entry on every arrow press,
                // resolves to the same (control, label) tuple in tight bursts.
                acclog::Trace("Menus.SiblingFallback",
                              "control=%p label=\"%s\"", control, outBuf);
            }
        }
    }
    return source;
}

}  // namespace acc::menus::extract
