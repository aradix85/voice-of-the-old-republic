# engine_offsets_fields.h (2533 lines)

Source: `patches/Accessibility/engine_offsets_fields.h`

Engine struct field offsets - the upstream AddressDatabase `offsets` table (keyed class + member; our flat k*Offset names carry the class in the comment above each block, which is the piece the name itself loses).

Part of the engine_offsets.h family - see engine_offsets_types.h for the split rationale and the full file list.

Scope note: two kinds of constant live here that are not literally offsets, because separating them from the field they describe makes both halves unreadable: * struct geometry - element counts, strides and sizeof values (kClassSelCharSize, kClassSelectionsCount, kSkillFlowColumnStride, ...); * field interpretation - the bit masks and sentinels documented with the field they decode (kControlVisibleBit, kSwsItemInfiniteStockBit, kFlowSkillStructEmptyFeatId). Constants that stand alone - vtable slot indices, TLK strrefs, enum bytes, panel input codes - are in engine_offsets_values.h instead.

Per-game values (KOTOR 2 port) ------------------------------ Every declaration here goes through one of the acc::off markers, so the file carries both games' layouts rather than only KOTOR 1's. See engine_offsets_select.h for how to choose between them:

Same(x)      - verified identical in both games Pick(k1, k2) - verified different Todo(k1)     - KOTOR 2 value not yet established; poisons on KOTOR 2

They are `const` rather than `constexpr` because Pick/Todo resolve at load time against the detected game. Nothing here needs to be a compile-time constant - that was checked across the whole codebase before the conversion (no array bounds, no static_assert, no case labels, no template arguments). If you add one that does, it cannot use these markers.

Everything is currently Todo(): the conversion was made deliberately as a pure structural change with the KOTOR 1 values untouched, so that KOTOR 1 behaviour is provably identical and the reverse-engineering can then land one constant at a time. `grep -c "Todo("` is the remaining-work counter.

Also 385 constants or variables at namespace scope.
