# pad_diag.cpp (511 lines)

Source: `patches/Accessibility/pad_diag.cpp`

Controller device census — see pad_diag.h for why this exists at all.

## Declarations (in source order)

- L69 `XiProcs Resolve(HMODULE xinput)`
- L84 `bool TryCapsEx(PFN_XiGetCapsEx fn, unsigned slot, XiCapsEx* out)`: Its own function because of the SEH: an ordinal export with no documented signature is exactly the call that must not be able to take the game down, and MSVC forbids __try in a function holding anything that needs unw...
- L92 `const char* SubTypeName(BYTE sub)`
- L164 `const char* DevTypeName(DWORD devType)`
- L187 `bool HidIdsFromProductGuid(const GUID& g, unsigned* vid, unsigned* pid)`: HID devices carry their USB ids in guidProduct: the low half of Data1 is the vendor, the high half the product, and the tail is the fixed "PIDVID" GUID that marks the encoding.
- L206 `void WideToUtf8(const wchar_t* in, char* out, int outCap)`: The log is UTF-8 — every other line in it is — so a device name has to be converted, not poured in as whatever bytes the platform hands over.
- L215 `BOOL CALLBACK OnDiDevice(const DiDeviceInstanceW* di, void* ref)`
- L242 `int LogDirectInputControllers()`: Returns the controller count, or -1 when the enumeration could not be run at all — which is NOT the same answer as zero and must never be read as one.
- L299 `int LogXInputSlots(const XiProcs& p, int* valveSlots)`: The census itself `valveSlots` counts the occupied slots whose vendor id is Valve's, which LogReading needs and nothing else here does.
- L357 `void LogReading(int xiCount, int diCount, int valveSlots)`: What the two counts together MEAN, in the words the reader of a tester's log needs.
- L419 `unsigned long long Fingerprint(const XiProcs& p)`
- L447 `int SlotSubType(HMODULE xinput, unsigned slot)`
- L457 `bool SlotVendorIds(HMODULE xinput, unsigned slot, unsigned* vid, unsigned* pid)`
- L470 `void LogDeviceCensus(HMODULE xinput, const wchar_t* xinputName, const char* why)`
- L503 `void LogDeviceCensusIfChanged(HMODULE xinput, const wchar_t* xinputName, const char* why)`
