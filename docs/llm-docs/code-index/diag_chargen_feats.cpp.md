# diag_chargen_feats.cpp (179 lines)

Source: `patches/Accessibility/diag_chargen_feats.cpp`

See diag_chargen_feats.h for purpose + removal note.

## Declarations (in source order)

- L23 `int FeatNameStrref(unsigned short featId)`: Read a CSWFeat.name_strref for `featId` from the rules table.
- L41 `void DumpUshortListSEH(unsigned char* base, size_t dataOff, size_t sizeOff, const char* tag)`
- L77 `void DumpChartCells(void* fcp)`
- L143 `void DumpStructureIfNeeded(void* panel)`
