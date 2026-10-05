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

  // --- Public Setters (for use by finder implementations) ---
  void SetProcessBankDepositAddr(uintptr_t addr) { m_processBankDepositAddr = addr; }
  void SetProcessBankWithdrawalAddr(uintptr_t addr) { m_processBankWithdrawalAddr = addr; }
  void SetTryProcessTransactionAddr(uintptr_t addr) { m_tryProcessTransactionAddr = addr; }
  void SetEconomyToBankOffset(int32_t off) { m_economyToBankOffset = off; }
  void SetEconomyToMailCtxOffset(int32_t off) { m_economyToMailCtxOffset = off; }
  void SetBankToBalanceOffset(int32_t off) { m_bankToBalanceOffset = off; }
  void SetBankDebtFlagOffset(int32_t off) { m_bankDebtFlagOffset = off; }

  // --- Money API ---
  int64_t GetBalance() const;
  bool AddMoney(int64_t delta);
  bool SetMoney(int64_t amount);
  void SetGameControl(bool lock);
  bool IsGameControlLocked() const { return m_gameControlLocked.load(std::memory_order_relaxed); }
  bool GetDebtFlag() const;

  bool InstallMoneyHooks();
  void RemoveMoneyHooks();

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
  BankFn_t m_depositTrampoline = nullptr;
  BankFn_t m_withdrawTrampoline = nullptr;
  bool m_hooksInstalled = false;
  std::atomic<bool> m_gameControlLocked{false};
  std::vector<std::unique_ptr<IEconomyDataFinder>> m_dataFinders;
};

}  // namespace SPF::Data::GameData
