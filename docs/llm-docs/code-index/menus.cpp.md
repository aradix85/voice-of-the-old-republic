# menus.cpp (1032 lines)

Source: `patches/Accessibility/menus.cpp`

menu-side hook handlers (chain navigation, focus events, input dispatch, per-tick monitors).

Layering: log.{h,cpp}             file/debug logging primitives prism.{h,cpp}            screen-reader bridge (LoadLibrary'd lazily) core_dllmain.cpp        DllMain + OnRulesInit + EnsurePrismInitialized engine_input.{h,cpp}    InputIndices name table + manager translate engine_offsets.h        engine struct/vtable offset constants + engine structs engine_reads.{h,cpp}    SEH-guarded readers (CallDowncast, ReadGuiString, ...) engine_panels.{h,cpp}   PanelKind enum + CGuiInGame slot classification engine_manager.{h,cpp}  CSWGuiManager surface + cursor / click-sim PFNs this file               the menu-accessibility hook handlers

Phase 0 of the long-term nav plan extracted the foundation (core_dllmain + engine_*) out of the original monolithic Accessibility.cpp; the remaining menu-accessibility code is what lives here under the new name. Per plan, the menu-side logic is NOT decomposed further in Phase 0 (incremental refactor discipline) — see docs/navsystem-longterm-plan.md.

## Declarations (in source order)

- L149 `bool InMovieBurstWindow()`: True while the engine-GUI SetActive rebuild burst must stay silent because a cutscene movie is on screen — or was, within the last kMovieBurstTailMs.
- L166 `void MarkSpoken(int channel, const char* text)`
- L172 `void SpeakIfChanged(int channel, const char* text, unsigned repeatAfterMs)`
- L186 `bool AlreadySpokenOnAnyChannel(const char* text)`
- L254 `void* CurrentPanelIfLive()`: See menus_internal.h for why every deref of g_currentPanel has to come through here.
- L456 `extern "C" void __cdecl OnSetActiveControl(void* panel, void* newControl)`: GATE CLEARED for KOTOR 2 on 2026-07-31 — the first handler to run on both games.
- L584 `extern "C" void __cdecl OnListBoxSetActiveControl(void* listBox, void* newRow, int param2)`: CSWGuiListBox::SetActiveControl — hooked mid-function at 0x0041c16b.
- L824 `void ValidatePanels()`
- L843 `void TickMonitors()`
- L868 `void PollHomeEndKeys()`
- L904 `void TickPendingOps()`: Drain the menu-side pending-op queue.
- L928 `void DrainPendingAnnounce()`: Drain the pending-announce slot.
- L1017 `void ClearPendingAnnounce()`
- L1022 `bool IsDrilledIntoSubScreen()`
- L1023 `void SetDrilledIntoSubScreen(bool drilled)`
