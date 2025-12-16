#include <memory>
#include <Windows.h>

#include "MinHook.h"

#include "SPF/Telemetry/Sdk.hpp"


#include "SPF/Core/Core.hpp"
#include "SPF/Logging/Logger.hpp"

static std::unique_ptr<SPF::Core::Core> g_Core;

/**
 * @brief Telemetry API initialization function.
 *
 * Called by the game when the plugin is loaded.
 */
SCSAPI_RESULT scs_telemetry_init(const scs_u32_t version, const scs_telemetry_init_params_t* const params) {
  if (!g_Core) {
    return SCS_RESULT_generic_error;
  }

  // Call the new telemetry init handler.
  g_Core->OnTelemetryInit(params);

  // Log the successful execution of this entry point.
  g_Core->GetLogger()->Info("scs_telemetry_init completed successfully.");
  return SCS_RESULT_ok;
}

/**
 * @brief Telemetry API deinitialization function.
 *
 * Called by the game when the plugin is unloaded.
 */
SCSAPI_VOID scs_telemetry_shutdown(void) {
  if (g_Core) {
    g_Core->OnTelemetryShutdown();
  }
}

SCSAPI_RESULT scs_input_init(const scs_u32_t version, const scs_input_init_params_t* const params) {
  if (!g_Core) {
    return SCS_RESULT_generic_error;
  }

  // Call the new input init handler.
  g_Core->OnInputInit(params);

  // Log the successful execution of this entry point.
  g_Core->GetLogger()->Info("scs_input_init completed successfully.");
  return SCS_RESULT_ok;
}

SCSAPI_VOID scs_input_shutdown(void) {
  if (g_Core) {
    g_Core->OnInputShutdown();
  }
}

/**
 * @brief DLL entry point.
 *
 * This function is not part of the SCS SDK, but is required for Windows DLLs.
 */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
  switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
      // This is called when the DLL is loaded into the game's process.
      // We initialize MinHook first.
      if (MH_Initialize() != MH_OK) {
        MessageBoxA(nullptr, "Failed to initialize MinHook.", "SPF Critical Error", MB_ICONERROR);
        return FALSE;
      }

      // Now, create our Core instance, passing the module handle.
      g_Core = std::make_unique<SPF::Core::Core>(hModule);
      g_Core->Preload();

      // Logger is now available after Core is constructed and Init is called inside scs_telemetry_init.
      // We can't log here yet, as the logger is initialized in Core::Init.
      break;
    }
    case DLL_PROCESS_DETACH: {
      // This is called when the DLL is unloaded.
      // We ensure our Core is destroyed and uninitialize MinHook.
      g_Core.reset();
      MH_Uninitialize();
      break;
    }
  }

  return TRUE;
}
