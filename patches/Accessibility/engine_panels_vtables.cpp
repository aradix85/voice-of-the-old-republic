// The heap-allocated panels with no CGuiInGame slot, identified by their vtable.

#include "engine_panels_internal.h"

#include "engine_rebase.h"
#include "engine_offsets.h"  // CExoArrayList, kPanelControlsOffset, kVtableListBox

namespace acc::engine {

namespace {

// CSWGuiUpgrade (upgrade.gui / upgrade_p.gui) — the workbench slot-detail
// screen. Heap-allocated, single class per game, so vtable equality is the
// identifier: K1 symbol CSWGuiUpgrade_vtable, K2 from the RTTI census
// (docs/llm-docs/re/k2/k2-vtables.csv).
//
// This replaces an id-shape probe ("listbox at id 0 + buttons at the game's
// BTN_ASSEMBLE and one slot id"). That probe needed a per-game id table
// because upgrade_p.gui renumbers every control, which is precisely the
// .gui-authored-id trust the gui-id audit removes — one variant file and
// the workbench screen stops identifying at all.
const uintptr_t kVtableCSWGuiUpgrade = acc::addr::Pick(0x00757298, 0x009A88A4);

// CSWGuiInGameMessages, second instance. KOTOR 2's journal constructs its
// own messages panel (journal ctor 0x007FAE60 stores it at journal+0x1a20)
// and its "Message Log" button opens THAT one, which the CGuiInGame slot
// table never sees — every K2 log up to 2026-09-17 shows it as "first sight
// UNKNOWN panel ... vtable=0x0099e5ec". Vtable from the RTTI census
// (.?AVCSWGuiInGameMessages@@). KOTOR 1 has one instance only, reached by
// its slot, so no K1 value is needed.
const uintptr_t kVtableCSWGuiInGameMessages = acc::addr::Kotor2Only(0x0099E5EC);

// CSWGuiUpgradeItemSelect (upgradeitems.gui / upgradeitems_p.gui) — the
// workbench per-category item picker. Vtable equality, same reasoning as
// CSWGuiUpgrade above; K1 symbol CSWGuiUpgradeItemSelect_vtable, K2 from the
// RTTI census. Replaces a probe that keyed on "listbox at id 0 + buttons at
// ids 4 and 5 + a 4..8 control count" — a shape shared with several other
// small listbox panels and one variant .gui away from failing.
const uintptr_t kVtableCSWGuiUpgradeItemSelect =
    acc::addr::Pick(0x00757228, 0x009A876C);

// CSWGuiUpgradeSelection (upgradesel.gui — the workbench category chooser:
// Ranged / Lightsaber / Melee / Armor). Heap-allocated, single class, so the
// vtable is the identifier (symbol CSWGuiUpgradeSelection_vtable).
//
// The old id-based fallback ("a control at id 0 + buttons at ids 9 and 10")
// is deleted — same call as PowersLevelUp's below, for a sharper reason: that
// shape is not distinctive at all. The audit's own tripwires caught it
// mislabelling the K1 attributes screen (patch-20260818-204614) and the K2
// skills screen (patch-20260818-204814, where the crafting UpgradeSel
// resolvers then probed the wrong panel and logged GuiIdMismatch — those two
// lines meant "wrong panel", not "variant .gui"). Both are level-up
// sub-screens that merely happen to carry that id shape, which is exactly the
// .gui-authored-id trust this audit removes. The vtable constant sits behind
// the installer's SHA-256 gate, so "a build relocates the vtable" cannot
// happen without new constants anyway.
const uintptr_t kVtableCSWGuiUpgradeSelection = acc::addr::Pick(0x007571b0, 0x009A86CC);

// KOTOR 2 crafting screens: CSWGuiCreateItem (component_p.gui — the
// workbench "Create Items" / breakdown screen, pushed by the
// upgradesel BTN_CREATEITEMS handler) and CSWGuiCreateMedicalItem
// (chemical_p.gui — the lab-station twin). Both heap-allocated modals with
// no CGuiInGame slot; single K2-only class each, so vtable equality is the
// identifier (RTTI census 2026-07-31, k2-vtables.csv). Poisons to the
// unported sentinel on KOTOR 1; the Ok() guard declines without a read.
const uintptr_t kVtableCSWGuiCreateItem        = acc::addr::Kotor2Only(0x009A8B4C);
const uintptr_t kVtableCSWGuiCreateMedicalItem = acc::addr::Kotor2Only(0x009A8CBC);

// CSWGuiLevelUpPanel identity by vtable. The panel is heap-allocated by
// CSWGuiInGameCharacter::ShowLevelUpGUI when the user clicks Level Up,
// so it has no CGuiInGame slot. Lane's SARIF labels the vtable at
// 0x00759568 (verified via Ghidra ListSymbolsByName 2026-05-14).
const uintptr_t kVtableCSWGuiLevelUpPanel = acc::addr::Pick(0x00759568, 0x009AA1AC);

// Character-creation step panels by vtable. Heap-allocated, no CGuiInGame
// slot, single class each, so vtable equality is the identifier. Verified
// via Ghidra ListSymbolsByName (CSWGuiCustomPanel_vtable / CSWGuiQuickPanel_
// vtable) and against the live panels in patch-20260608-135543.log
// (custom=0x007595e0 21 controls, quick=0x00759668 12 controls). Both drive
// their build steps (Portrait / Attributes / Skills / Feats / Name /
// Play) sequentially, enabling one at a time via CSWGuiControl::SetEnabled
// (bit_flags bit 3) — same shape as the in-game level-up wizard.
const uintptr_t kVtableCSWGuiCustomPanel = acc::addr::Pick(0x007595e0, 0x009AA9A4);
const uintptr_t kVtableCSWGuiQuickPanel  = acc::addr::Pick(0x00759668, 0x009AAA24);

// CSWGuiOptions title-screen options panel identity by vtable. The class
// is single-instance and lives in the engine's title-screen UI suite, so
// vtable equality is the cleanest identifier. Captured 2026-05-26 via the
// LogUnknownPanelDiagnostics probe (PanelProbe block in
// patch-20260526-180650.log). Its real class name is CSWGuiOptionsMain — the
// constant predates knowing that, and the name is what the KOTOR 2 value was
// looked up by.
const uintptr_t kVtableCSWGuiOptions = acc::addr::Pick(0x00758838, 0x009A183C);

// CSWGuiMainMenu title-screen panel. Single-instance, vtable equality is
// the cleanest identifier. Captured 2026-05-30 in the user-reported
// stuck-menu log (patch-20260530-191714.log frame at 19:17:40):
// `PanelProbe: first sight UNKNOWN panel=077F49D8 vtable=0x00752f70`.
// Classifying this lets AnnouncePanelTitle skip the generic label-walk
// (which lands on the DLC notice) and speak Id::PanelTitleMainMenu.
const uintptr_t kVtableCSWGuiMainMenu = acc::addr::Pick(0x00752f70, 0x009A597C);

// CSWGuiPazaakStart side-deck builder. Heap-allocated modal (no CGuiInGame
// slot), single class, so vtable equality is the identifier. Verified against
// the live panel dump (patch-20260601-071641.log: 79-control panel,
// vtable=0x007532e8).
const uintptr_t kVtableCSWGuiPazaakStart = acc::addr::Pick(0x007532e8, 0x009A614C);

// CSWGuiWagerPopup — the wager popup pushed over the
// side-deck builder. Single class, heap-allocated, so vtable equality is the
// identifier (CSWGuiWagerPopup_vtable, verified against the live panel dump in
// patch-20260601-090245.log: 8-control panel, vtable=0x007534c8).
const uintptr_t kVtableCSWGuiWagerPopup = acc::addr::Pick(0x007534c8, 0x009A6034);

// CSWGuiQuestItem — the journal's "Quest Items" sub-screen. Single
// class, heap-allocated and owned by the journal (no CGuiInGame slot), so
// vtable equality is the identifier (CSWGuiQuestItem_vtable, verified against
// the live panel dump in patch-20260603-090028.log: 3-element chain with a
// BTN_BACK at the bottom).
//
// KOTOR 2 has no such panel. Its RTTI names 122 CSWGui classes and covers every
// one KOTOR 1 has except this and CSWGuiScriptSelect below; the exe holds no
// "questitem" string either. K2's journal drops the quest-items sub-screen.
const uintptr_t kVtableCSWGuiQuestItem = acc::addr::Kotor1Only(0x00757c20);

// CSWGuiScriptSelect — the character sheet's "Shortcuts" combat-behaviour
// picker. Heap-allocated modal with no CGuiInGame slot, so vtable equality is
// the identifier (verified via Ghidra: destructor at 0x006e9ef0, vtable label
// at 0x007590a8, against the live panel dump in patch-20260613-205358.log).
//
// The other class KOTOR 2 does not have — see CSWGuiQuestItem above for the
// evidence. K2 exposes combat behaviour differently and ships no
// CSWGuiScriptSelect.
const uintptr_t kVtableCSWGuiScriptSelect = acc::addr::Kotor1Only(0x007590a8);

// Title-screen Options sub-screens. Each is a single-instance heap-allocated
// CSWGui* option panel with no CGuiInGame slot, so — like MainMenu / Options /
// Pazaak — vtable equality is the cleanest identifier. Vtables captured
// 2026-06-13 from the PanelProbe "first sight UNKNOWN" dumps in
// patch-20260613-194918.log; each is paired there with the title the
// label-walk speaks (e.g. 0x007587c0 → "Sound Settings"). GoG-derived
// addresses match the Steam build (see memory ghidra_gog_steam_bytes_match),
// matching every other hardcoded title-screen vtable above.
//
// KOTOR 2 values by RTTI class name (all nine classes exist there). The class
// each PanelKind corresponds to is named in the comment, because the KOTOR 1
// column was captured from a live probe and carries no name of its own.
struct OptionsSubScreenVtable {
    uintptr_t vtable;
    PanelKind kind;
};
static const OptionsSubScreenVtable kOptionsSubScreenVtables[] = {
    // CSWGuiOptionsSound
    { acc::addr::Pick(0x007587c0, 0x009A1D8C), PanelKind::SoundSettings            },
    // CSWGuiOptionsSoundAdvanced
    { acc::addr::Pick(0x00758550, 0x009A1E4C), PanelKind::AdvancedSoundSettings    },
    // CSWGuiOptionsGraphics
    { acc::addr::Pick(0x007586f8, 0x009A1A9C), PanelKind::GraphicsSettings         },
    // CSWGuiOptionsGraphicsAdvanced
    { acc::addr::Pick(0x007584a0, 0x009A1BF4), PanelKind::AdvancedGraphicsSettings },
    // CSWGuiInGameAutoPause
    { acc::addr::Pick(0x00758ee0, 0x009A8FCC), PanelKind::AutoPauseOptions         },
    // CSWGuiOptionsFeedback
    { acc::addr::Pick(0x007581e8, 0x009A18DC), PanelKind::FeedbackOptions          },
    // CSWGuiInGameGameplay
    { acc::addr::Pick(0x00758e00, 0x009A8EE4), PanelKind::GameSettings             },
    // CSWGuiOptionsMouse
    { acc::addr::Pick(0x007585f8, 0x009A1EFC), PanelKind::MouseSettings            },
    // CSWGuiInGameOptKeyMappings
    { acc::addr::Pick(0x00759358, 0x009A9EA4), PanelKind::KeyboardMapping          },
};

}  // namespace

// Declared in engine_panels.h. Lives above the anonymous namespace so the
// detectors below AND the menus layer can both reach it — see the header
// for what it is for.
bool HasVtable(void* obj, uintptr_t expected) {
    if (!obj) return false;
    __try {
        void** vt = *reinterpret_cast<void***>(obj);
        return reinterpret_cast<uintptr_t>(vt) == expected;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// CSWGuiSaveLoad. The panel is allocated dynamically when the user
// activates load/save and has no slot in CGuiInGame, so the offset table
// can't catch it — but as a heap-allocated panel it carries its own
// vtable, and the vtable IS its identity (ctor-written, per-exe constant,
// immune to .gui renumbering). Replaced the old .gui-id quartet probe in
// the gui-id audit (docs/gui-id-audit.md); that shape needed a
// belt-and-braces defence against upgrade.gui's identical id quartet and
// broke by construction on installs with a variant saveload.gui.
//
// Mirror of acc::menus::detail::IsSaveLoadPanel — kept here so engine-layer
// IdentifyPanel doesn't reach back into the menus layer.

bool IsSaveLoadStructural(void* panel) {
    return HasVtable(panel, kVtableCSWGuiSaveLoad);
}

bool IsWorkbenchUpgradeStructural(void* panel) {
    return HasVtable(panel, kVtableCSWGuiUpgrade);
}

bool IsInGameMessagesStructural(void* panel) {
    if (!acc::addr::Ok(kVtableCSWGuiInGameMessages)) return false;
    return HasVtable(panel, kVtableCSWGuiInGameMessages);
}

bool IsWorkbenchItemsStructural(void* panel) {
    return HasVtable(panel, kVtableCSWGuiUpgradeItemSelect);
}

bool IsWorkbenchSelectStructural(void* panel) {
    return HasVtable(panel, kVtableCSWGuiUpgradeSelection);
}

bool IsWorkbenchCreateItemStructural(void* panel) {
    if (!panel) return false;
    if (!acc::addr::Ok(kVtableCSWGuiCreateItem)) return false;
    __try {
        void** vt = *reinterpret_cast<void***>(panel);
        return reinterpret_cast<uintptr_t>(vt) == kVtableCSWGuiCreateItem;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsWorkbenchCreateMedicalStructural(void* panel) {
    if (!panel) return false;
    if (!acc::addr::Ok(kVtableCSWGuiCreateMedicalItem)) return false;
    __try {
        void** vt = *reinterpret_cast<void***>(panel);
        return reinterpret_cast<uintptr_t>(vt) == kVtableCSWGuiCreateMedicalItem;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsLevelUpStructural(void* panel) {
    return HasVtable(panel, kVtableCSWGuiLevelUpPanel);
}

bool IsCharGenStructural(void* panel) {
    if (!panel) return false;
    __try {
        void** vt = *reinterpret_cast<void***>(panel);
        uintptr_t v = reinterpret_cast<uintptr_t>(vt);
        return v == kVtableCSWGuiCustomPanel || v == kVtableCSWGuiQuickPanel;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsMainMenuOptionsStructural(void* panel) {
    return HasVtable(panel, kVtableCSWGuiOptions);
}

bool IsMainMenuStructural(void* panel) {
    return HasVtable(panel, kVtableCSWGuiMainMenu);
}

bool IsPazaakStartStructural(void* panel) {
    return HasVtable(panel, kVtableCSWGuiPazaakStart);
}

bool IsPazaakWagerStructural(void* panel) {
    return HasVtable(panel, kVtableCSWGuiWagerPopup);
}

bool IsQuestItemStructural(void* panel) {
    return HasVtable(panel, kVtableCSWGuiQuestItem);
}

bool IsScriptSelectStructural(void* panel) {
    return HasVtable(panel, kVtableCSWGuiScriptSelect);
}

// CSWGuiPowersLevelUp picker (pwrlvlup.gui). The same class backs both the
// chargen Force-selection screen and the InGameLevelUp Force sub-screen.
// Vtable equality only — the ctor-written identity of this single-instance
// heap panel, same as feats / level-up / main menu / SaveLoad. The old
// id-based structural fallback ({listbox 6, listbox 7, buttons 11/12})
// was deleted in the gui-id audit: it trusted .gui-authored ids (this
// panel's are re-numbered per GAME, let alone per mod — the K1 quartet
// never matched on K2 at all), and the vtable constant sits behind the
// installer's SHA-256 gate, so "a build relocates the vtable" can't
// happen without new constants anyway.
bool IsPowersLevelUpStructural(void* panel) {
    return HasVtable(panel, kVtableCSWGuiPowersLevelUp);
}

// Returns the specific sub-screen kind, or Unknown if `panel`'s vtable
// isn't one of the nine. One detector covers all nine (vs. nine near-
// identical IsXStructural functions) because the only distinguishing
// signal is the vtable.
PanelKind IdentifyOptionsSubScreen(void* panel) {
    if (!panel) return PanelKind::Unknown;
    __try {
        uintptr_t vt = reinterpret_cast<uintptr_t>(*reinterpret_cast<void***>(panel));
        for (const auto& e : kOptionsSubScreenVtables) {
            if (e.vtable == vt) return e.kind;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return PanelKind::Unknown;
}

}  // namespace acc::engine
