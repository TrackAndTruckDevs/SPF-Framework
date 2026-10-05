#include "SPF/Modules/API/EconomyApi.hpp"

#include "SPF/Data/GameData/EconomyService.hpp"
#include "SPF/SPF_API/SPF_Economy_API.h"
#include "SPF/SPF_API/SPF_Plugin.h"

#include <cstdint>

namespace SPF::Modules::API {

using namespace SPF::Data::GameData;

void EconomyApi::FillEconomyApi(SPF_Economy_API* economy_api) {
  if (!economy_api) return;

  economy_api->ECO_IsReady = &T_ECO_IsReady;
  economy_api->ECO_AreAllOffsetsFound = &T_ECO_AreAllOffsetsFound;
  economy_api->ECO_RefreshOffsets = &T_ECO_RefreshOffsets;

  economy_api->ECO_GetBalance = &T_ECO_GetBalance;
  economy_api->ECO_GetDebtFlag = &T_ECO_GetDebtFlag;
  economy_api->ECO_AddMoney = &T_ECO_AddMoney;
  economy_api->ECO_SetMoney = &T_ECO_SetMoney;

  economy_api->ECO_SetGameControl = &T_ECO_SetGameControl;
  economy_api->ECO_GetGameControlLocked = &T_ECO_GetGameControlLocked;
}

// --- Lifecycle ---

bool EconomyApi::T_ECO_IsReady() { return EconomyService::GetInstance().IsReady(); }

bool EconomyApi::T_ECO_AreAllOffsetsFound() { return EconomyService::GetInstance().AreAllFindersReady(); }

bool EconomyApi::T_ECO_RefreshOffsets() { return EconomyService::GetInstance().TryFindAllOffsets(); }

// --- Balance ---

int64_t EconomyApi::T_ECO_GetBalance() { return EconomyService::GetInstance().GetBalance(); }

bool EconomyApi::T_ECO_GetDebtFlag() { return EconomyService::GetInstance().GetDebtFlag(); }

bool EconomyApi::T_ECO_AddMoney(int64_t delta) { return EconomyService::GetInstance().AddMoney(delta); }

bool EconomyApi::T_ECO_SetMoney(int64_t amount) { return EconomyService::GetInstance().SetMoney(amount); }

// --- Game control lock ---

bool EconomyApi::T_ECO_SetGameControl(bool lock) {
  EconomyService::GetInstance().SetGameControl(lock);
  return EconomyService::GetInstance().IsGameControlLocked() == lock;
}

bool EconomyApi::T_ECO_GetGameControlLocked() { return EconomyService::GetInstance().IsGameControlLocked(); }

}  // namespace SPF::Modules::API
