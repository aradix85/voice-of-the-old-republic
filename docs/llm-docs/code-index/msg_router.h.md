# msg_router.h (55 lines)

Source: `patches/Accessibility/msg_router.h`

In-game message-buffer router.

Owns the CGuiInGame::AppendToMsgBuffer @0x0062b5c0 hook and dispatches each engine-emitted line through registered rules. First-match-wins; the first rule returning true claims the line and suppresses the raw- speak fallback. Unclaimed lines fire on-unmatched cleanup hooks then speak raw.

Suppression/merging/priority policy lives entirely in rules registered here — one place to read, one place to change.

AppendToMsgBuffer is main-thread-only. No locking.

## Declarations (in source order)

- L18 `using RuleFn`
- L23 `using OnUnmatchedFn`: Invoked when no rule claims a line, before the raw speech.
- L25 `class Router`
