# strings_it.cpp (1245 lines)

Source: `patches/Accessibility/strings_it.cpp`

Italian string table.

Encoding: Windows-1252 hex escapes for non-ASCII (\xE0=?, \xE8=?, \xE9=?, \xEC=?, \xF2=?, \xF9=?). Prism's ANSI overload converts from CP_ACP via MultiByteToWideChar; on an Italian Windows install CP_ACP = Windows-1252, so the literal bytes pass through unchanged.

Combat speech is fed by combat_strings.cpp::kIt (engine anchors extracted from dialog_it.tlk); this table covers the Id::* speech path.

## Declarations (in source order)

- L16 `const char* Get(Id id)`
