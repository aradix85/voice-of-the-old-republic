// AppManager resolve primitives. See engine_app.h for the chain and the
// reason this file exists.
//
// Every link is read guarded on its own: the callers run during engine
// teardown too, and a null or freed link has to yield nullptr.

#include "engine_app.h"
#include "engine_reads.h"

#include <windows.h>

namespace acc::engine {

void* GetAppManager() {
    return ReadPtrOrNull(reinterpret_cast<void*>(kAddrAppManagerPtr), 0);
}

void* GetClientApp() {
    void* appManager = GetAppManager();
    return ReadPtrOrNull(appManager, kAppManagerClientAppOffset);
}

void* GetClientAppInternal() {
    void* clientApp = GetClientApp();
    return ReadPtrOrNull(clientApp, kClientExoAppInternalOffset);
}

void* GetClientModule() {
    void* clientInternal = GetClientAppInternal();
    return ReadPtrOrNull(clientInternal, kClientInternalModuleOffset);
}

void* GetCamera() {
    void* module = GetClientModule();
    return ReadPtrOrNull(module, kCSWCModuleCameraOffset);
}

void* GetServerApp() {
    void* appManager = GetAppManager();
    return ReadPtrOrNull(appManager, kAppManagerServerAppOffset);
}

void* GetServerAppInternal() {
    void* serverApp = GetServerApp();
    return ReadPtrOrNull(serverApp, kServerExoAppInternalOffset);
}

}  // namespace acc::engine
