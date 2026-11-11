# engine_offsets_values.h (85 lines)

Source: `patches/Accessibility/engine_offsets_values.h`

Engine constants that are neither addresses nor struct field offsets: vtable slot indices, TLK strrefs, object-handle sentinels, enum bytes and panel-internal input codes.

Part of the engine_offsets.h family - see engine_offsets_types.h for the split rationale and the full file list.

None of these are in the upstream AddressDatabase, and that is the point. Strrefs and .gui control IDs are resource-derived: they come from dialog.tlk and the .gui layout files, so they vary with the game's content rather than with its executable - a re-pack of the same build can move them while every address stays put, and vice versa. On a KOTOR 2 port they need re-deriving from K2's own resources, independently of any address or offset work.

Also 21 constants or variables at namespace scope.
