# msg_router.cpp (221 lines)

Source: `patches/Accessibility/msg_router.cpp`

(From msg_router.h.) In-game message-buffer router.

Owns the CGuiInGame::AppendToMsgBuffer @0x0062b5c0 hook and dispatches each engine-emitted line through registered rules. First-match-wins; the first rule returning true claims the line and suppresses the raw- speak fallback. Unclaimed lines fire on-unmatched cleanup hooks then speak raw.

Suppression/merging/priority policy lives entirely in rules registered here — one place to read, one place to change.

AppendToMsgBuffer is main-thread-only. No locking.

## Declarations (in source order)

- L17 `Router& Router::Instance()`
- L22 `void Router::AddRule(const char* name, RuleFn fn)`
- L33 `void Router::AddOnUnmatched(OnUnmatchedFn fn)`
- L41 `void Router::SetLogTag(const char* tag)`
- L45 `void Router::Dispatch(const char* text)`
- L56 `void Router::Speak(const char* text)`
- L63 `void Router::LogRaw(const char* text)`
- L67 `void Router::LogEmit(const char* subTag, const char* text)`
- L92 `extern "C" void __cdecl OnAppendToMsgBuffer(void* /*guiInGame*/, void* esp_param1_addr)`
- L187 `void EnsureRulesRegistered()`
- L216 `extern "C" void __cdecl OnAppendToMsgBufferK2(void* thisGui, void* ebp)`: KOTOR 2 frame-unpacking wrapper (Batch 2).
