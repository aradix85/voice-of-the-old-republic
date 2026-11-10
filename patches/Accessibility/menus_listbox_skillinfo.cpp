// The skill and feat information box: its list, the feat behind a row, the
// row text with details, and its title.

#include "menus_listbox_internal.h"

#include "engine_manager.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_levelup.h"  // IsOpeningLevelUp — SkillInfoBox title override
#include "engine_reads.h"
#include "log.h"
#include "menus_extract.h"
#include "speech.h"
#include "engine_offsets_select.h"

namespace acc::menus::listbox {

namespace {

// ============================================================================
// SkillInfoBox — engine slot for "info popups" with a row list and OK.
//
// skillinfo.gui mounted on the engine's SkillInfoBox slot. Three live
// controls: title label (id=0), LB_SKILLS listbox (id=2), BTN_OK (id=4).
// The .gui's title is hard-baked to the BioWare placeholder "Items
// Available to Place in Container and blah blah blah" — the chargen flow
// doesn't override it at runtime, so we substitute via titleOverride.
//
// In the chargen Feats flow this surfaces as the "ShowGranted" popup —
// CSWGuiFeatsCharGen mounts skillinfo.gui to dump the class's auto-granted
// feats (different per class — Soldier, Scoundrel, Scout) before letting the
// user proceed to actual feat selection on the underlying main panel.
// Each row carries only icon + name strref; the feat ID itself isn't
// stored on the row, so we recover it by reverse-lookup against the
// rules' CSWFeat[] array (matching name strrefs).
//
// The underlying CSWGuiFeatsCharGen panel sits below this overlay on the
// modal stack — calling its OnEnterFeat(featId) repopulates its
// description_listbox.controls[0] with the wrapped feat description,
// which we then read for the per-row enrichment speech.
// ============================================================================

bool SkillInfoBoxMatches(void* p) {
    return IdentifyPanel(p) == PanelKind::SkillInfoBox;
}

void* SkillInfoBoxFindLb(void* p) {
    return SkillInfoBoxPanelSkillsListBox(p);
}

// Walk the manager's panels[] for a CSWGuiFeatsCharGen instance. Returns
// nullptr when none is mounted (e.g. SkillInfoBox shown from a different
// chargen substep). Cheap — panels.size is ≤16 in practice and we only
// fire from picker arrow steps.
void* FindFeatsCharGenPanel() {
    constexpr int kCap = 32;
    void* panelsData[kCap];
    int n = acc::engine::ReadPanelArray(
        acc::engine::GetGuiManager(), panelsData, kCap);
    for (int i = 0; i < n; ++i) {
        void* panel = panelsData[i];
        if (!panel) continue;
        void** vt = nullptr;
        __try {
            vt = *reinterpret_cast<void***>(panel);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            continue;
        }
        if (reinterpret_cast<uintptr_t>(vt) == kVtableCSWGuiFeatsCharGen) {
            return panel;
        }
    }
    return nullptr;
}

// Reverse-lookup: given the strref the engine wrote onto a SkillEntry row
// (kLabelStrRefOffset within the SkillEntry — its label_hilight.label.text
// .text_params.str_ref), find the matching feat in Rules->feats[] and
// return the feat ID. Returns -1 on miss (row strref unset, lookup fault,
// or no feat with matching name strref — happens in non-feat contexts
// where SkillInfoBox would be reused for skills/force-powers).
int ResolveFeatIdFromRowStrref(void* row, int& outRowStrref) {
    outRowStrref = 0;
    if (!row) return -1;
    __try {
        outRowStrref = *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(row) + kLabelStrRefOffset);
        if (outRowStrref == -1 || outRowStrref == 0) return -1;
        void* rules = *reinterpret_cast<void**>(kAddrRulesGlobal);
        if (!rules) return -1;
        auto* rulesBase = reinterpret_cast<unsigned char*>(rules);
        auto* feats = *reinterpret_cast<unsigned char**>(
            rulesBase + kRulesFeatsArrayOffset);
        int featCount = *reinterpret_cast<unsigned short*>(
            rulesBase + kRulesFeatCountOffset);
        if (!feats || featCount <= 0 || featCount > 0x4000) return -1;
        for (int i = 0; i < featCount; ++i) {
            int s = *reinterpret_cast<int*>(
                feats + i * kFeatStructSize + kFeatNameStrRefOffset);
            if (s == outRowStrref) return i;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
    return -1;
}

// Standard per-row speech: just "feat name, N of M". The
// fetch-and-speak-description side-channel runs in the enrichRow callback
// below.
void SkillInfoBoxAnnounce(void* /*lb*/, const ListBoxNavResult& r) {
    if (!r.row) return;
    char rowText[128];
    rowText[0] = '\0';
    if (!acc::menus::extract::FromControl(r.row, rowText, sizeof(rowText))) {
        return;
    }
    char head[200];
    snprintf(head, sizeof(head),
             acc::strings::Get(acc::strings::Id::FmtChargenFeatGrantedRow),
             rowText, r.newSel + 1, r.rowCount);
    acc::speech::Speak(head, /*interrupt=*/false);
}

// Per-row enrichment: refresh the underlying CSWGuiFeatsCharGen's
// description_listbox via OnEnterFeat(featId), then read + speak that
// description. Quietly no-ops in non-feat contexts (no main panel
// mounted, or row strref doesn't resolve to a known feat).
void SkillInfoBoxEnrichRow(void* /*panel*/, const ListBoxNavResult& r) {
    if (!r.row) return;

    void* fcp = FindFeatsCharGenPanel();
    if (!fcp) {
        // SkillInfoBox shown from a non-feat context — silent skip.
        return;
    }

    int rowStrref = 0;
    int featIdx = ResolveFeatIdFromRowStrref(r.row, rowStrref);
    if (featIdx < 0) {
        acclog::Write("SkillInfoBox",
                      "feat-id lookup miss fcp=%p sel=%d strref=%d",
                      fcp, r.newSel, rowStrref);
        return;
    }
    unsigned short featId = static_cast<unsigned short>(featIdx);

    // Synchronous engine call — fires DetermineFeat → SetDescription →
    // ClearItems + AddControls on description_listbox. After this returns,
    // description_listbox.controls[0] holds the wrapped text for featId.
    typedef void (__thiscall* PFN_OnEnterFeat)(void* this_,
                                               unsigned short featId);
    __try {
        auto fn = reinterpret_cast<PFN_OnEnterFeat>(
            kAddrCSWGuiFeatsCharGenOnEnterFeat);
        fn(fcp, featId);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("SkillInfoBox",
                      "OnEnterFeat faulted fcp=%p featId=%u",
                      fcp, (unsigned)featId);
        return;
    }

    // Ptr, not raw addition: kFeatsCharGenDescriptionListBoxOffset is still
    // Todo on KOTOR 2, and the feats panel can classify there via RTTI — a
    // raw base+poison would hand a wild non-null pointer to the reads below
    // (the Batch 3 poison-pointer lesson).
    auto* fcpBase = reinterpret_cast<unsigned char*>(fcp);
    void* descLb  = acc::off::Ptr(fcpBase, kFeatsCharGenDescriptionListBoxOffset);
    auto* descList = descLb ? reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(descLb) + kListBoxControlsOffset)
                            : nullptr;
    void* descRow = nullptr;
    __try {
        if (descList && descList->data && descList->size > 0) {
            descRow = descList->data[0];
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        descRow = nullptr;
    }

    char desc[1024];
    if (descRow && ReadLabelText(descRow, desc, sizeof(desc)) &&
        desc[0] != '\0') {
        acc::speech::Speak(desc, /*interrupt=*/false);
        // Newline-flatten for the diagnostic log line.
        char dump[1024];
        size_t dn = strnlen(desc, sizeof(desc) - 1);
        if (dn >= sizeof(dump)) dn = sizeof(dump) - 1;
        for (size_t i = 0; i < dn; ++i) {
            char c = desc[i];
            dump[i] = (c == '\n' || c == '\r') ? ' ' : c;
        }
        dump[dn] = '\0';
        acclog::Write("SkillInfoBox",
                      "desc fcp=%p featId=%u (first 300 chars: \"%.300s\")",
                      fcp, (unsigned)featId, dump);
    } else {
        acclog::Write("SkillInfoBox",
                      "desc fcp=%p featId=%u empty or unreadable",
                      fcp, (unsigned)featId);
    }
}

bool SkillInfoBoxOnEnter(void* panel) {
    QueueControlActivate(SkillInfoBoxPanelOkButton(panel),
                         "SkillInfoBox: Enter -> BTN_OK");
    return true;
}

// Read the panel's own title label, and say whether it still holds the BioWare
// dev-leftover baked into skillinfo.gui ("Items Available to Place in Container
// and blah blah blah"). Not localised — the string lives in the .gui, not the
// TLK, and came back verbatim on a German install.
//
// This is what separates the level-up popup's two lives. While the wizard is up
// the label is still the placeholder and the popup is a silent passenger. Once
// Accept commits, the engine writes the real "you gained these feats
// this level" text onto the same label and the popup becomes the
// last thing standing between the user and the world.
bool SkillInfoBoxTitleIsPlaceholder(void* panel, char* outText, size_t outSize) {
    if (outSize) outText[0] = '\0';
    void* title = SkillInfoBoxPanelMessageLabel(panel);
    if (!title ||
        !acc::menus::extract::FromControl(title, outText, outSize, panel) ||
        outText[0] == '\0') {
        return true;   // unreadable — the placeholder is the safe assumption
    }
    return strstr(outText, "blah blah blah") != nullptr ||
           strncmp(outText, "Items Available", 15) == 0;
}

// Title override: substitute our "You receive these feats"
// string for the BioWare placeholder text baked into skillinfo.gui.
// Only applies in the chargen Feats flow — gated by FindFeatsCharGenPanel
// so future Force-Powers / Skills reuse can layer on different titles.
const char* SkillInfoBoxTitleOverride(void* panel) {
    if (FindFeatsCharGenPanel()) {
        return acc::strings::Get(acc::strings::Id::ChargenFeatGrantedTitle);
    }
    // In-world level-up (Shift+L). Gated on an active in-world level-up wizard
    // so neither branch fires for a SkillInfoBox shown from any other host.
    if (acc::engine::HasActiveLevelUpPanel() ||
        acc::engine_levelup::IsOpeningLevelUp()) {
        static char s_composed[512];
        char title[320];
        if (SkillInfoBoxTitleIsPlaceholder(panel, title, sizeof(title))) {
            // Wizard opening: the popup rides along on the category screen
            // carrying the placeholder. Speak a short level-up hint instead —
            // the "what do I do here" a blind player can't see — which also
            // silences that gibberish.
            return acc::strings::Get(acc::strings::Id::LevelUpScreenHint);
        }
        // Wizard committed and destroyed: this popup is now the top modal and
        // the user has to press Enter on its OK button to get back to the
        // world. Speak what it actually says plus the way out — repeating the
        // category hint here was both wrong and a verbatim repeat of the
        // opening narration.
        snprintf(s_composed, sizeof(s_composed), "%s %s", title,
                 acc::strings::Get(acc::strings::Id::LevelUpGrantedPopupHint));
        return s_composed;
    }
    return nullptr;  // unknown SkillInfoBox host — let the generic walk run
}

}  // namespace

constexpr ListBoxPanelSpec kSkillInfoBoxSpec = {
    /*logTag*/                  "SkillInfoBox",
    /*matches*/                 SkillInfoBoxMatches,
    /*armed*/                   nullptr,
    /*resetStale*/              nullptr,
    /*findListBox*/             SkillInfoBoxFindLb,
    /*minSel*/                  0,
    /*announce*/                SkillInfoBoxAnnounce,
    /*enrichRow*/               SkillInfoBoxEnrichRow,
    /*logExtra*/                nullptr,
    /*onEnter*/                 SkillInfoBoxOnEnter,
    /*onEsc*/                   nullptr,            // no Cancel button on this overlay
    /*titleOverride*/           SkillInfoBoxTitleOverride,
    /*emptyStateId*/            acc::strings::Id::Count_,
    /*alwaysReturnFromHandler*/ true,               // modal popup — don't fall through
};

}  // namespace acc::menus::listbox
