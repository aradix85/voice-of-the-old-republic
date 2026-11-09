// The state the prism files share, defined in one place; each item is
// described where it is declared, in prism_internal.h.

#include "prism_internal.h"

namespace prism {

PFN_prism_init                  pPrism_init                   = nullptr;
PFN_prism_registry_count        pPrism_registry_count         = nullptr;
PFN_prism_registry_id_at        pPrism_registry_id_at         = nullptr;
PFN_prism_registry_acquire      pPrism_registry_acquire       = nullptr;
PFN_prism_registry_acquire_best pPrism_registry_acquire_best  = nullptr;
PFN_prism_backend_get_features  pPrism_backend_get_features   = nullptr;
PFN_prism_backend_initialize    pPrism_backend_initialize     = nullptr;
PFN_prism_backend_speak         pPrism_backend_speak          = nullptr;
PFN_prism_backend_output        pPrism_backend_output         = nullptr;
PFN_prism_backend_braille       pPrism_backend_braille        = nullptr;
PFN_prism_backend_stop          pPrism_backend_stop           = nullptr;
PFN_prism_backend_name          pPrism_backend_name           = nullptr;

PrismContext* g_prismCtx    = nullptr;
PrismBackend* g_prismNormal = nullptr;
bool          g_normalReady = false;

}  // namespace prism
