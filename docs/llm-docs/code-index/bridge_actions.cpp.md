# bridge_actions.cpp (85 lines)

Source: `patches/Accessibility/bridge_actions.cpp`

Local bridge: /do, the actions an assistant can take in the game.

## Declarations (in source order)

- L21 `std::string Error(const char* what)`
- L24 `bool SayNow(const std::string& utf8)`: False when there is nothing to say.
- L32 `std::string RunObjectAction(const std::string& action, const Params& params)`: The actions on one object, by its server handle.
- L57 `std::string RunAction(const Params& params)`
