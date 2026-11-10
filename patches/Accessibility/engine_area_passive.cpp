// Keeps Q/E targeting and passive narration live while the engine holds a
// finished, obscuring fade (the Endar Spire opening).

#include "engine_area_internal.h"

#include "engine_app.h"     // GetClientApp
#include "engine_offsets_select.h"
#include "engine_rebase.h"
#include "log.h"

namespace acc::engine {

// The Endar Spire opening HOLDS the global fade panel at alpha=1.0 for a long
// scripted stretch *after* the fade animation finishes (IsFadeDone), while the
// player still has full world control. The engine re-asserts that alpha every
// frame (an alpha=0 write from our tick is reverted before MainLoop's gate
// reads it), so clearing the fade can never win. MainLoop gates
// DoPassiveSelection behind !IsGlobalFadeObscuring (alpha > 0.001), which
// freezes the Q/E candidate halo AND passive hover-narration. For a sighted
// player a black screen means "nothing to target"; a blind player is actively
// navigating that (to them irrelevant) black screen and needs targeting and
// narration live.
//
// So rather than fight the fade, DRIVE the selection ourselves: when the ONLY
// thing MainLoop is waiting on is the held obscuring fade (input is normal
// world, area is displayed, the fade is finished — not mid-animation), call
// DoPassiveSelection directly. It rebuilds the halo from GetNearestObjects and
// refreshes passive narration — exactly what the engine would do if the fade
// weren't pinning the gate shut. We only act while MainLoop itself is skipping
// it (obscuring), so we never double-drive, and we leave the visible fade
// untouched (it's the engine's to own). Returns true when it drove a pass.
// SEH-guarded. Offsets: gui_in_game +0x40, fade +0x6c, panel.alpha +0x4c,
// input_class +0x9c, area_not_ready +0x288.
bool MaybeDrivePassiveSelection() {
    // K2: the client MainLoop twin FUN_00781be0
    // (its gate at +0x500-line reads `([internal+0x9c]==0 || ==4) &&
    // [internal+0x288]==0` — byte-for-byte K1's gate), and in the K2
    // IsGlobalFadeObscuring twin 0x007CBC70, which reads
    // `0.001 < [[gui+0x6c] + 0x50]` — fade slot unchanged, alpha +0x4c->+0x50.
    const size_t kClientInternalOffset = acc::off::Same(0x4);    // CClientExoApp -> internal
    const size_t kGuiInGameOffset      = acc::off::Same(0x40);   // internal.gui_in_game
    const size_t kFadeOffset           = acc::off::Same(0x6c);   // CGuiInGame.fade
    const size_t kFadeAlphaOffset      = acc::off::Pick(0x4c, 0x50);  // CSWGuiFade.panel.alpha
    const size_t kInputClassOffset     = acc::off::Same(0x9c);   // internal.input_class
    const size_t kAreaNotReadyOffset   = acc::off::Same(0x288);  // internal.area_not_ready
    // The two address lookups below run every tick: a binary search over a
    // small sorted table, below the noise floor. Caching them does not pay:
    // a function-local `static` needs a thread-safe init guard, which trips
    // C2712 in this SEH function, and a namespace-scope one would depend on
    // static-init order against CurrentBuild().
    // K2 twins at the K2 MainLoop gate (FUN_00781be0):
    // `if (!FUN_007cbc40()) if (!FUN_007cbc70()) FUN_0079a4c0(dt);` — the
    // same IsGlobalFading -> IsGlobalFadeObscuring -> DoPassiveSelection
    // sequence as K1's 0x00603b44/0x00603b50/0x00603b61.
    const uintptr_t kAddrIsGlobalFading =
        acc::addr::Pick(0x0062ac60, 0x007cbc40);  // __thiscall(gui)->int
    const uintptr_t kAddrDoPassiveSelection =
        acc::addr::Pick(0x005fa5a0, 0x0079a4c0);  // __thiscall(internal,float)
    using PFN_Fade      = int(__thiscall*)(void*);
    using PFN_DoPassive = void(__thiscall*)(void*, float);

    static bool s_driving = false;  // rising-edge log latch

    void* clientApp = GetClientApp();
    if (!clientApp) { s_driving = false; return false; }

    __try {
        void* internal = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(clientApp) + kClientInternalOffset);
        if (!internal) { s_driving = false; return false; }
        unsigned char* base = reinterpret_cast<unsigned char*>(internal);

        void* guiInGame = *reinterpret_cast<void**>(base + kGuiInGameOffset);
        void* fade = guiInGame ? *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(guiInGame) + kFadeOffset) : nullptr;
        if (!fade) { s_driving = false; return false; }

        float alpha = *reinterpret_cast<float*>(
            reinterpret_cast<unsigned char*>(fade) + kFadeAlphaOffset);
        uint32_t inputClass =
            *reinterpret_cast<uint32_t*>(base + kInputClassOffset);
        uint32_t areaNotReady =
            *reinterpret_cast<uint32_t*>(base + kAreaNotReadyOffset);

        // Drive only when MainLoop is skipping DoPassiveSelection *purely*
        // because of the held obscuring fade: obscuring (alpha > 0.001), normal
        // world input, area displayed, and the fade FINISHED (not an active
        // transition — leave those brief windows alone).
        bool obscuring = alpha > 0.001f;
        bool worldReady = (inputClass == 0 || inputClass == 4) &&
                          areaNotReady == 0;
        if (!obscuring || !worldReady) { s_driving = false; return false; }

        int fading = reinterpret_cast<PFN_Fade>(kAddrIsGlobalFading)(guiInGame);
        if (fading != 0) { s_driving = false; return false; }

        reinterpret_cast<PFN_DoPassive>(kAddrDoPassiveSelection)(internal, 0.016f);

        if (!s_driving) {
            s_driving = true;
            acclog::Write("FadeUnstick",
                "obscuring fade held (alpha=%.2f) with world input — driving "
                "DoPassiveSelection directly to keep Q/E halo + narration live",
                alpha);
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        s_driving = false;
        return false;
    }
}

}  // namespace acc::engine
