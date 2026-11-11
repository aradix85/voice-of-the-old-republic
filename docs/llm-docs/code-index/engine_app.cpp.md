# engine_app.cpp (49 lines)

Source: `patches/Accessibility/engine_app.cpp`

AppManager resolve primitives. See engine_app.h for the chain and the reason this file exists.

Every link is read guarded on its own: the callers run during engine teardown too, and a null or freed link has to yield nullptr.

## Declarations (in source order)

- L14 `void* GetAppManager()`
- L18 `void* GetClientApp()`
- L23 `void* GetClientAppInternal()`
- L28 `void* GetClientModule()`
- L33 `void* GetCamera()`
- L38 `void* GetServerApp()`
- L43 `void* GetServerAppInternal()`
