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

  economy_api->ECO_GetCreditLimit = &T_ECO_GetCreditLimit;
  economy_api->ECO_GetLoanLimit = &T_ECO_GetLoanLimit;
  economy_api->ECO_SetCreditLimit = &T_ECO_SetCreditLimit;

  economy_api->ECO_GetActiveLoanCount = &T_ECO_GetActiveLoanCount;
  economy_api->ECO_GetActiveLoan = &T_ECO_GetActiveLoan;

  economy_api->ECO_GetOfferCount = &T_ECO_GetOfferCount;
  economy_api->ECO_GetOffer = &T_ECO_GetOffer;

  economy_api->ECO_CanRepayLoan = &T_ECO_CanRepayLoan;
  economy_api->ECO_RepayLoan = &T_ECO_RepayLoan;
  economy_api->ECO_TakeLoan = &T_ECO_TakeLoan;
  economy_api->ECO_TakeOffer = &T_ECO_TakeOffer;

  economy_api->ECO_SetLoanRemaining = &T_ECO_SetLoanRemaining;
  economy_api->ECO_SetLoanAmount = &T_ECO_SetLoanAmount;
  economy_api->ECO_SetLoanRate = &T_ECO_SetLoanRate;
  economy_api->ECO_SetLoanWeeks = &T_ECO_SetLoanWeeks;
  economy_api->ECO_SetLoanStartDay = &T_ECO_SetLoanStartDay;
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

// --- Credit limit ---

int64_t EconomyApi::T_ECO_GetCreditLimit() { return EconomyService::GetInstance().GetCreditLimit(); }

int64_t EconomyApi::T_ECO_GetLoanLimit() { return EconomyService::GetInstance().GetLoanLimit(); }

bool EconomyApi::T_ECO_SetCreditLimit(int64_t limit) { return EconomyService::GetInstance().SetCreditLimit(limit); }

// --- Active loans ---

int32_t EconomyApi::T_ECO_GetActiveLoanCount() { return EconomyService::GetInstance().GetActiveLoanCount(); }

bool EconomyApi::T_ECO_GetActiveLoan(int32_t idx, SPF_ECO_ActiveLoanInfo* out) {
  if (!out) return false;
  EconomyService::ActiveLoanInfo info;
  if (!EconomyService::GetInstance().GetActiveLoan(idx, info)) return false;
  out->remaining = info.remaining;
  out->amount = info.amount;
  out->startDay = info.startDay;
  out->rate = info.rate;
  out->weeks = info.weeks;
  return true;
}

// --- Loan offers ---

int32_t EconomyApi::T_ECO_GetOfferCount() { return EconomyService::GetInstance().GetOfferCount(); }

bool EconomyApi::T_ECO_GetOffer(int32_t idx, SPF_ECO_LoanOfferInfo* out) {
  if (!out) return false;
  EconomyService::LoanOfferInfo info;
  if (!EconomyService::GetInstance().GetOffer(idx, info)) return false;
  out->amount = info.amount;
  out->rate = info.rate;
  out->weeks = info.weeks;
  return true;
}

// --- Loan take/repay ---

bool EconomyApi::T_ECO_CanRepayLoan(int32_t idx) { return EconomyService::GetInstance().CanRepayLoan(idx); }

bool EconomyApi::T_ECO_RepayLoan(int32_t idx) { return EconomyService::GetInstance().RepayLoan(idx); }

bool EconomyApi::T_ECO_TakeLoan(uint64_t amount, float rate, uint32_t weeks) {
  return EconomyService::GetInstance().TakeLoan(amount, rate, weeks);
}

bool EconomyApi::T_ECO_TakeOffer(int32_t idx) { return EconomyService::GetInstance().TakeOffer(idx); }

// --- Loan field editing ---

bool EconomyApi::T_ECO_SetLoanRemaining(int32_t idx, int64_t value) {
  return EconomyService::GetInstance().SetLoanRemaining(idx, value);
}

bool EconomyApi::T_ECO_SetLoanAmount(int32_t idx, int64_t value) {
  return EconomyService::GetInstance().SetLoanAmount(idx, value);
}

bool EconomyApi::T_ECO_SetLoanRate(int32_t idx, float value) { return EconomyService::GetInstance().SetLoanRate(idx, value); }

bool EconomyApi::T_ECO_SetLoanWeeks(int32_t idx, uint32_t value) {
  return EconomyService::GetInstance().SetLoanWeeks(idx, value);
}

bool EconomyApi::T_ECO_SetLoanStartDay(int32_t idx, uint32_t value) {
  return EconomyService::GetInstance().SetLoanStartDay(idx, value);
}

}  // namespace SPF::Modules::API
