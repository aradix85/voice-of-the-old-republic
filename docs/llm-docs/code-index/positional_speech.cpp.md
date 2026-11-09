# positional_speech.cpp (831 lines)

Source: `patches/Accessibility/positional_speech.cpp`

(From positional_speech.h.) Positional (3D) speech for combat announcements — both games.

A line is rendered to PCM by the mod and played as an in-memory WAV through the engine's own 3D sound path (LoopSource::StartFromMemory; mechanism in docs/llm-docs/in-memory-sound-resource.md), FROM the direction of the creature it is about. So it gets the engine's panning, volume sliders and reverb, like any cue.

Placement (2026-10-28, the swoop beacons' method): the engine's own distance falloff is switched off and the source sits at a fixed radius from the listener, in the subject's direction, so the engine supplies only the pan. Loudness is the mod's own curve over the real distance: full up close, down to a floor that stays clearly audible. Over the last metres before that floor the direction eases to straight ahead, so lines from far away end up centred without a jump in level or position.

Placement rule (decided 2026-10-27): a line sounds from WHERE ITS EFFECT LANDS — a party attack from its target (the engine already plays swing and hit sounds at the attacker), an incoming hit from the hit party member, a status / save / kill / absorb from the affected creature, a cast with no target from the caster. The CALLER picks the subject; this module only plays the line there.

Voices: Normal — imitates the screen reader's voice where the mod can reproduce it: NVDA speaking through IBMTTS (Eloquence, an already-installed engine only — never shipped), a SAPI voice or a OneCore voice, with NVDA's saved rate. Anything else (JAWS, NVDA's eSpeak, ...) falls back to the urgent voice. Urgent — the announcement (SAPI) voice the user picked in Mod settings. The screen reader itself can never be positional: it plays its own audio and gives the mod no way to get at it.

Each channel cancels the way its centred counterpart does. Normal queues, one line at a time in order; a key press empties it, as the screen reader drops its queue (Tick watches the keyboard), and so does a controller press (pad_input's NoteNavPress). Urgent: a new line replaces the one playing, as SpeakUrgent interrupts. The two channels may overlap, as the screen reader and the SAPI urgent voice do.

Volume and speed are the mod's own per channel (Mod settings → Combat announcements), NOT the screen reader's: its volume is tuned for centred menu speech at system level, while these lines play inside the game's mix after distance. Speed is an offset from the voice's own rate, so it still follows a change made in NVDA.

NEVER drops a combat line: when the feature is off, when the subject cannot be found, a render fails, or the engine gives the source no 3D voice (no free voice), the line is spoken the usual centred way instead.

3D exit descriptions (2026-10-28) ride a third channel, Exits: a walking region label ("Kreuzung: Nord, Tür Süd-West nach X") is split at its exit entries and each exit sounds from where it is, the rest straight ahead. Unlike combat lines, an exit list goes stale: a new label REPLACES the one still playing, and combat or a blocking menu cuts it off. It uses the announcement voice and the urgent sliders. Far exits keep their direction at every distance (the guidance beacon's soft knee); see docs/llm-docs/positional-speech.md "3D exit descriptions".

## Declarations (in source order)

- L118 `int Clamp(int v, int lo, int hi)`
- L121 `ChannelSettings& SettingsOf(Channel c)`: Exits shares Urgent's record: same voice, same sliders.
- L140 `const char* KindName(VoiceKind k)`
- L155 `std::string TokenDisplayName(const char* tokenPath)`: NVDA stores a SAPI / OneCore voice as its registry token path ("HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Speech_OneCore\Voices\Tokens\...").
- L175 `VoiceChoice ChooseNormalVoice()`: What voice the normal channel imitates right now.
- L198 `const VoiceChoice& CachedNormalVoice()`
- L218 `void TrimName(char* s)`: ---- Subject lookup ------------------------------------------------------ Trim trailing spaces / dots, as the combat log parser and party cache do.
- L226 `bool IsCombatSubjectKind(int kind)`: Anything a combat line can be about: creatures, and the doors and placeables (security doors, containers, terminals) the party can attack.
- L236 `bool HasName(void* obj, uint32_t handle, const char* name)`: Does `obj` go by `name` in the combat log?
- L252 `uint32_t FindSubject(const char* name, const Vector& ref, Vector& outPos)`: The object named `name` closest to `ref`.
- L306 `ChannelState& StateOf(Channel c)`
- L308 `const char* ChannelName(Channel c)`
- L318 `void SpeakCentred(const Line& line, const char* why)`: The centred fallback — the path these lines took before this feature.
- L328 `bool RenderNow(Channel channel, const VoiceChoice& voice, const char* text, prism::RenderedSpeech& out)`: Synchronous render for every voice but Eloquence (SAPI / OneCore take ~16 ms).
- L345 `float DistanceGain(float d)`: Our loudness curve over the real distance (see Tuning).
- L352 `float ExitDistanceGain(float d)`: The exit lines' curve, the beacon's soft knee (see Tuning).
- L370 `bool Place(const Vector* subject, bool exitProfile, Vector& outPos, float& outGain, float& outDist, float& outRear)`: Where a line about `subject` (null = straight ahead, the preview) is played from, and its distance gain.
- L408 `bool Play(ChannelState& ch, Line& line)`: Starts `line` on the channel's source.
- L460 `void TickChannel(ChannelState& ch)`
- L499 `void CollectEloquence()`
- L522 `void Enqueue(Line&& line, const char* subjectName)`: Renders `line` in its channel's voice (Eloquence in the background), queues it and starts it at once when the channel is idle.
- L553 `bool IsWordByte(unsigned char c)`: A byte that continues a word: a match ending or starting next to one is inside a longer word ("Nord" in "Nord-Süd").
- L560 `void AddAhead(const std::string& text, size_t from, size_t to, std::vector<Piece>& out)`: Adds the text between two exits as a straight-ahead piece, minus the list separators around it; nothing when only separators are left.
- L574 `void SplitAtExits(const std::string& text, const std::vector<acc::room_topology::ExitMark>& exits, std::vector<Piece>& out)`: Splits `text` at the exits.
- L643 `bool IsCancelKey(int vk)`
- L655 `void CutNormalOnKeyPress()`
- L676 `void Cut(Channel channel)`
- L683 `bool IsEnabled(Channel channel)`
- L696 `bool SpeakExitList(const char* text, const std::vector<acc::room_topology::ExitMark>& exits)`
- L740 `bool IsSpeaking(Channel channel)`
- L746 `unsigned long LastExitLeadMs()`
- L750 `bool Speak(Channel channel, const char* text, const char* subjectIn, const char* nearIn)`
- L789 `void Preview(Channel channel, const char* text)`
- L801 `int GetVolumePercent(Channel channel)`
- L803 `void SetVolumePercent(Channel channel, int percent)`
- L809 `int GetRateOffset(Channel channel)`
- L811 `void SetRateOffset(Channel channel, int offset)`
- L817 `void Tick()`
