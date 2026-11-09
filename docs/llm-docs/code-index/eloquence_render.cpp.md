# eloquence_render.cpp (426 lines)

Source: `patches/Accessibility/eloquence_render.cpp`

(From eloquence_render.h.) Eloquence (IBM ECI) render-to-memory — the NVDA IBMTTS add-on's voice, rendered by the mod so it can be played positionally.

We NEVER ship or copy ECI.DLL (no distribution rights). This only loads an engine the user already installed with the IBMTTS add-on, from the add-on's own folder, and only in-process: the add-on's ECI.DLL is 32-bit, like the game. Paths and voice settings come from NVDA's saved config (nvda_config: [ibmeci] TTSPath / dllName, and [speech] [[ibmeci]] voice / variant / rate / rateBoost / pitch / inflection / hsz / rgh / bth / sampleRate), mapped the way the add-on's driver maps them (synthDrivers/ibmeci.py + _ibmeci.py) — except volume: rendered at full, the player sets the loudness.

Rendering runs on one persistent worker thread that keeps its engine handle between lines (a fresh handle per line cost ~110 ms). ECI delivers its audio callbacks as window messages to the thread that created the handle, so the worker pumps its own queue until each line's end mark comes back.

## Declarations (in source order)

- L87 `Settings ReadSettings(const nvda_config::Config& c)`
- L107 `std::string EngineDir(const nvda_config::Config& c)`: Engine folder: [ibmeci] TTSPath, relative to the add-on's synthDrivers folder unless absolute (the driver's setPathsFromConfig).
- L114 `std::string EngineFile(const nvda_config::Config& c)`
- L120 `bool Resolve(HMODULE lib, T& fn, const char* name)`
- L126 `bool EnsureLoaded(const nvda_config::Config& c)`
- L190 `int __stdcall OnEciMessage(void* /*eci*/, int msg, long lParam, void* /*data*/)`
- L203 `void SetVoiceParamIfSet(int param, int value)`
- L210 `void RunJob(Job* job)`: One line on the worker's persistent handle.
- L293 `DWORD WINAPI WorkerThread(void*)`
- L319 `bool EnsureWorker()`
- L335 `Job* Enqueue(const char* text, uint32_t ticket, HANDLE done, int rateOffset)`: Builds a job from NVDA's current saved settings and queues it.
- L356 `bool Collect(Job* job, prism::RenderedSpeech& out)`: Moves a finished job's audio into `out`.
- L382 `bool IsNvdaSynth()`
- L390 `bool Submit(const char* text, uint32_t ticket, int rateOffset)`
- L394 `bool TakeFinished(uint32_t& ticket, prism::RenderedSpeech& out, bool& ok)`
- L407 `bool RenderToMemory(const char* text, prism::RenderedSpeech& out)`
