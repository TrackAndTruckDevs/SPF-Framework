#pragma once

#include "SPF/SPF_API/SPF_Economy_API.h"
#include <cstdint>

namespace SPF::Modules::API {
class EconomyApi {
 public:
  static void FillEconomyApi(SPF_Economy_API* economy_api);

 private:
  static bool T_ECO_IsReady();
  static bool T_ECO_AreAllOffsetsFound();
  static bool T_ECO_RefreshOffsets();

  static int64_t T_ECO_GetBalance();
  static bool T_ECO_GetDebtFlag();
  static bool T_ECO_AddMoney(int64_t delta);
  static bool T_ECO_SetMoney(int64_t amount);

  static bool T_ECO_SetGameControl(bool lock);
  static bool T_ECO_GetGameControlLocked();
};

}  // namespace SPF::Modules::API
