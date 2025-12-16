#pragma once

#include "SPF/SPF_API/SPF_Telemetry_API.h"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Modules::API {
class TelemetryApi {
 public:
  static void FillTelemetryApi(SPF_Telemetry_API* api);

 private:
  static SPF_Telemetry_Handle* T_GetContext(const char* pluginName);
  static void T_GetGameState(SPF_Telemetry_Handle* handle, SPF_GameState* out_data);
  static void T_GetTimestamps(SPF_Telemetry_Handle* handle, SPF_Timestamps* out_data);
  static void T_GetCommonData(SPF_Telemetry_Handle* handle, SPF_CommonData* out_data);
  static void T_GetTruckConstants(SPF_Telemetry_Handle* handle, SPF_TruckConstants* out_data);
  static void T_GetTruckData(SPF_Telemetry_Handle* handle, SPF_TruckData* out_data);
  static void T_GetTrailers(SPF_Telemetry_Handle* handle, SPF_Trailer* out_trailers, uint32_t* in_out_count);
  static void T_GetJobConstants(SPF_Telemetry_Handle* handle, SPF_JobConstants* out_data);
  static void T_GetJobData(SPF_Telemetry_Handle* handle, SPF_JobData* out_data);
  static void T_GetNavigationData(SPF_Telemetry_Handle* handle, SPF_NavigationData* out_data);
  static void T_GetControls(SPF_Telemetry_Handle* handle, SPF_Controls* out_data);
  static void T_GetSpecialEvents(SPF_Telemetry_Handle* handle, SPF_SpecialEvents* out_data);
  static void T_GetGameplayEvents(SPF_Telemetry_Handle* handle, SPF_GameplayEvents* out_data);
  static void T_GetGearboxConstants(SPF_Telemetry_Handle* handle, SPF_GearboxConstants* out_data);
  static int T_GetLastGameplayEventId(SPF_Telemetry_Handle* handle, char* out_buffer, int buffer_size);
};
}  // namespace Modules::API
SPF_NS_END
