# engine_app.cpp (96 lines)

Source: `patches/Accessibility/engine_app.cpp`

AppManager resolve primitives. See engine_app.h for the chain and the reason this file exists.

Each walk gets its own __try rather than sharing one: a fault reading the global is a different situation from a fault reading a facade field, and both have to yield nullptr cleanly to callers that run during engine teardown.

## Declarations (in source order)

- L15 `void* GetAppManager()`
- L23 `void* GetClientApp()`
- L35 `void* GetClientAppInternal()`
- L47 `void* GetClientModule()`
- L59 `void* GetCamera()`
- L71 `void* GetServerApp()`
- L83 `void* GetServerAppInternal()`
