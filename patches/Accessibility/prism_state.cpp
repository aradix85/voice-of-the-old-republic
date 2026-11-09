// The state the prism files share, defined in one place; each item is
// described where it is declared, in prism_internal.h.

#include "prism_internal.h"

namespace prism {

HMODULE       g_prismLib       = nullptr;
PrismContext* g_prismCtx       = nullptr;
PrismBackend* g_prismNormal    = nullptr;
PrismBackend* g_prismSapi      = nullptr;
bool          g_initTried      = false;
bool          g_normalReady    = false;
bool          g_sapiReady      = false;
size_t g_sapiVoiceCurrent = static_cast<size_t>(-1);
size_t g_sapiVoiceCount   = 0;
char g_sapiVoiceName[160] = {0};
bool g_sapiVoiceLoaded    = false;
int  g_sapiRatePercent = kSapiRateDefaultPercent;
bool g_sapiRateLoaded  = false;
PFN_prism_init                  pPrism_init                   = nullptr;
PFN_prism_shutdown              pPrism_shutdown               = nullptr;
PFN_prism_registry_exists       pPrism_registry_exists        = nullptr;
PFN_prism_registry_count        pPrism_registry_count         = nullptr;
PFN_prism_registry_id_at        pPrism_registry_id_at         = nullptr;
PFN_prism_registry_acquire      pPrism_registry_acquire       = nullptr;
PFN_prism_registry_acquire_best pPrism_registry_acquire_best  = nullptr;
PFN_prism_registry_create       pPrism_registry_create        = nullptr;
PFN_prism_backend_get_features  pPrism_backend_get_features   = nullptr;
PFN_prism_backend_initialize    pPrism_backend_initialize     = nullptr;
PFN_prism_backend_speak         pPrism_backend_speak          = nullptr;
PFN_prism_backend_output        pPrism_backend_output         = nullptr;
PFN_prism_backend_braille       pPrism_backend_braille        = nullptr;
PFN_prism_backend_stop          pPrism_backend_stop           = nullptr;
PFN_prism_backend_is_speaking   pPrism_backend_is_speaking    = nullptr;
PFN_prism_backend_set_rate      pPrism_backend_set_rate       = nullptr;
PFN_prism_backend_set_volume    pPrism_backend_set_volume     = nullptr;
PFN_prism_backend_set_voice     pPrism_backend_set_voice      = nullptr;
PFN_prism_backend_count_voices  pPrism_backend_count_voices   = nullptr;
PFN_prism_backend_get_voice_name pPrism_backend_get_voice_name = nullptr;
PFN_prism_backend_get_voice     pPrism_backend_get_voice      = nullptr;
PFN_prism_backend_name          pPrism_backend_name           = nullptr;
PFN_prism_backend_speak_to_memory pPrism_backend_speak_to_memory = nullptr;
char s_lastSpoken[4096] = {0};
bool s_suppressNextCapture = false;
PrismBackend* g_prismCompass       = nullptr;
bool          g_compassReady       = false;
size_t        g_compassVoiceCurrent = static_cast<size_t>(-1);
RenderBackend g_renderSapi    = {"SAPI",    kPrismBackendIdSapi,    nullptr, false, {}};
RenderBackend g_renderOneCore = {"OneCore", kPrismBackendIdOneCore, nullptr, false, {}};

}  // namespace prism
