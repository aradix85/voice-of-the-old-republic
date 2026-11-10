# engine_options.cpp (56 lines)

Source: `patches/Accessibility/engine_options.cpp`

(From engine_options.h.) CClientOptions reads: the "Mouse Look" and "Action Menu" auto-pause options.

Chain: *kAddrAppManagerPtr → +0x4 CClientExoApp* → +0x4 Internal* → +0x4 CClientOptions* → +0x8 int bitfield, bit 1 (mask 0x2) = mouse_look.

Bitfield layout at +0x8 (5 bits in one int — preserve siblings on write): bit 0 auto_level bit 1 mouse_look bit 2 autosave bit 3 minigame_yaxis bit 4 combat_movement

User-facing swkotor.ini "Mouse Look=N" matches CClientOptions.mouse_look exclusively. CSWCameraOnAStick.mouseCameraRotateToggle @+0xb0 is a different struct (runtime camera state) — distinct from the user setting.

## Declarations (in source order)

- L13 `void* GetClientOptions()`: Null at any null link or fault.
- L27 `bool GetMouseLook(bool& out)`
- L41 `bool GetActionMenuAutoPause(bool& out)`
