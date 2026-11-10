
#include "engine_panels_internal.h"

#include "engine_manager.h"  // kAddrGuiManagerPtr, kMgrPanels*Offset, GetForegroundPanel
#include "engine_offsets.h"  // CExoArrayList, kPanelControlsOffset, kVtableListBox
#include "engine_reads.h"    // ReadGuiString
#include "log.h"

namespace acc::engine {

namespace {

// CGuiInGame resolution chain. Address values verified against Lane's
// SARIF (CAppManager_vtable @ 0x007A39FC). Field offsets from the struct
// definitions in docs/llm-docs/re/swkotor.exe.h.
// (values live in engine_panels_internal.h — engine_panels_state.cpp
// walks the same chain)
// One row per named CGuiInGame slot. Adding a panel kind = add an enum
// value in engine_panels.h and a row here with its CGuiInGame field offset.
//
// Rows with `offset == kNoSlotOffset` are heap-allocated panels with no
// fixed CGuiInGame field — they are skipped during slot lookup and only
// referenced by PanelKindName for friendly-name resolution. They are
// identified structurally inside IdentifyPanel.
constexpr size_t kNoSlotOffset = static_cast<size_t>(-1);

struct PanelKindOffset {
    size_t      offset;
    PanelKind   kind;
    const char* name;
};

// KOTOR 2 values recovered 2026-08-01 by tools/re-scripts/k2_slot_table.py,
// which reads each slot off the `mov [this+off], eax` that files away the
// panel CGuiInGame's creator just constructed. See docs/kotor2-port.md.
//
// The shape: identical up to +0x74, then KOTOR 2 inserts members and every
// slot above shifts. So this table is a mixture and `Same` is NOT the default
// — InGameMessages in particular moves 0x1c -> 0x78, which on KOTOR 1 is
// PartySelection, so an unported table maps those two screens onto each other
// and misclassifies in silence rather than failing.
//
// Todo() rows are slots whose KOTOR 2 counterpart is not yet located. They
// poison to kUnportedOffset there and SlotTableLookup skips them, so the rest
// of the walk still runs; each simply falls through to structural / vtable
// identification the way it did before this port.
static const PanelKindOffset kPanelKindOffsets[] = {
    // Witnessed on both games (2026-08-01): ShowSWInGameGui / HideSWInGameGui
    // add/remove the [this+0x08] panel as the strip menu in both decompiles.
    { acc::off::Same(0x08), PanelKind::InGameMenu,          "InGameMenu" },
    { acc::off::Same(0x0c), PanelKind::InGameEquip,         "InGameEquip" },
    { acc::off::Same(0x10), PanelKind::InGameInventory,     "InGameInventory" },
    // RESOLVED 2026-08-02: KOTOR 2 stores its character sheet at the SAME
    // slot. The panel creator (0x007BE4C0) constructs it at 0x0084C3A0 —
    // which vtable_xrefs proves stores the CSWGuiInGameCharacter vtable
    // (0x009A3E7C) — and writes the result with `MOV [gui+0x14],EAX` at
    // 0x007BF439. The earlier "K2 puts a CSWGui3DSceneView here" note was
    // the slot-table tool misattributing a neighbouring allocation, the same
    // way it misread the DialogComputerCamera slot as BlackenedLabel.
    // Until this was resolved the character sheet was the one sub-screen
    // that never announced on KOTOR 2 (patch-20260801-232432.log: six
    // sub-screens spoke, InGameCharacter never appeared at all).
    { acc::off::Same(0x14), PanelKind::InGameCharacter,     "InGameCharacter" },
    { acc::off::Same(0x18), PanelKind::InGameAbilities,     "InGameAbilities" },
    { acc::off::Pick(0x1c, 0x78), PanelKind::InGameMessages, "InGameMessages" },
    { acc::off::Same(0x20), PanelKind::InGameJournal,       "InGameJournal" },
    { acc::off::Same(0x24), PanelKind::InGameMap,           "InGameMap" },
    { acc::off::Same(0x28), PanelKind::InGameOptions,       "InGameOptions" },
    // 0x3c is the ACTIVE-dialog-panel pointer (aliases 0x40 or 0x44), not a
    // creator-built slot — Same on K2, witnessed by the K2 helper 0x007CB750
    // (`[gui+0x3c] && [gui+0x3c]==[gui+0x40]`), which is K1's inline
    // dialog_cinematic_copy_==dialog_cinematic check factored out.
    { acc::off::Same(0x3c), PanelKind::DialogCinematicCopy, "DialogCinematicCopy" },
    { acc::off::Same(0x40), PanelKind::DialogCinematic,     "DialogCinematic" },
    { acc::off::Same(0x44), PanelKind::DialogComputer,      "DialogComputer" },
    // Same(0x48) witnessed directly: the panel creator stores the
    // CSWGuiDialogComputerCamera ctor result (0x008BD910) via
    // `MOV [gui+0x48],ECX` at 0x007BE9D4. Batch 2's note that
    // CSWGuiBlackenedLabel sits here was the slot-table tool misattributing
    // the immediately-following BlackenedLabel allocation.
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
    // KOTOR 2's 0x78 is InGameMessages (above), so this must NOT inherit it.
    // KOTOR 2 swaps this slot with InGameMessages: K1 files PartySelection
    // at 0x78 and Messages at 0x1c; K2's creator (0x007BE4C0) stores the
    // CSWGuiPartySelection ctor result (0x0089CF30) at [gui+0x1c].
    { acc::off::Pick(0x78, 0x1c), PanelKind::PartySelection, "PartySelection" },
    { acc::off::Same(0x7c), PanelKind::InGamePause,         "InGamePause" },
    // 0x80 holds CSWGui3DSceneView on KOTOR 2.
    // Same slot on both games: K2 creator stores the CSWGuiInGameGalaxyMap
    // ctor result (0x008973D0) at [gui+0x80].
    { acc::off::Same(0x80), PanelKind::InGameGalaxyMap,     "InGameGalaxyMap" },
    { acc::off::Same(0x84), PanelKind::Store,               "Store" },
    { acc::off::Pick(0x8c, 0x94), PanelKind::SoloModeQuery, "SoloModeQuery" },
    // KOTOR 2 witnessed by SetSWGuiStatus @0x007C9C40: status 1 adds, any
    // other status removes, the panel at [this+0x98] — the exact behaviour
    // KOTOR 1's SetSWGuiStatus shows for main_interface at +0x90.
    { acc::off::Pick(0x90, 0x98), PanelKind::MainInterface, "MainInterface" },
    { acc::off::Pick(0x94, 0x9c), PanelKind::AreaTransition, "AreaTransition" },
    // KOTOR 2 files three CSWGuiMessageBox instances (0xa0, 0xa4, one more not
    // yet followed); KOTOR 1 tracks only the modal one. 0xa0 is the first.
    { acc::off::Pick(0x98, 0xa0), PanelKind::MessageBoxModal, "MessageBox" },
    { acc::off::Pick(0x9c, 0xac), PanelKind::SkillInfoBox,  "SkillInfoBox" },
    { acc::off::Pick(0xa0, 0xb0), PanelKind::TutorialBox,   "TutorialBox" },
    // KOTOR 2's 0xa4 is a CSWGuiMessageBox, not the controller-loss box —
    // which it does have as a class (CSWGuiControllerLossBox in the RTTI).
    // K2 creator stores the ControllerLossBox at [gui+0xb4] (the ctor is
    // inlined there — the CSWGuiControllerLossBox vftable store is visible
    // in the creator body itself). K2's 0xa4 is a second MessageBox
    // instance (ctor 0x0075B370 with arg 1), not this box.
    { acc::off::Pick(0xa4, 0xb4), PanelKind::ControllerLossBox, "ControllerLossBox" },
    { acc::off::Pick(0xa8, 0xb8), PanelKind::StatusSummary, "StatusSummary" },
    // Dialogue input-routing surfaces (per CGuiInGame layout in
    // swkotor.exe.h:10282). The in-game session log shows that during
    // a CSWGuiDialogCinematic conversation, arrow-key input routes to
    // a separate foreground panel (0FDEE418 in patch-20260502-182804.log)
    // distinct from the rendering panel (DialogCinematicCopy at +0x3c).
    // Hypothesis: that routing target is one of these two — registering
    // both so the next log identifies which.
    // KOTOR 1 only: K2's creator stores no panel slot past +0xb8, and the
    // +0xf8..0x11f region there belongs to other fields (message rings at
    // +0x110/+0x118). On K2 these kinds fall through to the vtable detector.
    { acc::off::Kotor1Only(0xf8), PanelKind::DialogMessagesAux, "DialogMessagesAux" },
    { acc::off::Kotor1Only(0xfc), PanelKind::DialogMessages,    "DialogMessages" },
    // Heap-allocated kinds with no CGuiInGame slot. The sentinel offset
    // skips them during slot lookup; PanelKindName still resolves the
    // friendly name, and IdentifyPanel falls through to a structural
    // detector below.
    { kNoSlotOffset, PanelKind::SaveLoad,          "SaveLoad" },
    { kNoSlotOffset, PanelKind::InGameLevelUp,     "InGameLevelUp" },
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
static constexpr int kPanelKindOffsetCount =
    sizeof(kPanelKindOffsets) / sizeof(kPanelKindOffsets[0]);

// (panel, kind) pairs already logged. Keeps the log tidy when persistent
// panels (HUD) get re-checked on every input event. FIFO-evicted at cap.
struct PanelKindCacheEntry {
    void*     panel;
    PanelKind kind;
};
static constexpr int kPanelKindCacheSize = 32;
static PanelKindCacheEntry g_panelKindCache[kPanelKindCacheSize];
static int g_panelKindCacheCount = 0;

// Unknown-panel probe. First-sight diagnostic dump for panels that miss
// the slot table AND every structural detector — the canonical case is
// the title-screen Options panel (CGuiInGame isn't resolvable pre-game,
// so the slot scan is skipped entirely, and no detector currently knows
// its shape). Dedup is by panel *vtable* (not panel pointer) so the dump
// fires exactly once per unique panel class across the whole session —
// re-opening Options reuses the same class so we don't re-log.
//
// What we capture: panel vtable, panel.controls.size, and per-control
// {vtable, .gui-id at +0x50, button-or-label rendered text}. That's
// enough to write a structural detector matching SaveLoad / Workbench
// shapes once the user sends us the log line.
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
        uintptr_t cvt = 0;
        int       cid = -1;
        __try {
            cvt = reinterpret_cast<uintptr_t>(*reinterpret_cast<void**>(c));
            cid = *reinterpret_cast<int*>(
                reinterpret_cast<unsigned char*>(c) + kControlIdOffset);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            cvt = 0;
            cid = -1;
        }
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

// The CGuiInGame slot walk, hoisted out of IdentifyPanel so the __try owns no
// unwinding objects (C2712) and returns the matching table index, or -1.
//
// The SEH is not optional. Before it existed this walk ran on KOTOR 2 against
// a KOTOR 1 pointer chain and dereferenced a garbage base, taking the process
// down within the first Update ticks of a session (first Batch 1 test round,
// 2026-07-31: WER fault inside accessibility.dll at this very compare). It
// stays on KOTOR 1 too — the walk reads engine memory through a multi-hop
// pointer chain that a mid-teardown frame can invalidate.
//
// Runs on BOTH games since Batch 2 (2026-08-01), when the slot table gained
// its KOTOR 2 column. Rows still on Todo() poison to kUnportedOffset there and
// are skipped alongside the no-slot sentinel, so an unported row costs that
// one kind its slot-table identification — it falls through to the structural
// and vtable detectors, exactly as it did before the port — and never a fault
// that would abandon the rest of the walk.
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

    auto recordAndReturn = [&](PanelKind k, const char* name) -> PanelKind {
        // First-sight log per (panel, kind) pair.
        for (int j = 0; j < g_panelKindCacheCount; ++j) {
            if (g_panelKindCache[j].panel == panel &&
                g_panelKindCache[j].kind  == k) {
                return k;  // already logged
            }
        }
        if (g_panelKindCacheCount >= kPanelKindCacheSize) {
            memmove(g_panelKindCache, g_panelKindCache + 1,
                    sizeof(g_panelKindCache[0]) * (kPanelKindCacheSize - 1));
            g_panelKindCacheCount = kPanelKindCacheSize - 1;
        }
        g_panelKindCache[g_panelKindCacheCount++] = { panel, k };
        acclog::Write("PanelKind", "panel=%p identified as %s", panel, name);
        return k;
    };

    int slotIdx = SlotTableLookup(panel);
    if (slotIdx >= 0) {
        return recordAndReturn(kPanelKindOffsets[slotIdx].kind,
                               kPanelKindOffsets[slotIdx].name);
    }

    // Slot-table miss: structural detectors for heap-allocated kinds. Run
    // unconditionally on every miss — these panels live transiently and
    // we don't know in advance whether `panel` is currently one of them.
    // Cost is bounded (a few control-id walks per probe) and only paid
    // when the slot table didn't classify the panel.
    //
    // Probe order: vtable-equality detectors first — collision-proof and
    // cheap — then the remaining .gui-shape probes, tighter (more-
    // distinctive) signatures before looser ones.
    // SaveLoad identifies by vtable since the gui-id audit (it used to be
    // a .gui-id quartet probe that collided with upgrade.gui and had to
    // run AFTER the workbench shapes; that ordering constraint is gone).
    if (IsSaveLoadStructural(panel)) {
        return recordAndReturn(PanelKind::SaveLoad, "SaveLoad");
    }
    // K2 crafting screens — vtable equality, collision-proof, probe early.
    if (IsWorkbenchCreateItemStructural(panel)) {
        return recordAndReturn(PanelKind::WorkbenchCreateItem, "WorkbenchCreateItem");
    }
    if (IsWorkbenchCreateMedicalStructural(panel)) {
        return recordAndReturn(PanelKind::WorkbenchCreateMedical, "WorkbenchCreateMedical");
    }
    if (IsWorkbenchUpgradeStructural(panel)) {
        return recordAndReturn(PanelKind::WorkbenchUpgrade, "WorkbenchUpgrade");
    }
    if (IsInGameMessagesStructural(panel)) {
        return recordAndReturn(PanelKind::InGameMessages, "InGameMessages");
    }
    if (IsWorkbenchItemsStructural(panel)) {
        return recordAndReturn(PanelKind::WorkbenchItems, "WorkbenchItems");
    }
    // Order here is now historical rather than load-bearing: every workbench
    // probe below identifies by vtable, so none of them can claim a panel
    // that isn't theirs. It used to matter — WorkbenchSelect's loose id-shape
    // fallback ("id 0 present + id 9 button + id 10 button") also matched the
    // force-power picker (id 0 placeholder, id 9 "Recommended", id 10
    // BTN_SELECT) and stole it, silencing the skill tree, which is why
    // PowersLevelUp was moved ahead of it. Both fallbacks are gone; keeping
    // the tighter detector first still costs nothing.
    if (IsPowersLevelUpStructural(panel)) {
        return recordAndReturn(PanelKind::PowersLevelUp, "PowersLevelUp");
    }
    if (IsWorkbenchSelectStructural(panel)) {
        return recordAndReturn(PanelKind::WorkbenchSelect, "WorkbenchSelect");
    }
    if (IsLevelUpStructural(panel)) {
        return recordAndReturn(PanelKind::InGameLevelUp, "InGameLevelUp");
    }
    if (IsCharGenStructural(panel)) {
        return recordAndReturn(PanelKind::CharGen, "CharGen");
    }
    if (IsMainMenuOptionsStructural(panel)) {
        return recordAndReturn(PanelKind::MainMenuOptions, "MainMenuOptions");
    }
    if (IsMainMenuStructural(panel)) {
        return recordAndReturn(PanelKind::MainMenu, "MainMenu");
    }
    if (IsPazaakStartStructural(panel)) {
        return recordAndReturn(PanelKind::PazaakStart, "PazaakStart");
    }
    if (IsPazaakWagerStructural(panel)) {
        return recordAndReturn(PanelKind::PazaakWager, "PazaakWager");
    }
    if (IsQuestItemStructural(panel)) {
        return recordAndReturn(PanelKind::InGameQuestItems, "InGameQuestItems");
    }
    if (IsScriptSelectStructural(panel)) {
        return recordAndReturn(PanelKind::ScriptSelect, "ScriptSelect");
    }
    // Title-screen Options sub-screens (Sound / Graphics / Advanced variants /
    // Auto-Pause / Feedback / Game / Mouse / Keyboard). One vtable-table probe
    // covers all nine.
    if (PanelKind k = IdentifyOptionsSubScreen(panel); k != PanelKind::Unknown) {
        return recordAndReturn(k, PanelKindName(k));
    }

    // Last resort: dump diagnostics so we can write a structural detector
    // for this shape later. Deduped by panel vtable so we get exactly one
    // log block per unique panel class — title-screen Options, future
    // mod-added screens, etc.
    LogUnknownPanelDiagnostics(panel);
    return PanelKind::Unknown;
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
