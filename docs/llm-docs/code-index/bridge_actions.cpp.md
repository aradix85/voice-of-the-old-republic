# bridge_actions.cpp (82 lines)

Source: `patches/Accessibility/bridge_actions.cpp`

Local bridge: /do, the actions an assistant can take in the game.

## Declarations (in source order)

- L20 `std::string Error(const char* what)`
- L23 `bool SayNow(const std::string& utf8)`: False when there is nothing to say.
- L31 `std::string RunObjectAction(const std::string& action, const Params& params)`: The actions on one object, by its server handle.
- L56 `std::string RunAction(const Params& params)`
