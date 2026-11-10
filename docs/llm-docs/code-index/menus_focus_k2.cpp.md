# menus_focus_k2.cpp (521 lines)

Source: `patches/Accessibility/menus_focus_k2.cpp`

KOTOR 2 focus announce — the port's first vertical slice.

Why this exists instead of running the KOTOR 1 handler ----------------------------------------------------- OnSetActiveControl's KOTOR 1 body is not portable yet, and the blocker is not the handler itself but everything it reaches: IsLoadingSaveGame dispatches through an address that resolves to 0 on KOTOR 2, and IdentifyPanel needs the CGuiInGame slot table, which is a description of ONE binary's layout and almost certainly reordered in a game that added screens KOTOR 1 never had. Porting that whole graph before anything can be heard would be a long stretch with no feedback in the middle of it.

So KOTOR 2 gets its own deliberately small path that touches ONLY constants this port has actually verified against the KOTOR 2 binary:

kControlIdOffset     0x54  (from CSWGuiControl::Load reading "ID") kLabelTextOffset     0xf0  (label text object 0xd8 + text_params 0x18) kButtonTextOffset    0x178 (button text object 0x160 + text_params 0x18)

No panel classification, no engine calls, no first-sight bookkeeping. It reads the focused control's inline caption and speaks it. That is enough to answer the only question that matters right now — does our code run inside KOTOR 2 and see real GUI state — and every piece it uses is independently confirmed, so a wrong result points at exactly one of three numbers.

Deliberately NOT used: ReadLabelText / ReadButtonText. Those go through ReadGuiString for the *rendered* string, which needs kAurGuiStringCStrOffset — the one offset in the text chain still unverified (CAurGUIStringInternal is a class whose vtable slot count did not match). Reading the inline CExoString sidesteps it. On KOTOR 1 the inline field is often empty, which is why the real extractor prefers gui_string — and the first working run answered the open question: KOTOR 2 DOES populate it on buttons ("Neues Spiel" / "Spiel laden" / "Filmsequenzen" all read straight out of it). So the unverified gui_string offset does not block text extraction here, which is a real divergence from KOTOR 1.

The vtable is logged on purpose: it tells us which control classes actually reach focus in KOTOR 2. So far the main menu produces only CSWGuiButton (vtable 00987A1C), matching the RTTI scan exactly.

## Declarations (in source order)

- L89 `bool CallerIsFocusChange(void* caller)`: Did this SetActiveControl originate inside HandleFocusChange?
- L154 `void WarpCursorToControl(void* control)`
- L237 `bool ReadFocusProbe(void* panel, void* control, FocusProbe* p)`: Returns false when any read faulted.
- L294 `void AnnounceFocus(void* panel, void* control, void* caller)`
- L406 `extern "C" void __cdecl OnSetActiveControlK2(void* ebp)`: CSWGuiPanel::SetActiveControl @0x0040EC00, hooked at 0x0040EC09 — after the prologue has stored `this`, so both arguments are already in the frame.
- L452 `extern "C" int __cdecl OnHandleInputEventK2(void* ebp)`: CSWGuiManager::HandleInputEvent @0x00410AA0, hooked at 0x00410AC8 — after the SEH prologue has stored `this` at [EBP-0x68].
- L485 `extern "C" void __cdecl OnHandleFocusChangeK2(void* thisCtrl, void* ebp)`: CSWGuiControl::HandleFocusChange @0x00418FE0, hooked at 0x00418FE6 — before `this` is stored, so the hook passes ECX (this) and EBP (frame) separately.
- L506 `extern "C" void __cdecl OnListBoxSetActiveControlK2(void* listBox, void* ebp)`: CSWGuiListBox::SetActiveControl implementation @0x0041E9A0 (vtable slot 0x0041FEE0 is a forwarder that calls it, so hooking the body catches both routes), hooked at 0x0041E9A4 — before `this` is stored.
