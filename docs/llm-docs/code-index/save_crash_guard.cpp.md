# save_crash_guard.cpp (173 lines)

Source: `patches/Accessibility/save_crash_guard.cpp`

(From save_crash_guard.h.) Save-game crash guard (engine ImageScale divide-by-zero).

CServerExoAppInternal::SaveGame -> CAppManager::DoSaveGameScreenShot -> AurSaveGameSnapshot -> ImageScale. AurSaveGameSnapshot captures the framebuffer via glReadPixels and hands ImageScale the source dimensions. With "Frame Buffer Effects" disabled (swkotor.ini Frame Buffer=0) — the documented KOTOR save-crash trigger — that capture comes back 0-sized, and ImageScale divides by (srcW*srcH) without guarding it (it only guards the destination area), so saving #DE's (0xc0000094 INTEGER_DIVIDE_BY_ZERO).

We install an inline trampoline detour over ImageScale: when the source area is degenerate we return a zeroed destination buffer (the save just gets a blank thumbnail — irrelevant for our audience) instead of dividing; otherwise the original runs untouched. See memory project_save_crash_imagescale_framebuffer.

## Declarations (in source order)

- L47 `void* __cdecl ImageScaleGuard(unsigned char* src, int srcW, int srcH, int bpp, int dstW, int dstH)`: Our replacement.
- L81 `void WriteRel32Jmp(uint8_t* at, uintptr_t target)`
- L90 `void InstallSaveScreenshotGuard()`
