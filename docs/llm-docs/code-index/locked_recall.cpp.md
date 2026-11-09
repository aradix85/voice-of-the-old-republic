# locked_recall.cpp (152 lines)

Source: `patches/Accessibility/locked_recall.cpp`

(From locked_recall.h.) Locked-object story-bark recall.

Story-locked doors and containers explain themselves only once. The engine's generic "This object is locked" feedback (strref 1437) fires on every failed open, but the flavour bark that says WHY ("Security lock. Access code required." — an ownerless BarkBubble raised by the object's fail-to-open script) plays a single time and is then guarded off game-side. A blind player who misses that first bark never hears the reason again; the door just keeps repeating the unhelpful generic line.

This module caches each locked object's story bark the first time it fires, keyed by the object's (stable) narrated-target handle, and replays it on subsequent locked interactions with the same object.

Wiring: * RegisterMsgRule() — adds the strref-1437 rule to the feedback router (msg_router). The rule never consumes the line (the generic "locked" still speaks); it notes the interacted object and, on a repeat attempt, queues the cached explanation for replay. * MaybeCapture()    — called from the BarkBubble handler for every fresh bark; stores an ownerless bark as the explanation for the most-recently locked object. * Tick()            — flushes a queued replay (one line, after the generic "locked" line the router already spoke this frame).

## Declarations (in source order)

- L53 `bool IsLockedMessage(const char* text)`
- L66 `const char* Lookup(uint32_t handle)`
- L73 `void Store(uint32_t handle, const char* bark)`
- L96 `bool RuleLocked(const char* text)`: Feedback-router rule.
- L125 `void RegisterMsgRule()`
- L129 `bool IsGenericLockedMessage(const char* text)`
- L133 `void MaybeCapture(const char* barkText, bool ownerless)`
- L144 `void Tick()`
