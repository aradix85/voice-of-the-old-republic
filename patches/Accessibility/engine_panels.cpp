// Panel identity: the CGuiInGame slot table, IdentifyPanel and the probe
// that logs a panel nothing recognises.

#include "engine_panels_internal.h"

#include "engine_manager.h"
#include "engine_offsets.h"
#include "engine_reads.h"
#include "log.h"

namespace acc::engine {

namespace {

// Rows with kNoSlotOffset are heap-allocated panels with no CGuiInGame field:
// the slot lookup skips them, PanelKindName still names them, and
// IdentifyByVtable recognises them.
constexpr size_t kNoSlotOffset = static_cast<size_t>(-1);

struct PanelKindOffset {
    size_t      offset;
    PanelKind   kind;
    const char* name;
};

// One row per named CGuiInGame slot. KOTOR 2's column comes from the
// `mov [this+off], eax` that files away each panel CGuiInGame's creator
// constructs (tools/re-scripts/k2_slot_table.py, docs/kotor2-port.md). The
// layouts agree up to +0x74, then KOTOR 2 inserts members and the slots
// above shift; InGameMessages and PartySelection trade 0x1c and 0x78, so an
// unported table would map those two screens onto each other in silence. A
// Todo() row poisons to kUnportedOffset on KOTOR 2 and the lookup skips it.
const PanelKindOffset kPanelKindOffsets[] = {
    // ShowSWInGameGui / HideSWInGameGui add and remove [this+0x08] as the
    // strip menu in both games.
    { acc::off::Same(0x08), PanelKind::InGameMenu,          "InGameMenu" },
    { acc::off::Same(0x0c), PanelKind::InGameEquip,         "InGameEquip" },
    { acc::off::Same(0x10), PanelKind::InGameInventory,     "InGameInventory" },
    // KOTOR 2's creator (0x007BE4C0) constructs the character sheet at
    // 0x0084C3A0 (it stores the CSWGuiInGameCharacter vtable 0x009A3E7C) and
    // writes it with `MOV [gui+0x14],EAX` at 0x007BF439.
    { acc::off::Same(0x14), PanelKind::InGameCharacter,     "InGameCharacter" },
    { acc::off::Same(0x18), PanelKind::InGameAbilities,     "InGameAbilities" },
    { acc::off::Pick(0x1c, 0x78), PanelKind::InGameMessages, "InGameMessages" },
    { acc::off::Same(0x20), PanelKind::InGameJournal,       "InGameJournal" },
    { acc::off::Same(0x24), PanelKind::InGameMap,           "InGameMap" },
    { acc::off::Same(0x28), PanelKind::InGameOptions,       "InGameOptions" },
    // 0x3c is the ACTIVE-dialog-panel pointer (aliases 0x40 or 0x44), not a
    // creator-built slot; KOTOR 2's helper 0x007CB750 checks
    // `[gui+0x3c] && [gui+0x3c]==[gui+0x40]`.
    { acc::off::Same(0x3c), PanelKind::DialogCinematicCopy, "DialogCinematicCopy" },
    { acc::off::Same(0x40), PanelKind::DialogCinematic,     "DialogCinematic" },
    { acc::off::Same(0x44), PanelKind::DialogComputer,      "DialogComputer" },
    // KOTOR 2's creator stores the CSWGuiDialogComputerCamera ctor result
    // (0x008BD910) via `MOV [gui+0x48],ECX` at 0x007BE9D4.
    { acc::off::Same(0x48), PanelKind::DialogComputerCamera, "DialogComputerCamera" },
    { acc::off::Same(0x4c), PanelKind::BarkBubble,          "BarkBubble" },
    { acc::off::Same(0x50), PanelKind::Examine,             "Examine" },
    { acc::off::Same(0x54), PanelKind::Container,           "Container" },
    { acc::off::Same(0x58), PanelKind::CreateItemMenu,      "CreateItemMenu" },
    { acc::off::Same(0x5c), PanelKind::CreateItemSubMenu,   "CreateItemSubMenu" },
    { acc::off::Same(0x60), PanelKind::DialogLetterbox1,    "DialogLetterbox1" },
    { acc::off::Same(0x64), PanelKind::DialogLetterbox2,    "DialogLetterbox2" },
    { acc::off::Same(0x68), PanelKind::DialogLetterbox3,    "DialogLetterbox3" },
    { acc::off::Same(0x6c), PanelKind::Fade,                "Fade" },
    { acc::off::Same(0x70), PanelKind::LoadModuleDebugMenu, "LoadModuleDebugMenu" },
    { acc::off::Same(0x74), PanelKind::PowersFeatsSkillsDebugMenu,
                                                            "PowersFeatsSkillsDebugMenu" },
    // KOTOR 2's creator stores the CSWGuiPartySelection ctor result
    // (0x0089CF30) at [gui+0x1c].
    { acc::off::Pick(0x78, 0x1c), PanelKind::PartySelection, "PartySelection" },
    { acc::off::Same(0x7c), PanelKind::InGamePause,         "InGamePause" },
    // KOTOR 2's creator stores the CSWGuiInGameGalaxyMap ctor result
    // (0x008973D0) at [gui+0x80].
    { acc::off::Same(0x80), PanelKind::InGameGalaxyMap,     "InGameGalaxyMap" },
    { acc::off::Same(0x84), PanelKind::Store,               "Store" },
    { acc::off::Pick(0x8c, 0x94), PanelKind::SoloModeQuery, "SoloModeQuery" },
    // KOTOR 2's SetSWGuiStatus @0x007C9C40 adds the [this+0x98] panel on
    // status 1 and removes it otherwise, as KOTOR 1's does for +0x90.
    { acc::off::Pick(0x90, 0x98), PanelKind::MainInterface, "MainInterface" },
    { acc::off::Pick(0x94, 0x9c), PanelKind::AreaTransition, "AreaTransition" },
    // KOTOR 2 files three CSWGuiMessageBox instances (0xa0, 0xa4 and one not
    // yet followed); KOTOR 1 tracks only the modal one. 0xa0 is the first.
    { acc::off::Pick(0x98, 0xa0), PanelKind::MessageBoxModal, "MessageBox" },
    { acc::off::Pick(0x9c, 0xac), PanelKind::SkillInfoBox,  "SkillInfoBox" },
    { acc::off::Pick(0xa0, 0xb0), PanelKind::TutorialBox,   "TutorialBox" },
    // KOTOR 2's creator builds the ControllerLossBox inline and stores it at
    // [gui+0xb4]; its 0xa4 is the second MessageBox (ctor 0x0075B370, arg 1).
    { acc::off::Pick(0xa4, 0xb4), PanelKind::ControllerLossBox, "ControllerLossBox" },
    { acc::off::Pick(0xa8, 0xb8), PanelKind::StatusSummary, "StatusSummary" },
    // Dialogue input-routing surfaces (CGuiInGame layout, swkotor.exe.h:10282):
    // during a cinematic conversation the arrow keys go to a foreground panel
    // other than the rendering one at +0x3c. KOTOR 1 only: KOTOR 2's creator
    // stores no slot past +0xb8, and its +0xf8..0x11f holds other fields.
    { acc::off::Kotor1Only(0xf8), PanelKind::DialogMessagesAux, "DialogMessagesAux" },
    { acc::off::Kotor1Only(0xfc), PanelKind::DialogMessages,    "DialogMessages" },
    // Heap-allocated kinds with no CGuiInGame slot.
    { kNoSlotOffset, PanelKind::SaveLoad,          "SaveLoad" },
    { kNoSlotOffset, PanelKind::InGameLevelUp,     "InGameLevelUp" },
    { kNoSlotOffset, PanelKind::CharGen,           "CharGen" },
    { kNoSlotOffset, PanelKind::WorkbenchSelect,   "WorkbenchSelect" },
    { kNoSlotOffset, PanelKind::WorkbenchItems,    "WorkbenchItems" },
    { kNoSlotOffset, PanelKind::WorkbenchUpgrade,  "WorkbenchUpgrade" },
    { kNoSlotOffset, PanelKind::WorkbenchCreateItem,    "WorkbenchCreateItem" },
    { kNoSlotOffset, PanelKind::WorkbenchCreateMedical, "WorkbenchCreateMedical" },
    { kNoSlotOffset, PanelKind::PowersLevelUp,     "PowersLevelUp" },
    { kNoSlotOffset, PanelKind::MainMenuOptions,   "MainMenuOptions" },
    { kNoSlotOffset, PanelKind::MainMenu,          "MainMenu" },
    { kNoSlotOffset, PanelKind::PazaakStart,       "PazaakStart" },
    { kNoSlotOffset, PanelKind::PazaakWager,       "PazaakWager" },
    { kNoSlotOffset, PanelKind::InGameQuestItems,  "InGameQuestItems" },
    { kNoSlotOffset, PanelKind::ScriptSelect,      "ScriptSelect" },
    { kNoSlotOffset, PanelKind::SoundSettings,            "SoundSettings" },
    { kNoSlotOffset, PanelKind::AdvancedSoundSettings,    "AdvancedSoundSettings" },
    { kNoSlotOffset, PanelKind::GraphicsSettings,         "GraphicsSettings" },
    { kNoSlotOffset, PanelKind::AdvancedGraphicsSettings, "AdvancedGraphicsSettings" },
    { kNoSlotOffset, PanelKind::AutoPauseOptions,         "AutoPauseOptions" },
    { kNoSlotOffset, PanelKind::FeedbackOptions,          "FeedbackOptions" },
    { kNoSlotOffset, PanelKind::GameSettings,             "GameSettings" },
    { kNoSlotOffset, PanelKind::MouseSettings,            "MouseSettings" },
    { kNoSlotOffset, PanelKind::KeyboardMapping,          "KeyboardMapping" },
};
constexpr int kPanelKindOffsetCount =
    sizeof(kPanelKindOffsets) / sizeof(kPanelKindOffsets[0]);

// (panel, kind) pairs already logged, so persistent panels (the HUD) checked
// on every input event log once. FIFO-evicted at the cap.
struct PanelKindCacheEntry {
    void*     panel;
    PanelKind kind;
};
constexpr int kPanelKindCacheSize = 32;
PanelKindCacheEntry g_panelKindCache[kPanelKindCacheSize];
int g_panelKindCacheCount = 0;

void LogIdentifiedOnce(void* panel, PanelKind kind) {
    for (int j = 0; j < g_panelKindCacheCount; ++j) {
        if (g_panelKindCache[j].panel == panel && g_panelKindCache[j].kind == kind) {
            return;
        }
    }
    if (g_panelKindCacheCount >= kPanelKindCacheSize) {
        memmove(g_panelKindCache, g_panelKindCache + 1,
                sizeof(g_panelKindCache[0]) * (kPanelKindCacheSize - 1));
        g_panelKindCacheCount = kPanelKindCacheSize - 1;
    }
    g_panelKindCache[g_panelKindCacheCount++] = { panel, kind };
    acclog::Write("PanelKind", "panel=%p identified as %s", panel,
                  PanelKindName(kind));
}

// Unknown-panel probe: a first-sight dump of a panel nothing recognises, once
// per panel class (deduped by vtable): its vtable, control count, and per
// control the vtable, .gui id and rendered text. Enough to add the class to
// the vtable table from one log.
constexpr int kUnknownVtableCacheSize = 16;
uintptr_t g_unknownVtableCache[kUnknownVtableCacheSize] = {};
int       g_unknownVtableCacheCount = 0;

bool IsVtableAlreadyDumped(uintptr_t vt) {
    for (int i = 0; i < g_unknownVtableCacheCount; ++i) {
        if (g_unknownVtableCache[i] == vt) return true;
    }
    return false;
}

void RememberDumpedVtable(uintptr_t vt) {
    if (g_unknownVtableCacheCount >= kUnknownVtableCacheSize) {
        memmove(g_unknownVtableCache, g_unknownVtableCache + 1,
                sizeof(g_unknownVtableCache[0]) *
                    (kUnknownVtableCacheSize - 1));
        g_unknownVtableCacheCount = kUnknownVtableCacheSize - 1;
    }
    g_unknownVtableCache[g_unknownVtableCacheCount++] = vt;
}

void LogUnknownPanelDiagnostics(void* panel) {
    if (!panel) return;
    uintptr_t panelVt = 0;
    __try {
        panelVt = reinterpret_cast<uintptr_t>(*reinterpret_cast<void**>(panel));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return;
    }
    if (panelVt == 0) return;
    if (IsVtableAlreadyDumped(panelVt)) return;
    RememberDumpedVtable(panelVt);

    CExoArrayList* list = nullptr;
    int size = 0;
    __try {
        list = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
        size = list ? list->size : 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        size = 0;
    }
    acclog::Write("PanelProbe",
                  "first sight UNKNOWN panel=%p vtable=0x%08x controls=%d",
                  panel, static_cast<unsigned>(panelVt), size);
    if (!list || !list->data || size <= 0) return;

    int n = size > 32 ? 32 : size;
    for (int i = 0; i < n; ++i) {
        void* c = nullptr;
        __try {
            c = list->data[i];
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            c = nullptr;
        }
        if (!c) {
            acclog::Write("PanelProbe", "  [%d] (null)", i);
            continue;
        }
        const uintptr_t cvt = ReadVtable(c);
        const int       cid = ReadControlId(c);
        char text[96];
        text[0] = '\0';
        if (!ReadGuiString(c, kButtonGuiStringPtrOffset,
                           text, sizeof(text))) {
            ReadGuiString(c, kLabelGuiStringPtrOffset,
                          text, sizeof(text));
        }
        acclog::Write("PanelProbe",
                      "  [%d] %p vtable=0x%08x id=%d text=\"%s\"",
                      i, c, static_cast<unsigned>(cvt), cid, text);
    }
}

// The CGuiInGame slot walk: the matching table index, or -1. In its own
// function so the __try owns no unwinding objects (C2712). The SEH is not
// optional: the walk reads engine memory through a multi-hop pointer chain
// that a mid-teardown frame can invalidate, and an unported chain once took
// the process down at this very compare.
int SlotTableLookup(void* panel) {
    void* gui = ResolveGuiInGame();
    if (!gui) return -1;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(gui);
        for (int i = 0; i < kPanelKindOffsetCount; ++i) {
            const size_t off = kPanelKindOffsets[i].offset;
            if (off == kNoSlotOffset || off == acc::off::kUnportedOffset) {
                continue;
            }
            void* slot = *reinterpret_cast<void**>(base + off);
            if (slot == panel) return i;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return -1;
}

}  // namespace

const char* PanelKindName(PanelKind k) {
    if (k == PanelKind::Unknown) return "Unknown";
    for (int i = 0; i < kPanelKindOffsetCount; ++i) {
        if (kPanelKindOffsets[i].kind == k) return kPanelKindOffsets[i].name;
    }
    return "?";
}

PanelKind IdentifyPanel(void* panel) {
    if (!panel) return PanelKind::Unknown;

    int slotIdx = SlotTableLookup(panel);
    PanelKind kind = slotIdx >= 0 ? kPanelKindOffsets[slotIdx].kind
                                  : IdentifyByVtable(panel);
    if (kind == PanelKind::Unknown) {
        LogUnknownPanelDiagnostics(panel);
        return kind;
    }
    LogIdentifiedOnce(panel, kind);
    return kind;
}

bool IsPanelKindInGameMenu(void* panel) {
    return IdentifyPanel(panel) == PanelKind::InGameMenu;
}

void* FindPanelByKind(PanelKind kind) {
    constexpr int kCap = 32;
    void* panels[kCap];
    int n = ReadPanelArray(GetGuiManager(), panels, kCap);
    for (int i = 0; i < n; ++i) {
        void* p = panels[i];
        if (!p) continue;
        if (IdentifyPanel(p) == kind) return p;
    }
    return nullptr;
}

bool IsMainMenuOptionsSubScreen(PanelKind k) {
    switch (k) {
    case PanelKind::SoundSettings:
    case PanelKind::AdvancedSoundSettings:
    case PanelKind::GraphicsSettings:
    case PanelKind::AdvancedGraphicsSettings:
    case PanelKind::AutoPauseOptions:
    case PanelKind::FeedbackOptions:
    case PanelKind::GameSettings:
    case PanelKind::MouseSettings:
    case PanelKind::KeyboardMapping:
        return true;
    default:
        return false;
    }
}

bool IsModalPopupPanel(PanelKind k) {
    switch (k) {
    case PanelKind::MessageBoxModal:
    case PanelKind::TutorialBox:
    case PanelKind::AreaTransition:
    case PanelKind::StatusSummary:
    case PanelKind::ControllerLossBox:
    case PanelKind::SkillInfoBox:
    case PanelKind::SoloModeQuery:
        return true;
    default:
        return false;
    }
}

}  // namespace acc::engine
