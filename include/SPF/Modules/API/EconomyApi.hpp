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

  static int64_t T_ECO_GetCreditLimit();
  static int64_t T_ECO_GetLoanLimit();
  static bool T_ECO_SetCreditLimit(int64_t limit);

  static int32_t T_ECO_GetActiveLoanCount();
  static bool T_ECO_GetActiveLoan(int32_t idx, SPF_ECO_ActiveLoanInfo* out);

  static int32_t T_ECO_GetOfferCount();
  static bool T_ECO_GetOffer(int32_t idx, SPF_ECO_LoanOfferInfo* out);

  static bool T_ECO_CanRepayLoan(int32_t idx);
  static bool T_ECO_RepayLoan(int32_t idx);
  static bool T_ECO_TakeLoan(uint64_t amount, float rate, uint32_t weeks);
  static bool T_ECO_TakeOffer(int32_t idx);

  static bool T_ECO_SetLoanRemaining(int32_t idx, int64_t value);
  static bool T_ECO_SetLoanAmount(int32_t idx, int64_t value);
  static bool T_ECO_SetLoanRate(int32_t idx, float value);
  static bool T_ECO_SetLoanWeeks(int32_t idx, uint32_t value);
  static bool T_ECO_SetLoanStartDay(int32_t idx, uint32_t value);
};

}  // namespace SPF::Modules::API
