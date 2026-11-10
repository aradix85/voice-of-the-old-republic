# cycle_input_announce.cpp (363 lines)

Source: `patches/Accessibility/cycle_input_announce.cpp`

Stepping through the cycle and announcing the focused entry: its name, payload and position, and the category when it changes.

## Declarations (in source order)

- L25 `void ResolvePinNoteText(void* pin, char* outBuf, size_t bufSize)`: Resolve a map pin's spoken note text exactly as AnnounceCurrent does, so the same-name comparison used for numbering keys off the string the user actually hears (note_text, or the "no text" fallback).
- L38 `void ResolveEntryName(const acc::cycle::CategoryListing& listing, int idx, bool mapHint, char* out, size_t size)`: Resolve listing entry `idx`'s spoken label, mirroring AnnounceCurrent's focused-entry cascade so the same-name comparison used for numbering keys off exactly the string the user hears.
- L81 `void AppendPositionOrdinal(const acc::cycle::CategoryListing& listing, int focusedIndex, bool mapHint, char* name, size_t nameSize)`: Append a north-to-south positional ordinal to a LISTING entry's name when two or more entries in the listing share the same spoken label.
- L123 `void AnnounceCurrent(const acc::cycle::CategoryListing& listing, const char* categoryPrefix, acc::filter::CycleContext ctx)`: Speak the locked Pillar 4 payload for whatever the cycle state currently focuses.
- L307 `int FormatItemPayload(const char* name, bool haveYaw, int clock, int metres, char* outBuf, size_t outBufSize)`: Format the "{name}, {clock} o'clock, {metres} metres" payload (or the no-clock variant) into outBuf.
- L322 `void OnCycleItem(bool prev, acc::filter::CycleContext ctx)`: Step within the current category and announce the new focused item.
- L333 `void OnCycleEdge(bool last, acc::filter::CycleContext ctx)`: Jump to the first (closest) or last (farthest) item of the current category and announce it.
- L345 `void OnCycleCategory(bool prev, acc::filter::CycleContext ctx)`: Step to the next/prev non-empty category and announce "{Category}.
