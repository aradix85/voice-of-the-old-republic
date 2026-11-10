# strings_de.cpp (1239 lines)

Source: `patches/Accessibility/strings_de.cpp`

German string table.

Encoding: Windows-1252 hex escapes for non-ASCII (\xE4=?, \xF6=?, \xFC=?, \xDF=?, \xC4=?, \xD6=?, \xDC=?). The narrow Speak overload converts from CP_ACP via MultiByteToWideChar; on a German Windows install CP_ACP = Windows-1252, so the literal bytes here pass through unchanged. UTF-8 source would also work but only with /utf-8 set, and the patch build script (create-patch.bat) doesn't pass that flag.

Direction frame: German nav uses "auf X Uhr" ("at X o'clock") rather than the English "X o'clock" ? the bare "X Uhr" form would read as time-of-day. Plurals: empty-state phrases use the plural form (T\xFCren, not T\xFCr) since "Keine T\xFCr in Reichweite" sounds odd.

## Declarations (in source order)

- L19 `const char* Get(Id id)`
