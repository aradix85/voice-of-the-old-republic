# menus.cpp (1019 lines)

Source: `patches/Accessibility/menus.cpp`

menu-side hook handlers (chain navigation, focus events, input dispatch, per-tick monitors).

Layering: log.{h,cpp}             file/debug logging primitives speech.{h,cpp}           NVDA controller client (LoadLibrary'd lazily) core_dllmain.cpp        DllMain + OnRulesInit + EnsureSpeechInitialized engine_input.{h,cpp}    InputIndices name table + manager translate engine_offsets.h        engine struct/vtable offset constants + engine structs engine_reads.{h,cpp}    SEH-guarded readers (CallDowncast, ReadGuiString, ...) engine_panels.{h,cpp}   PanelKind enum + CGuiInGame slot classification engine_manager.{h,cpp}  CSWGuiManager surface + cursor / click-sim PFNs this file               the menu-accessibility hook handlers

Phase 0 of the long-term nav plan extracted the foundation (core_dllmain + engine_*) out of the original monolithic Accessibility.cpp; the remaining menu-accessibility code is what lives here under the new name. Per plan, the menu-side logic is NOT decomposed further in Phase 0 (incremental refactor discipline) — see docs/navsystem-longterm-plan.md.

## Declarations (in source order)

- L145 `bool InMovieBurstWindow()`: True while the engine-GUI SetActive rebuild burst must stay silent because a cutscene movie is on screen — or was, within the last kMovieBurstTailMs.
- L162 `void MarkSpoken(int channel, const char* text)`
- L168 `void SpeakIfChanged(int channel, const char* text, unsigned repeatAfterMs)`
- L182 `bool AlreadySpokenOnAnyChannel(const char* text)`
- L241 `void* CurrentPanelIfLive()`: See menus_internal.h for why every deref of g_currentPanel has to come through here.
- L443 `extern "C" void __cdecl OnSetActiveControl(void* panel, void* newControl)`: GATE CLEARED for KOTOR 2 on 2026-07-31 — the first handler to run on both games.
- L571 `extern "C" void __cdecl OnListBoxSetActiveControl(void* listBox, void* newRow, int param2)`: CSWGuiListBox::SetActiveControl — hooked mid-function at 0x0041c16b.
- L811 `void ValidatePanels()`
- L830 `void TickMonitors()`
- L855 `void PollHomeEndKeys()`
- L891 `void TickPendingOps()`: Drain the menu-side pending-op queue.
- L915 `void DrainPendingAnnounce()`: Drain the pending-announce slot.
- L1004 `void ClearPendingAnnounce()`
- L1009 `bool IsDrilledIntoSubScreen()`
- L1010 `void SetDrilledIntoSubScreen(bool drilled)`
