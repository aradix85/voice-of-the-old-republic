# diag_chargen_feats.cpp (172 lines)

Source: `patches/Accessibility/diag_chargen_feats.cpp`

See diag_chargen_feats.h for purpose + removal note.

## Declarations (in source order)

- L24 `int FeatNameStrref(unsigned short featId)`: Read a CSWFeat.name_strref for `featId` from the rules table.
- L42 `void DumpUshortListSEH(unsigned char* base, size_t dataOff, size_t sizeOff, const char* tag)`
- L78 `void DumpChartCells(void* fcp)`
- L144 `void DumpStructureIfNeeded(void* panel)`
