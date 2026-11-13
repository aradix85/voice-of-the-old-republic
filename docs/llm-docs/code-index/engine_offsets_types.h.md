# engine_offsets_types.h (116 lines)

Source: `patches/Accessibility/engine_offsets_types.h`

Engine C++ type definitions shared across the patch.

The engine_offsets.h family is split by what decides portability between the two games:

engine_offsets_types.h      This file. Plain C++ struct layouts for engine types we read through or construct, and the distances between positions and the angle units. No addresses, no offsets. engine_offsets_addresses.h  Executable addresses: functions, vtables and .data global pointers, through acc::addr. engine_offsets_fields.h     Struct field offsets. engine_offsets_values.h     Constants that are neither: vtable slot indices, TLK strrefs, sentinels, enum bytes, panel input codes.

engine_offsets.h includes all four.

On KOTOR 2 each group moves on its own: every address differs, struct offsets mostly differ, and resource-derived values follow dialog.tlk and the .gui files, not the executable.

## Declarations (in source order)

- L30 `struct CExoArrayList`: CExoArrayList — engine container for arrays of pointers.
- L42 `struct Vector`: Vector — Aurora-engine 3D vector.
- L50 `inline float DistXYSq(const Vector& a, const Vector& b)`: Distances in metres.
- L56 `inline float DistXY(const Vector& a, const Vector& b)`
- L61 `inline float DistXYToSegmentSq(const Vector& p, const Vector& a, const Vector& b)`: From p to the nearest point of the segment a-b, on the floor plan.
- L76 `inline bool StandsBetweenXY(const Vector& p, const Vector& a, const Vector& b, float maxSideM)`: True when p stands in the way from a to b on the floor plan: beside the segment by at most maxSideM, and between its ends (not at or behind a, not at or past b).
- L87 `inline float DistXYZSq(const Vector& a, const Vector& b)`
- L93 `inline int WholeMetres(float metres)`: A distance as the whole metres the player is told.
- L98 `inline int WholeMetresAtLeastOne(float metres)`: The same for something that is not where the player stands: never 0.
- L112 `struct CExoString`: CExoString - the engine's string type: heap c_string plus explicit length.

Also 3 constants or variables at namespace scope.
