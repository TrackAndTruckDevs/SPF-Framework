#pragma once

#include "SPF/Data/GameData/IEconomyDataFinder.hpp"
#include "SPF/Data/GameData/IWorldScopedService.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

namespace SPF::Data::GameData {

/**
 * @class EconomyService
 * @brief A singleton service that owns and runs economy finders (ProcessBankDeposit, ...).
 */
class EconomyService : public IWorldScopedService {
 public:
  static EconomyService& GetInstance();

  EconomyService(const EconomyService&) = delete;
  void operator=(const EconomyService&) = delete;

  void Initialize();
  void Reset();
  bool TryFindAllOffsets();
  bool AreAllFindersReady() const;
  bool IsFinderReady(const char* finderName) const;

  // --- IWorldScopedService ---
  const char* GetName() const override { return "EconomyService"; }
  void ResetForWorldReload() override { Reset(); }
  bool TryFinalizeWorldInit() override { return TryFindAllOffsets(); }
  bool IsReady() const { return m_isInitialized && m_processBankDepositAddr != 0 && m_processBankWithdrawalAddr != 0 && m_tryProcessTransactionAddr != 0; }

  // --- Public Getters ---
  uintptr_t GetProcessBankDepositAddr() const { return m_processBankDepositAddr; }
  uintptr_t GetProcessBankWithdrawalAddr() const { return m_processBankWithdrawalAddr; }
  uintptr_t GetTryProcessTransactionAddr() const { return m_tryProcessTransactionAddr; }
  int32_t GetEconomyToBankOffset() const { return m_economyToBankOffset; }
  int32_t GetEconomyToMailCtxOffset() const { return m_economyToMailCtxOffset; }
  int32_t GetBankToBalanceOffset() const { return m_bankToBalanceOffset; }
  int32_t GetBankDebtFlagOffset() const { return m_bankDebtFlagOffset; }

  // --- Bank loan struct offsets (ConfirmBankDialogAction) ---
  uintptr_t GetConfirmBankDialogAddr() const { return m_confirmBankDialogAddr; }
  uintptr_t GetGetCreditLimitAddr() const { return m_getCreditLimitAddr; }
  uintptr_t GetAddLoanAddr() const { return m_addLoanAddr; }
  uintptr_t GetCanRepayLoanAddr() const { return m_canRepayLoanAddr; }
  uintptr_t GetRemoveLoanAddr() const { return m_removeLoanAddr; }
  int32_t GetBankToOffersOffset() const { return m_bankToOffersOffset; }
  int32_t GetBankCanLoanOffset() const { return m_bankCanLoanOffset; }
  int32_t GetBankToLoansOffset() const { return m_bankToLoansOffset; }
  int32_t GetLoansPtrOffset() const { return m_loansPtrOffset; }
  int32_t GetLoansCountOffset() const { return m_loansCountOffset; }
  int32_t GetLoanOfferAmountOffset() const { return m_loanOfferAmountOffset; }
  int32_t GetLoanOfferRateOffset() const { return m_loanOfferRateOffset; }
  int32_t GetLoanOfferWeeksOffset() const { return m_loanOfferWeeksOffset; }
  int32_t GetLoanRemainingOffset() const { return m_loanRemainingOffset; }
  int32_t GetLoanAmountOffset() const { return m_loanAmountOffset; }
  int32_t GetLoanStartDayOffset() const { return m_loanStartDayOffset; }
  int32_t GetLoanRateOffset() const { return m_loanRateOffset; }
  int32_t GetLoanWeeksOffset() const { return m_loanWeeksOffset; }

  // --- Loan create entries (factory + unit system from ConfirmBankDialogAction) ---
  uintptr_t GetAddrOfferFields() const { return m_addrOfferFields; }
  uintptr_t GetCreateLoanAddr() const { return m_createLoanAddr; }
  uintptr_t GetUnitSystemIndexAddr() const { return m_unitSystemIndexAddr; }
  uintptr_t GetUnitSystemTableAddr() const { return m_unitSystemTableAddr; }
  uintptr_t GetFreeRefCountAddr() const { return m_freeRefCountAddr; }

  // --- Loan offers container offsets (OpenNewLoanConfirmDialog) ---
  uintptr_t GetOpenNewLoanConfirmAddr() const { return m_openNewLoanConfirmAddr; }
  int32_t GetOffersBaseAddOffset() const { return m_offersBaseAddOffset; }
  int32_t GetOffersCountOffset() const { return m_offersCountOffset; }
  int32_t GetOffersPtrOffset() const { return m_offersPtrOffset; }

  // --- Reflection attribute offsets (SII: bank_data / bank_loan_data / insurance_data) ---
  int32_t GetAttrMinAfterLoanRepay() const { return m_attrMinAfterLoanRepay; }
  int32_t GetAttrLoanOffers() const { return m_attrLoanOffers; }
  int32_t GetAttrOverdraftDuration() const { return m_attrOverdraftDuration; }
  int32_t GetAttrOverdraftWarnDay() const { return m_attrOverdraftWarnDay; }
  int32_t GetAttrInsuranceData() const { return m_attrInsuranceData; }
  int32_t GetAttrLoanLimit() const { return m_attrLoanLimit; }
  int32_t GetAttrMoneyAccount() const { return m_attrMoneyAccount; }
  int32_t GetAttrCoinsuranceFixed() const { return m_attrCoinsuranceFixed; }
  int32_t GetAttrCoinsuranceRatio() const { return m_attrCoinsuranceRatio; }
  int32_t GetAttrAccidentSeverity() const { return m_attrAccidentSeverity; }
  int32_t GetAttrLoans() const { return m_attrLoans; }
  int32_t GetAttrAppEnabled() const { return m_attrAppEnabled; }
  int32_t GetAttrPaymentTimer() const { return m_attrPaymentTimer; }
  int32_t GetAttrOverdraft() const { return m_attrOverdraft; }
  int32_t GetAttrOverdraftTimer() const { return m_attrOverdraftTimer; }
  int32_t GetAttrOverdraftWarnCount() const { return m_attrOverdraftWarnCount; }
  int32_t GetAttrSellPlayersTruckLater() const { return m_attrSellPlayersTruckLater; }
  int32_t GetAttrSellPlayersTrailerLater() const { return m_attrSellPlayersTrailerLater; }
  int32_t GetAttrLoanAmount() const { return m_attrLoanAmount; }
  int32_t GetAttrLoanInterestRate() const { return m_attrLoanInterestRate; }
  int32_t GetAttrLoanDuration() const { return m_attrLoanDuration; }
  int32_t GetAttrCoinsuranceFixedBase() const { return m_attrCoinsuranceFixedBase; }
  int32_t GetAttrCoinsuranceFixedMin() const { return m_attrCoinsuranceFixedMin; }
  int32_t GetAttrCoinsuranceFixedMax() const { return m_attrCoinsuranceFixedMax; }
  int32_t GetAttrCoinsuranceFixedInc() const { return m_attrCoinsuranceFixedInc; }
  int32_t GetAttrCoinsuranceFixedDec() const { return m_attrCoinsuranceFixedDec; }
  int32_t GetAttrCoinsuranceRatioBase() const { return m_attrCoinsuranceRatioBase; }
  int32_t GetAttrCoinsuranceRatioMin() const { return m_attrCoinsuranceRatioMin; }
  int32_t GetAttrCoinsuranceRatioMax() const { return m_attrCoinsuranceRatioMax; }
  int32_t GetAttrCoinsuranceRatioInc() const { return m_attrCoinsuranceRatioInc; }
  int32_t GetAttrCoinsuranceRatioDec() const { return m_attrCoinsuranceRatioDec; }

  // --- Public Setters (for use by finder implementations) ---
  void SetProcessBankDepositAddr(uintptr_t addr) { m_processBankDepositAddr = addr; }
  void SetProcessBankWithdrawalAddr(uintptr_t addr) { m_processBankWithdrawalAddr = addr; }
  void SetTryProcessTransactionAddr(uintptr_t addr) { m_tryProcessTransactionAddr = addr; }
  void SetEconomyToBankOffset(int32_t off) { m_economyToBankOffset = off; }
  void SetEconomyToMailCtxOffset(int32_t off) { m_economyToMailCtxOffset = off; }
  void SetBankToBalanceOffset(int32_t off) { m_bankToBalanceOffset = off; }
  void SetBankDebtFlagOffset(int32_t off) { m_bankDebtFlagOffset = off; }
  void SetConfirmBankDialogAddr(uintptr_t addr) { m_confirmBankDialogAddr = addr; }
  void SetGetCreditLimitAddr(uintptr_t addr) { m_getCreditLimitAddr = addr; }
  void SetAddLoanAddr(uintptr_t addr) { m_addLoanAddr = addr; }
  void SetCanRepayLoanAddr(uintptr_t addr) { m_canRepayLoanAddr = addr; }
  void SetRemoveLoanAddr(uintptr_t addr) { m_removeLoanAddr = addr; }
  void SetBankToOffersOffset(int32_t off) { m_bankToOffersOffset = off; }
  void SetBankCanLoanOffset(int32_t off) { m_bankCanLoanOffset = off; }
  void SetBankToLoansOffset(int32_t off) { m_bankToLoansOffset = off; }
  void SetLoansPtrOffset(int32_t off) { m_loansPtrOffset = off; }
  void SetLoansCountOffset(int32_t off) { m_loansCountOffset = off; }
  void SetLoanOfferAmountOffset(int32_t off) { m_loanOfferAmountOffset = off; }
  void SetLoanOfferRateOffset(int32_t off) { m_loanOfferRateOffset = off; }
  void SetLoanOfferWeeksOffset(int32_t off) { m_loanOfferWeeksOffset = off; }
  void SetLoanRemainingOffset(int32_t off) { m_loanRemainingOffset = off; }
  void SetLoanAmountOffset(int32_t off) { m_loanAmountOffset = off; }
  void SetLoanStartDayOffset(int32_t off) { m_loanStartDayOffset = off; }
  void SetLoanRateOffset(int32_t off) { m_loanRateOffset = off; }
  void SetLoanWeeksOffset(int32_t off) { m_loanWeeksOffset = off; }
  void SetAddrOfferFields(uintptr_t addr) { m_addrOfferFields = addr; }
  void SetCreateLoanAddr(uintptr_t addr) { m_createLoanAddr = addr; }
  void SetUnitSystemIndexAddr(uintptr_t addr) { m_unitSystemIndexAddr = addr; }
  void SetUnitSystemTableAddr(uintptr_t addr) { m_unitSystemTableAddr = addr; }
  void SetFreeRefCountAddr(uintptr_t addr) { m_freeRefCountAddr = addr; }
  void SetOpenNewLoanConfirmAddr(uintptr_t addr) { m_openNewLoanConfirmAddr = addr; }
  void SetOffersBaseAddOffset(int32_t off) { m_offersBaseAddOffset = off; }
  void SetOffersCountOffset(int32_t off) { m_offersCountOffset = off; }
  void SetOffersPtrOffset(int32_t off) { m_offersPtrOffset = off; }
  void SetAttrMinAfterLoanRepay(int32_t off) { m_attrMinAfterLoanRepay = off; }
  void SetAttrLoanOffers(int32_t off) { m_attrLoanOffers = off; }
  void SetAttrOverdraftDuration(int32_t off) { m_attrOverdraftDuration = off; }
  void SetAttrOverdraftWarnDay(int32_t off) { m_attrOverdraftWarnDay = off; }
  void SetAttrInsuranceData(int32_t off) { m_attrInsuranceData = off; }
  void SetAttrLoanLimit(int32_t off) { m_attrLoanLimit = off; }
  void SetAttrMoneyAccount(int32_t off) { m_attrMoneyAccount = off; }
  void SetAttrCoinsuranceFixed(int32_t off) { m_attrCoinsuranceFixed = off; }
  void SetAttrCoinsuranceRatio(int32_t off) { m_attrCoinsuranceRatio = off; }
  void SetAttrAccidentSeverity(int32_t off) { m_attrAccidentSeverity = off; }
  void SetAttrLoans(int32_t off) { m_attrLoans = off; }
  void SetAttrAppEnabled(int32_t off) { m_attrAppEnabled = off; }
  void SetAttrPaymentTimer(int32_t off) { m_attrPaymentTimer = off; }
  void SetAttrOverdraft(int32_t off) { m_attrOverdraft = off; }
  void SetAttrOverdraftTimer(int32_t off) { m_attrOverdraftTimer = off; }
  void SetAttrOverdraftWarnCount(int32_t off) { m_attrOverdraftWarnCount = off; }
  void SetAttrSellPlayersTruckLater(int32_t off) { m_attrSellPlayersTruckLater = off; }
  void SetAttrSellPlayersTrailerLater(int32_t off) { m_attrSellPlayersTrailerLater = off; }
  void SetAttrLoanAmount(int32_t off) { m_attrLoanAmount = off; }
  void SetAttrLoanInterestRate(int32_t off) { m_attrLoanInterestRate = off; }
  void SetAttrLoanDuration(int32_t off) { m_attrLoanDuration = off; }
  void SetAttrCoinsuranceFixedBase(int32_t off) { m_attrCoinsuranceFixedBase = off; }
  void SetAttrCoinsuranceFixedMin(int32_t off) { m_attrCoinsuranceFixedMin = off; }
  void SetAttrCoinsuranceFixedMax(int32_t off) { m_attrCoinsuranceFixedMax = off; }
  void SetAttrCoinsuranceFixedInc(int32_t off) { m_attrCoinsuranceFixedInc = off; }
  void SetAttrCoinsuranceFixedDec(int32_t off) { m_attrCoinsuranceFixedDec = off; }
  void SetAttrCoinsuranceRatioBase(int32_t off) { m_attrCoinsuranceRatioBase = off; }
  void SetAttrCoinsuranceRatioMin(int32_t off) { m_attrCoinsuranceRatioMin = off; }
  void SetAttrCoinsuranceRatioMax(int32_t off) { m_attrCoinsuranceRatioMax = off; }
  void SetAttrCoinsuranceRatioInc(int32_t off) { m_attrCoinsuranceRatioInc = off; }
  void SetAttrCoinsuranceRatioDec(int32_t off) { m_attrCoinsuranceRatioDec = off; }

  // --- Money API ---
  int64_t GetBalance() const;
  bool AddMoney(int64_t delta);
  bool SetMoney(int64_t amount);
  void SetGameControl(bool lock);
  bool IsGameControlLocked() const { return m_gameControlLocked.load(std::memory_order_relaxed); }
  bool GetDebtFlag() const;

  // --- Loan API ---
  struct ActiveLoanInfo {
    int64_t remaining = 0;
    int64_t amount = 0;
    uint32_t startDay = 0;
    float rate = 0.0f;
    uint32_t weeks = 0;
  };
  struct LoanOfferInfo {
    uint32_t amount = 0;
    float rate = 0.0f;
    uint32_t weeks = 0;
  };

  int64_t GetCreditLimit() const;
  int32_t GetActiveLoanCount() const;
  bool GetActiveLoan(int32_t idx, ActiveLoanInfo& out) const;
  int32_t GetOfferCount() const;
  bool GetOffer(int32_t idx, LoanOfferInfo& out) const;
  bool CanRepayLoan(int32_t idx) const;
  bool RepayLoan(int32_t idx);
  bool TakeLoan(uint64_t amount, float rate, uint32_t weeks);
  bool TakeOffer(int32_t idx);

  // --- Loan editing ---
  int64_t GetLoanLimit() const;
  bool SetCreditLimit(int64_t limit);
  uintptr_t GetActiveLoanPtr(int32_t idx) const;
  bool SetLoanRemaining(int32_t idx, int64_t value);
  bool SetLoanAmount(int32_t idx, int64_t value);
  bool SetLoanRate(int32_t idx, float value);
  bool SetLoanWeeks(int32_t idx, uint32_t value);
  bool SetLoanStartDay(int32_t idx, uint32_t value);

  bool InstallMoneyHooks();
  void RemoveMoneyHooks();

  // --- Bank config diagnostics ---
  void LogBankConfigSnapshot() const;

 private:
  using BankFn_t = int64_t (*)(uintptr_t bank, uintptr_t mailCtx, int64_t amount, char playSound);

  static int64_t HookedProcessBankDeposit(uintptr_t bank, uintptr_t mailCtx, int64_t amount, char playSound);
  static int64_t HookedProcessBankWithdrawal(uintptr_t bank, uintptr_t mailCtx, int64_t amount, char playSound);

  EconomyService();
  ~EconomyService() = default;

  void RegisterFinders();

  bool m_isInitialized = false;
  uintptr_t m_processBankDepositAddr = 0;
  uintptr_t m_processBankWithdrawalAddr = 0;
  uintptr_t m_tryProcessTransactionAddr = 0;
  int32_t m_economyToBankOffset = 0;
  int32_t m_economyToMailCtxOffset = 0;
  int32_t m_bankToBalanceOffset = 0;
  int32_t m_bankDebtFlagOffset = 0;

  uintptr_t m_confirmBankDialogAddr = 0;
  int32_t m_bankToOffersOffset = 0;
  int32_t m_bankCanLoanOffset = 0;
  int32_t m_bankToLoansOffset = 0;
  int32_t m_loansPtrOffset = 0;
  int32_t m_loansCountOffset = 0;
  int32_t m_loanOfferAmountOffset = 0;
  int32_t m_loanOfferRateOffset = 0;
  int32_t m_loanOfferWeeksOffset = 0;
  int32_t m_loanRemainingOffset = 0;
  int32_t m_loanAmountOffset = 0;
  int32_t m_loanStartDayOffset = 0;
  int32_t m_loanRateOffset = 0;
  int32_t m_loanWeeksOffset = 0;

  uintptr_t m_openNewLoanConfirmAddr = 0;
  uintptr_t m_getCreditLimitAddr = 0;
  uintptr_t m_addLoanAddr = 0;
  uintptr_t m_canRepayLoanAddr = 0;
  uintptr_t m_removeLoanAddr = 0;
  int32_t m_offersBaseAddOffset = 0;
  int32_t m_offersCountOffset = 0;
  int32_t m_offersPtrOffset = 0;

  uintptr_t m_addrOfferFields = 0;
  uintptr_t m_createLoanAddr = 0;
  uintptr_t m_unitSystemIndexAddr = 0;
  uintptr_t m_unitSystemTableAddr = 0;
  uintptr_t m_freeRefCountAddr = 0;

  uintptr_t GetBankObject() const;
  void ReleaseLoanObject(uintptr_t loan);

  int32_t m_attrMinAfterLoanRepay = 0;
  int32_t m_attrLoanOffers = 0;
  int32_t m_attrOverdraftDuration = 0;
  int32_t m_attrOverdraftWarnDay = 0;
  int32_t m_attrInsuranceData = 0;
  int32_t m_attrLoanLimit = 0;
  int32_t m_attrMoneyAccount = 0;
  int32_t m_attrCoinsuranceFixed = 0;
  int32_t m_attrCoinsuranceRatio = 0;
  int32_t m_attrAccidentSeverity = 0;
  int32_t m_attrLoans = 0;
  int32_t m_attrAppEnabled = 0;
  int32_t m_attrPaymentTimer = 0;
  int32_t m_attrOverdraft = 0;
  int32_t m_attrOverdraftTimer = 0;
  int32_t m_attrOverdraftWarnCount = 0;
  int32_t m_attrSellPlayersTruckLater = 0;
  int32_t m_attrSellPlayersTrailerLater = 0;
  int32_t m_attrLoanAmount = 0;
  int32_t m_attrLoanInterestRate = 0;
  int32_t m_attrLoanDuration = 0;
  int32_t m_attrCoinsuranceFixedBase = 0;
  int32_t m_attrCoinsuranceFixedMin = 0;
  int32_t m_attrCoinsuranceFixedMax = 0;
  int32_t m_attrCoinsuranceFixedInc = 0;
  int32_t m_attrCoinsuranceFixedDec = 0;
  int32_t m_attrCoinsuranceRatioBase = 0;
  int32_t m_attrCoinsuranceRatioMin = 0;
  int32_t m_attrCoinsuranceRatioMax = 0;
  int32_t m_attrCoinsuranceRatioInc = 0;
  int32_t m_attrCoinsuranceRatioDec = 0;
  BankFn_t m_depositTrampoline = nullptr;
  BankFn_t m_withdrawTrampoline = nullptr;
  bool m_hooksInstalled = false;
  std::atomic<bool> m_gameControlLocked{false};
  std::vector<std::unique_ptr<IEconomyDataFinder>> m_dataFinders;
};

}  // namespace SPF::Data::GameData
