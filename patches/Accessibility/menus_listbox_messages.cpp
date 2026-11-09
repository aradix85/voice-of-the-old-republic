// The in-game messages screen: its views (left and right switch them),
// the list and its title.

#include "menus_listbox_internal.h"

#include "engine_game.h"     // IsKotor2 — K2 one-step upgrade install
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "log.h"
#include "menus_pending.h"
#include "speech.h"
#include "engine_offsets_select.h"

namespace acc::menus::listbox {

namespace {

// ============================================================================
// InGameMessages — combat-feedback log + dialog history. Two listboxes
// inside the same panel (messages_listbox @+0x64, dialog_listbox @+0x344);
// the user toggles which is shown via show_button @+0x76c.
//
// Phase 1C of the combat-system plan. Skeleton routes Up/Down to
// messages_listbox by default — switching to dialog_listbox requires a
// state bit we don't yet capture from the toggle button. The user can
// still navigate the active view via the engine's mouse-driven toggle;
// the spec-driven keyboard nav follows the messages_listbox until that
// state-tracking lands.
//
// KOTOR 2 has five listboxes behind four view buttons (Dialog / Feedback /
// Combat / Effects, Effects showing a good and a bad list side by side).
// There Left/Right walk five stops — the Effects view counts twice, once
// per list — pressing the engine's own view button when the stop needs a
// different view, and Up/Down read whichever list is on screen. The engine's
// visible bit says which view is up; which effects list is ours is the only
// state kept here. Ends clamp and repeat the current stop.
// ============================================================================
enum class MsgView : int { Dialog, Feedback, Combat, EffectsGood, EffectsBad };
constexpr int kMsgViewLast = static_cast<int>(MsgView::EffectsBad);

void*    s_msgBadSidePanel = nullptr;  // panel whose Effects stop is the bad list
// A view press is queued, so the visible bits lag it by a tick or two. A
// second Left/Right inside that window must step from where the user is,
// not from what the engine still shows.
void*    s_msgPendingPanel = nullptr;
MsgView  s_msgPendingView  = MsgView::Feedback;
DWORD    s_msgPendingAt    = 0;
constexpr DWORD kMsgPendingMs = 500;

void* MsgViewListBox(void* panel, MsgView v) {
    switch (v) {
    case MsgView::Dialog:      return acc::off::Ptr(panel, kInGameMessagesDialogListBoxOffset);
    case MsgView::Feedback:    return acc::off::Ptr(panel, kInGameMessagesMessagesListBoxOffset);
    case MsgView::Combat:      return acc::off::Ptr(panel, kInGameMessagesCombatListBoxOffset);
    case MsgView::EffectsGood: return acc::off::Ptr(panel, kInGameMessagesEffectsGoodListBoxOffset);
    case MsgView::EffectsBad:  return acc::off::Ptr(panel, kInGameMessagesEffectsBadListBoxOffset);
    }
    return nullptr;
}

// The engine view button that shows `v`. Both effects stops share one.
void* MsgViewButton(void* panel, MsgView v) {
    switch (v) {
    case MsgView::Dialog:      return acc::off::Ptr(panel, kInGameMessagesDialogButtonOffset);
    case MsgView::Feedback:    return acc::off::Ptr(panel, kInGameMessagesFeedbackButtonOffset);
    case MsgView::Combat:      return acc::off::Ptr(panel, kInGameMessagesCombatButtonOffset);
    case MsgView::EffectsGood:
    case MsgView::EffectsBad:  return acc::off::Ptr(panel, kInGameMessagesEffectsButtonOffset);
    }
    return nullptr;
}

// The name the screen itself shows for the stop: the view button's caption,
// or for the two effects lists their headings ("Gute Effekte" / ...).
bool ReadMsgViewName(void* panel, MsgView v, char* out, size_t outN) {
    if (v == MsgView::EffectsGood) {
        return ReadLabelTextAt(panel, kInGameMessagesEffectsGoodLabelOffset, out, outN);
    }
    if (v == MsgView::EffectsBad) {
        return ReadLabelTextAt(panel, kInGameMessagesEffectsBadLabelOffset, out, outN);
    }
    return ReadButtonText(MsgViewButton(panel, v), out, outN);
}

MsgView CurrentMsgView(void* panel) {
    if (panel == s_msgPendingPanel &&
        GetTickCount() - s_msgPendingAt < kMsgPendingMs) {
        return s_msgPendingView;
    }
    if (IsControlVisible(MsgViewListBox(panel, MsgView::Dialog))) return MsgView::Dialog;
    if (IsControlVisible(MsgViewListBox(panel, MsgView::Combat))) return MsgView::Combat;
    if (IsControlVisible(MsgViewListBox(panel, MsgView::EffectsGood)) ||
        IsControlVisible(MsgViewListBox(panel, MsgView::EffectsBad))) {
        return panel == s_msgBadSidePanel ? MsgView::EffectsBad
                                          : MsgView::EffectsGood;
    }
    // Feedback visible — or nothing yet (mount frame): Feedback is the list
    // the panel has always defaulted to.
    return MsgView::Feedback;
}

// "<view>, N Einträge" / "<view>, leer".
void FormatMsgView(void* panel, MsgView v, char* out, size_t outN) {
    char name[128];
    if (!ReadMsgViewName(panel, v, name, sizeof(name))) name[0] = '\0';
    int rows = ReadListBoxRowCount(MsgViewListBox(panel, v));
    if (rows > 0) {
        snprintf(out, outN, acc::strings::Get(acc::strings::Id::FmtMessagesView),
                 name, rows);
    } else {
        snprintf(out, outN,
                 acc::strings::Get(acc::strings::Id::FmtMessagesViewEmpty), name);
    }
}

bool InGameMessagesOnLeftRight(void* panel, int dir) {
    if (!acc::game::IsKotor2()) return false;
    MsgView cur = CurrentMsgView(panel);
    int idx = static_cast<int>(cur) + dir;
    if (idx < 0) idx = 0;
    if (idx > kMsgViewLast) idx = kMsgViewLast;
    MsgView target = static_cast<MsgView>(idx);

    const char* action = "clamp";
    if (target != cur) {
        void* curButton    = MsgViewButton(panel, cur);
        void* targetButton = MsgViewButton(panel, target);
        if (targetButton != curButton) {
            action = acc::menus::pending::QueueActivate(targetButton)
                         ? "view-press" : "view-press-refused";
        } else {
            action = "effects-side";
        }
        s_msgPendingPanel = panel;
        s_msgPendingView  = target;
        s_msgPendingAt    = GetTickCount();
        s_msgBadSidePanel = (target == MsgView::EffectsBad) ? panel : nullptr;
    }

    char msg[200];
    FormatMsgView(panel, target, msg, sizeof(msg));
    acc::speech::Speak(msg, /*interrupt=*/true);
    acclog::Write("InGameMessages", "%s view %d -> %d (%s) \"%s\"",
                  dir < 0 ? "Left" : "Right", static_cast<int>(cur),
                  idx, action, msg);
    return true;
}

bool InGameMessagesMatches(void* p) {
    return IdentifyPanel(p) == PanelKind::InGameMessages;
}

void* InGameMessagesFindLb(void* p) {
    // acc::off::Ptr, not raw addition: an unported offset must yield nullptr
    // (the spec then declines and logs "empty; nav ignored") rather than a
    // non-null wild pointer that sails through every null check downstream.
    if (acc::game::IsKotor2()) return MsgViewListBox(p, CurrentMsgView(p));
    return acc::off::Ptr(p, kInGameMessagesMessagesListBoxOffset);
}

// NB: no `rowCount <= 0` guard here, unlike the other two users of
// SpeakRowAndPosition. That divergence predates this consolidation and is
// left exactly as it was rather than quietly unified — see the note on
// ExamineAnnounce.
void InGameMessagesAnnounce(void* /*lb*/, const ListBoxNavResult& r) {
    SpeakRowAndPosition(r);
}

const char* InGameMessagesTitleOverride(void* panel) {
    if (acc::game::IsKotor2()) {
        // The view the screen opened on, with its size — tells the user
        // where they are before the first Left/Right.
        static char buf[200];
        FormatMsgView(panel, CurrentMsgView(panel), buf, sizeof(buf));
        return buf;
    }
    return acc::strings::Get(acc::strings::Id::MessagesTitleCombatLog);
}

}  // namespace

constexpr ListBoxPanelSpec kInGameMessagesSpec = {
    /*logTag*/                  "InGameMessages",
    /*matches*/                 InGameMessagesMatches,
    /*armed*/                   nullptr,
    /*resetStale*/              nullptr,
    /*findListBox*/             InGameMessagesFindLb,
    /*minSel*/                  0,
    /*announce*/                InGameMessagesAnnounce,
    /*enrichRow*/               nullptr,
    /*logExtra*/                nullptr,
    /*onEnter*/                 nullptr,
    /*onEsc*/                   nullptr,
    /*titleOverride*/           InGameMessagesTitleOverride,
    /*emptyStateId*/            acc::strings::Id::Count_,
    /*alwaysReturnFromHandler*/ false,
    /*minSelFn*/                nullptr,
    /*useEngineSelect*/         false,
    /*navLive*/                 nullptr,
    /*onLeftRight*/             InGameMessagesOnLeftRight,  // K2 view switch; declines on K1
};

}  // namespace acc::menus::listbox
