// The heap-allocated panels with no CGuiInGame slot, identified by their
// vtable: each is a single class written by its constructor, a per-exe
// constant that no .gui file can change. K1 values are symbol-table or
// live-probe captures, K2 values come from the RTTI census
// (docs/llm-docs/re/k2/k2-vtables.csv).

#include "engine_panels_internal.h"

#include "engine_addr.h"
#include "engine_offsets.h"

namespace acc::engine {

bool HasVtable(void* obj, uintptr_t expected) {
    if (!obj) return false;
    __try {
        void** vt = *reinterpret_cast<void***>(obj);
        return reinterpret_cast<uintptr_t>(vt) == expected;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

namespace {

struct VtableKind {
    uintptr_t vtable;
    PanelKind kind;
};

// In probe order. A vtable the running game lacks (Kotor1Only/Kotor2Only)
// is skipped.
const VtableKind kVtableKinds[] = {
    // CSWGuiSaveLoad, allocated when the user picks Load or Save.
    { kVtableCSWGuiSaveLoad, PanelKind::SaveLoad },
    // KOTOR 2 crafting: CSWGuiCreateItem (component_p, the workbench "Create
    // Items" / breakdown screen) and CSWGuiCreateMedicalItem (chemical_p, the
    // lab station).
    { acc::addr::Kotor2Only(0x009A8B4C), PanelKind::WorkbenchCreateItem },
    { acc::addr::Kotor2Only(0x009A8CBC), PanelKind::WorkbenchCreateMedical },
    // CSWGuiUpgrade (upgrade / upgrade_p), the workbench slot-detail screen.
    { acc::addr::Pick(0x00757298, 0x009A88A4), PanelKind::WorkbenchUpgrade },
    // CSWGuiInGameMessages, KOTOR 2's second instance: the journal builds its
    // own (journal ctor 0x007FAE60, at journal+0x1a20) and its "Message Log"
    // button opens that one, which the slot table never sees. KOTOR 1 has one
    // instance, reached by its slot.
    { acc::addr::Kotor2Only(0x0099E5EC), PanelKind::InGameMessages },
    // CSWGuiUpgradeItemSelect (upgradeitems / upgradeitems_p), the workbench
    // per-category item picker.
    { acc::addr::Pick(0x00757228, 0x009A876C), PanelKind::WorkbenchItems },
    // CSWGuiPowersLevelUp (pwrlvlup): both the chargen Force selection and
    // the level-up Force sub-screen.
    { kVtableCSWGuiPowersLevelUp, PanelKind::PowersLevelUp },
    // CSWGuiUpgradeSelection (upgradesel), the workbench category chooser.
    { acc::addr::Pick(0x007571b0, 0x009A86CC), PanelKind::WorkbenchSelect },
    // CSWGuiLevelUpPanel, allocated by CSWGuiInGameCharacter::ShowLevelUpGUI.
    { acc::addr::Pick(0x00759568, 0x009AA1AC), PanelKind::InGameLevelUp },
    // Character creation: CSWGuiCustomPanel and CSWGuiQuickPanel. Both enable
    // their build steps one at a time (CSWGuiControl::SetEnabled, bit_flags
    // bit 3), like the level-up wizard.
    { acc::addr::Pick(0x007595e0, 0x009AA9A4), PanelKind::CharGen },
    { acc::addr::Pick(0x00759668, 0x009AAA24), PanelKind::CharGen },
    // CSWGuiOptionsMain, the title-screen options panel.
    { acc::addr::Pick(0x00758838, 0x009A183C), PanelKind::MainMenuOptions },
    // CSWGuiMainMenu, the title screen.
    { acc::addr::Pick(0x00752f70, 0x009A597C), PanelKind::MainMenu },
    // CSWGuiPazaakStart, the side-deck builder.
    { acc::addr::Pick(0x007532e8, 0x009A614C), PanelKind::PazaakStart },
    // CSWGuiWagerPopup, pushed over the side-deck builder.
    { acc::addr::Pick(0x007534c8, 0x009A6034), PanelKind::PazaakWager },
    // CSWGuiQuestItem, the journal's "Quest Items" sub-screen, and
    // CSWGuiScriptSelect, the character sheet's combat-behaviour picker:
    // the two CSWGui classes KOTOR 2 does not have (its RTTI names every
    // other KOTOR 1 one).
    { acc::addr::Kotor1Only(0x00757c20), PanelKind::InGameQuestItems },
    { acc::addr::Kotor1Only(0x007590a8), PanelKind::ScriptSelect },
    // The title-screen Options sub-screens: CSWGuiOptionsSound, ...SoundAdvanced,
    // ...Graphics, ...GraphicsAdvanced, CSWGuiInGameAutoPause,
    // CSWGuiOptionsFeedback, CSWGuiInGameGameplay, CSWGuiOptionsMouse and
    // CSWGuiInGameOptKeyMappings.
    { acc::addr::Pick(0x007587c0, 0x009A1D8C), PanelKind::SoundSettings },
    { acc::addr::Pick(0x00758550, 0x009A1E4C), PanelKind::AdvancedSoundSettings },
    { acc::addr::Pick(0x007586f8, 0x009A1A9C), PanelKind::GraphicsSettings },
    { acc::addr::Pick(0x007584a0, 0x009A1BF4), PanelKind::AdvancedGraphicsSettings },
    { acc::addr::Pick(0x00758ee0, 0x009A8FCC), PanelKind::AutoPauseOptions },
    { acc::addr::Pick(0x007581e8, 0x009A18DC), PanelKind::FeedbackOptions },
    { acc::addr::Pick(0x00758e00, 0x009A8EE4), PanelKind::GameSettings },
    { acc::addr::Pick(0x007585f8, 0x009A1EFC), PanelKind::MouseSettings },
    { acc::addr::Pick(0x00759358, 0x009A9EA4), PanelKind::KeyboardMapping },
};

}  // namespace

PanelKind IdentifyByVtable(void* panel) {
    if (!panel) return PanelKind::Unknown;
    uintptr_t vt = 0;
    __try {
        vt = reinterpret_cast<uintptr_t>(*reinterpret_cast<void***>(panel));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return PanelKind::Unknown;
    }
    for (const auto& e : kVtableKinds) {
        if (acc::addr::Ok(e.vtable) && e.vtable == vt) return e.kind;
    }
    return PanelKind::Unknown;
}

}  // namespace acc::engine
