# audio_pitch.cpp (87 lines)

Source: `patches/Accessibility/audio_pitch.cpp`

Pitch-variance neutraliser: scoped to our Play*OneShotSound calls, makes every fire of the same resref land at the same frequency. Engine jitter shifts HRTF response (smearing the perceived spatial position) and blurs per-cue timbre identification.

Detour at 0x005db3d0 (function entry): - 5-byte prologue cut: PUSH ECX; PUSH ESI; PUSH EDI; MOV ESI, ECX (skip_original_bytes=false so the wrapper emits it). - exclude_from_restore=["eax"] so the handler's return survives into the wrapper's TEST EAX,EAX dispatch. - consumed_exit_address=0x005db449 (shared POP/RET epilogue): pops the 12 bytes the cut pushed and RETs with EAX = base_frequency. - Handler returns 0 to fall through (flag clear or bad source) → wrapper lands at hookAddress+5 with the prologue already done.

base_frequency=0 with flag set: return 1 instead of 0 so the wrapper's TEST doesn't route to the fall-through path. Defensive only — sample- derived base_frequency is always positive in practice.

## Declarations (in source order)

- L43 `void BeginScopedZero()`
- L47 `void EndScopedZero()`
- L51 `bool IsScopedZeroActive()`
- L57 `extern "C" int __cdecl OnCalculatePitchVarianceFrequency(void* source)`
