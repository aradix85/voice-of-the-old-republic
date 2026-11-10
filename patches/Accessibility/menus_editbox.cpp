// The editbox: arms when chain focus lands on one, speaks each typed or
// deleted character from a per-tick text diff, and handles the re-read,
// submit and cancel keys (menus_editbox.h has the model). Vanilla has two
// editbox panels (chargen Name and the save-name popup), so one armed
// editbox at a time.

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "menus_editbox.h"

#include "engine_input.h"
#include "engine_manager.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "hotkeys.h"
#include "input_pipeline.h"  // NoteEditboxSubmitClosed
#include "log.h"
#include "menus.h"           // ClearPendingAnnounce
#include "menus_extract.h"
#include "speech.h"
#include "strings.h"

using namespace acc::engine;

namespace acc::menus::editbox {

namespace {

// A panel that owns an editbox: its vtable, the embedded CSWGuiEditbox, and
// the label whose text is the panel's title. CSWGuiNameChargen's first label
// is the chargen header BioWare reuses on every chargen sub-panel, so its
// title is subtitle_label ("Name"); the save-name popup's is its own
// title_label. Both are read live, in the game's language.
struct EditboxPanelSpec {
    const char*      logTag;
    const uintptr_t* vtable;
    const size_t*    editboxOffset;
    const size_t*    titleLabelOffset;
};

const EditboxPanelSpec kSpecs[] = {
    {"ChargenName", &kVtableCSWGuiNameChargen, &kNameChargenEditboxOffset,
     &kNameChargenSubtitleLabelOffset},
    {"SaveName", &kVtableCSWGuiSaveNamePanel, &kSaveNameEditboxOffset,
     &kSaveNameTitleLabelOffset},
};

bool Matches(const EditboxPanelSpec& spec, void* panel) {
    void* vt = nullptr;
    return TryReadPtr(panel, 0, &vt) && reinterpret_cast<uintptr_t>(vt) == *spec.vtable;
}

void* At(void* panel, size_t offset) {
    return static_cast<unsigned char*>(panel) + offset;
}

const EditboxPanelSpec* SpecFor(void* panel) {
    for (const EditboxPanelSpec& spec : kSpecs) {
        if (Matches(spec, panel)) return &spec;
    }
    return nullptr;
}

struct ArmedState {
    const EditboxPanelSpec* spec;
    void* panel;
    void* editbox;
    bool  editMode;  // false after Esc: armed but passive
    char  text[64];  // a chargen name is about 16 bytes
    uint32_t textLen;
};

ArmedState s_state = {nullptr, nullptr, nullptr, false, {0}, 0};

// The editbox's text. Its length comes from strnlen: the engine does not
// keep the CExoString length in sync on Backspace, but it terminates the
// buffer on every change.
bool ReadEditboxText(void* editbox, char* out, size_t cap, uint32_t& outLen) {
    out[0] = '\0';
    outLen = 0;
    void* cstr = nullptr;
    if (!TryReadPtr(editbox, kEditboxStringCStrOffset, &cstr)) return false;
    if (!cstr) return true;
    __try {
        const size_t n = strnlen(static_cast<const char*>(cstr), cap - 1);
        memcpy(out, cstr, n);
        out[n] = '\0';
        outLen = static_cast<uint32_t>(n);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out[0] = '\0';
        outLen = 0;
        return false;
    }
    return true;
}

void Snapshot(ArmedState& s) {
    ReadEditboxText(s.editbox, s.text, sizeof(s.text), s.textLen);
}

void SpeakFullText(const char* text, uint32_t len) {
    acc::speech::Speak(len ? text : acc::strings::Get(acc::strings::Id::EditboxEmpty),
                       /*interrupt=*/false);
}

// One character as a one-char string, read per NVDA's character verbosity.
void SpeakSingleChar(char c) {
    const char buf[2] = {c, '\0'};
    acc::speech::Speak(buf, /*interrupt=*/false);
}

// CSWGuiEditbox has no caret (a struct-wide diff against 21 Left/Right
// presses showed nothing), so one character more is taken as appended and
// one fewer as Backspace from the end; Up/Down re-read the whole value.
void PollAndAnnounceDiff(ArmedState& s) {
    char newText[sizeof(s.text)];
    uint32_t newLen = 0;
    if (!ReadEditboxText(s.editbox, newText, sizeof(newText), newLen)) return;
    if (newLen == s.textLen && memcmp(newText, s.text, newLen) == 0) return;
    const int32_t delta = static_cast<int32_t>(newLen) - static_cast<int32_t>(s.textLen);
    if (delta == 1) {
        SpeakSingleChar(newText[newLen - 1]);
        acclog::Write("Editbox", "%s insert char='%c' len=%u->%u",
                      s.spec->logTag, newText[newLen - 1], s.textLen, newLen);
    } else if (delta == -1) {
        const char deleted = s.textLen > 0 ? s.text[s.textLen - 1] : '?';
        SpeakSingleChar(deleted);
        acclog::Write("Editbox", "%s delete char='%c' len=%u->%u",
                      s.spec->logTag, deleted, s.textLen, newLen);
    } else {
        // The Random button, or several characters at once.
        SpeakFullText(newText, newLen);
        acclog::Write("Editbox", "%s bulk text change len=%u->%u new=\"%s\"",
                      s.spec->logTag, s.textLen, newLen, newText);
    }
    memcpy(s.text, newText, newLen + 1);
    s.textLen = newLen;
}

// The engine's text input eats the keys at the editbox before the manager
// hook sees them (no Menus.Input line during typing), so the modal keys are
// polled. Left/Right stay unbound: with no engine caret, a virtual cursor
// would disagree with Backspace, which always deletes from the end.
void PollModalKeys(ArmedState& s) {
    namespace hk = acc::hotkeys;
    // Edges are taken every tick, so a key held on re-entry is not fresh.
    const bool upEdge    = hk::Pressed(hk::Action::EditboxReReadUp);
    const bool downEdge  = hk::Pressed(hk::Action::EditboxReReadDown);
    const bool enterEdge = hk::Pressed(hk::Action::EditboxSubmit);
    const bool escEdge   = hk::Pressed(hk::Action::EditboxCancel);
    if (!s.editMode || !s.spec) return;

    if (upEdge || downEdge) {
        Snapshot(s);
        SpeakFullText(s.text, s.textLen);
        acclog::Write("Editbox", "%s %s (Win32) -> re-speak text len=%u",
                      s.spec->logTag, upEdge ? "VK_UP" : "VK_DOWN", s.textLen);
    }
    if (enterEdge) {
        // The engine submits on Enter itself (HandleDoneButton). A second
        // activation of OK a frame later ran into the closing logic and
        // popped the panel underneath, stranding the routing.
        acclog::Write("Editbox", "%s VK_RETURN (Win32) -> edit-mode off "
                      "(engine handles submit natively)", s.spec->logTag);
        s.editMode = false;
        // The save-name popup classifies as Unknown, so the same Enter would
        // also reach the in-world interact and queue a dialogue that fired
        // when the menu closed; the latch swallows it.
        acc::input::NoteEditboxSubmitClosed();
    }
    if (escEdge) {
        // The engine usually cancels the panel on Esc too (HandleCancelButton),
        // and the monitor then disarms; otherwise the user goes on with the
        // chain keys.
        acclog::Write("Editbox", "%s VK_ESCAPE (Win32) -> exit edit mode", s.spec->logTag);
        s.editMode = false;
    }
}

struct PanelMatch {
    const EditboxPanelSpec* spec;
    void* panel;
};

PanelMatch FindMatchingPanel() {
    constexpr int kCap = 32;
    void* panels[kCap];
    const int count = ReadPanelArray(GetGuiManager(), panels, kCap);
    for (int i = 0; i < count; ++i) {
        if (const EditboxPanelSpec* spec = panels[i] ? SpecFor(panels[i]) : nullptr) {
            return {spec, panels[i]};
        }
    }
    return {nullptr, nullptr};
}

void DisarmIfArmed(const char* reason) {
    if (!s_state.spec) return;
    acclog::Write("Editbox", "%s disarm: %s", s_state.spec->logTag, reason);
    s_state = {nullptr, nullptr, nullptr, false, {0}, 0};
}

// Focus has landed on the editbox: arm, swallow the edges of keys still
// held (the Enter that opened the editbox fired a submit on the first poll),
// and speak "Editbox. <value>" here, clearing the pending slot so the drain
// does not speak the bare text too.
void Arm(const PanelMatch& m, void* editbox) {
    s_state.spec     = m.spec;
    s_state.panel    = m.panel;
    s_state.editbox  = editbox;
    s_state.editMode = true;
    Snapshot(s_state);
    namespace hk = acc::hotkeys;
    hk::Consume(hk::Action::EditboxReReadUp);
    hk::Consume(hk::Action::EditboxReReadDown);
    hk::Consume(hk::Action::EditboxSubmit);
    hk::Consume(hk::Action::EditboxCancel);
    char msg[160];
    snprintf(msg, sizeof(msg), "%s. %s", acc::strings::Get(acc::strings::Id::EditboxRole),
             s_state.textLen > 0 ? s_state.text
                                 : acc::strings::Get(acc::strings::Id::EditboxEmpty));
    acc::speech::Speak(msg, /*interrupt=*/false);
    acc::menus::ClearPendingAnnounce();
    acclog::Write("Editbox", "%s arm: panel=%p editbox=%p initialText=\"%s\" len=%u spoke=\"%s\"",
                  m.spec->logTag, m.panel, editbox, s_state.text, s_state.textLen, msg);
}

}  // namespace

bool TryHandleInput(int /*n*/, void* /*thisPtr*/, void* activePanel,
                    int param_1, int param_2, int& outRv) {
    if (!s_state.editMode || !activePanel || activePanel != s_state.panel) return false;
    if (param_2 == 0) return false;  // press edge only

    // Esc: a silent exit; the editbox keeps focus, and a second Esc reaches
    // the engine's cancel.
    if (param_1 == kInputEsc1 || param_1 == kInputEsc2) {
        acclog::Write("Editbox", "%s Esc -> exit edit mode (silent)", s_state.spec->logTag);
        s_state.editMode = false;
        outRv = 1;
        return true;
    }
    // Enter: the engine commits the name itself (see PollModalKeys). The
    // engine normally eats Enter before this hook while editing.
    if (param_1 == kInputEnter1 || param_1 == kInputEnter2) {
        acclog::Write("Editbox", "%s Enter -> edit-mode off (engine handles submit natively)",
                      s_state.spec->logTag);
        s_state.editMode = false;
        outRv = 1;
        return true;
    }
    // Up / Down / Home / End re-read the value; consumed so Home/End cannot
    // warp the chain off the editbox mid-typing.
    if (param_1 == kInputNavUp || param_1 == kInputNavDown ||
        param_1 == kInputHome || param_1 == kInputEnd) {
        Snapshot(s_state);
        SpeakFullText(s_state.text, s_state.textLen);
        acclog::Write("Editbox", "%s %s -> re-speak text len=%u", s_state.spec->logTag,
                      param_1 == kInputNavUp     ? "Up"
                      : param_1 == kInputNavDown ? "Down"
                      : param_1 == kInputHome    ? "Home" : "End",
                      s_state.textLen);
        outRv = 1;
        return true;
    }
    // Everything else is the engine's; the tick diff announces the result.
    return false;
}

void TickEditboxMonitors() {
    const PanelMatch m = FindMatchingPanel();
    if (!m.spec) {
        DisarmIfArmed("no matching panel in stack");
        return;
    }
    void* editbox = At(m.panel, *m.spec->editboxOffset);
    void* activeControl = nullptr;
    TryReadPtr(m.panel, kPanelActiveControlOffset, &activeControl);
    const bool focusedNow = activeControl == editbox;

    if (s_state.editbox != editbox) {
        if (s_state.spec) DisarmIfArmed("editbox pointer changed");
        if (focusedNow) Arm(m, editbox);
        return;
    }
    // Focus moved on (to OK, Random or Cancel): disarm; coming back re-arms.
    if (!focusedNow) {
        DisarmIfArmed("focus left editbox");
        return;
    }
    if (s_state.editMode) {
        PollAndAnnounceDiff(s_state);
        PollModalKeys(s_state);
    }
}

const char* GetTitleOverride(void* panel) {
    const EditboxPanelSpec* spec = panel ? SpecFor(panel) : nullptr;
    if (!spec) return nullptr;
    static char s_buf[128];
    return acc::menus::extract::FromControl(At(panel, *spec->titleLabelOffset), s_buf,
                                            sizeof(s_buf), panel)
               ? s_buf
               : nullptr;
}

}  // namespace acc::menus::editbox
