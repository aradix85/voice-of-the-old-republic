# strings_fr.cpp (1246 lines)

Source: `patches/Accessibility/strings_fr.cpp`

French string table.

Encoding: Windows-1252 hex escapes for non-ASCII (\xE9=?, \xE8=?, \xEA=?, \xE0=?, \xE2=?, \xE7=?, \xEE=?, \xEF=?, \xF4=?, \xF9=?, \xFB=?, \xFC=?). Prism's ANSI overload converts from CP_ACP via MultiByteToWideChar; on a French Windows install CP_ACP = Windows-1252, so the literal bytes pass through unchanged.

Combat speech is fed by combat_strings.cpp::kFr (engine anchors extracted from dialog_fr.tlk); this table covers the Id::* speech path.

## Declarations (in source order)

- L17 `const char* Get(Id id)`
