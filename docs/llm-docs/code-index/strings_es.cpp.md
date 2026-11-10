# strings_es.cpp (1220 lines)

Source: `patches/Accessibility/strings_es.cpp`

Spanish string table.

Encoding: Windows-1252 hex escapes for non-ASCII (\xE1=?, \xE9=?, \xED=?, \xF3=?, \xFA=?, \xF1=?, \xFC=?, \xC1=?, \xC9=?, \xCD=?, \xD3=?, \xDA=?, \xD1=?, \xBF=?, \xA1=?). The narrow Speak overload converts from CP_ACP via MultiByteToWideChar; on a Spanish Windows install CP_ACP = Windows-1252, so the literal bytes pass through unchanged.

Combat speech is fed by combat_strings.cpp::kEs (engine anchors extracted from dialog_es.tlk); this table covers the Id::* speech path.

## Declarations (in source order)

- L18 `const char* Get(Id id)`
