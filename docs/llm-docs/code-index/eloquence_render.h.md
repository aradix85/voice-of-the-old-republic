# eloquence_render.h (48 lines)

Source: `patches/Accessibility/eloquence_render.h`

Eloquence (IBM ECI) render-to-memory — the NVDA IBMTTS add-on's voice, rendered by the mod so it can be played positionally.

We NEVER ship or copy ECI.DLL (no distribution rights). This only loads an engine the user already installed with the IBMTTS add-on, from the add-on's own folder, and only in-process: the add-on's ECI.DLL is 32-bit, like the game. Paths and voice settings come from NVDA's saved config (nvda_config: [ibmeci] TTSPath / dllName, and [speech] [[ibmeci]] voice / variant / rate / rateBoost / pitch / inflection / hsz / rgh / bth / sampleRate), mapped the way the add-on's driver maps them (synthDrivers/ibmeci.py + _ibmeci.py) — except volume: rendered at full, the player sets the loudness.

Rendering runs on one persistent worker thread that keeps its engine handle between lines (a fresh handle per line cost ~110 ms). ECI delivers its audio callbacks as window messages to the thread that created the handle, so the worker pumps its own queue until each line's end mark comes back.

## Declarations (in source order)

- L30 `bool IsNvdaSynth()`: True when NVDA's saved synthesizer is IBMTTS ("ibmeci") and the add-on's engine file exists.
- L38 `bool Submit(const char* text, uint32_t ticket, int rateOffset)`: Queues `text` (game codepage; cp1252-compatible for the Western ECI languages) for background rendering with NVDA's current saved Eloquence settings, the rate moved by `rateOffset` NVDA percent points (prism::OffsetRa...
- L42 `bool TakeFinished(uint32_t& ticket, prism::RenderedSpeech& out, bool& ok)`: Main-thread poll: hands back one finished render, if any.
- L45 `bool RenderToMemory(const char* text, prism::RenderedSpeech& out)`: Blocking variant (5 s cap) for diagnostics — probe_memory_sound.
