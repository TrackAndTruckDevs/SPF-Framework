#include "SPF/Data/GameData/EconomyService.hpp"

#include "SPF/Data/GameData/Finders/EconomyDataFinder.hpp"
#include "SPF/Data/GameData/ManagerCoreService.hpp"
#include "SPF/Data/GameData/WorldServiceRegistry.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <MinHook.h>
#include <minwindef.h>

namespace SPF::Data::GameData {

namespace {
// Thread-local bypass: set only around the service's own deposit/withdraw calls so
// the game-control detours let them through while game-originated calls are blocked.
thread_local bool t_bypassMoneyCall = false;

struct MoneyBypassGuard {
  MoneyBypassGuard() { t_bypassMoneyCall = true; }
  ~MoneyBypassGuard() { t_bypassMoneyCall = false; }
};
}  // namespace

// ============================================================================
// Singleton / Lifecycle
// ============================================================================

EconomyService::EconomyService() { WorldServiceRegistry::Get().Register(this); }

EconomyService& EconomyService::GetInstance() {
  static EconomyService instance;
  return instance;
}

// ============================================================================
// Initialization / Finders
// ============================================================================

void EconomyService::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");
  logger->Info("Attempting to initialize EconomyService...");

  RegisterFinders();

  m_isInitialized = false;
  logger->Info("EconomyService initialization finished. Waiting for function entries.");
}

void EconomyService::Reset() {
  RemoveMoneyHooks();

  m_isInitialized = false;

  m_processBankDepositAddr = 0;
  m_processBankWithdrawalAddr = 0;
  m_tryProcessTransactionAddr = 0;

  for (const auto& finder : m_dataFinders) {
    finder->Reset();
  }
}

void EconomyService::RegisterFinders() { m_dataFinders.push_back(std::make_unique<Finders::EconomyDataFinder>()); }

bool EconomyService::IsFinderReady(const char* name) const {
  for (const auto& finder : m_dataFinders) {
    if (strcmp(finder->GetName(), name) == 0) {
      return finder->IsReady();
    }
  }
  return false;
}

bool EconomyService::AreAllFindersReady() const {
  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) return false;
  }
  return true;
}

bool EconomyService::TryFindAllOffsets() {
  if (m_isInitialized) return true;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");

  for (auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      if (!finder->TryFindOffsets(*this)) {
        return false;
      }
    }
  }

  m_isInitialized = true;
  logger->Info("EconomyService: All function entries found. Service is READY.");
  return true;
}

// ============================================================================
// Money API
// ============================================================================

int64_t EconomyService::GetBalance() const {
  uintptr_t economyPtrAddr = ManagerCoreService::GetInstance().GetEconomyManagerAddr();
  if (!economyPtrAddr) return 0;
  uintptr_t economy = *(uintptr_t*)economyPtrAddr;
  if (!economy) return 0;
  uintptr_t bank = *(uintptr_t*)(economy + m_economyToBankOffset);
  if (!bank) return 0;
  return *(int64_t*)(bank + m_bankToBalanceOffset);
}

bool EconomyService::GetDebtFlag() const {
  uintptr_t economyPtrAddr = ManagerCoreService::GetInstance().GetEconomyManagerAddr();
  if (!economyPtrAddr) return false;
  uintptr_t economy = *(uintptr_t*)economyPtrAddr;
  if (!economy) return false;
  uintptr_t bank = *(uintptr_t*)(economy + m_economyToBankOffset);
  if (!bank) return false;
  return *(uint8_t*)(bank + m_bankDebtFlagOffset) != 0;
}

bool EconomyService::AddMoney(int64_t delta) {
  if (!IsReady() || delta == 0) return false;

  uintptr_t economyPtrAddr = ManagerCoreService::GetInstance().GetEconomyManagerAddr();
  if (!economyPtrAddr) return false;
  uintptr_t economy = *(uintptr_t*)economyPtrAddr;
  if (!economy) return false;
  uintptr_t bank = *(uintptr_t*)(economy + m_economyToBankOffset);
  if (!bank) return false;
  uintptr_t mailCtx = *(uintptr_t*)(economy + m_economyToMailCtxOffset);

  auto fn = reinterpret_cast<BankFn_t>(delta > 0 ? m_processBankDepositAddr : m_processBankWithdrawalAddr);
  int64_t amount = delta > 0 ? delta : -delta;

  MoneyBypassGuard guard;
  int64_t result = fn(bank, mailCtx, amount, 1);
  (void)result;
  return true;
}

bool EconomyService::SetMoney(int64_t amount) { return AddMoney(amount - GetBalance()); }

void EconomyService::SetGameControl(bool lock) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");
  if (lock && !InstallMoneyHooks()) {
    logger->Error("SetGameControl(true): failed to install money hooks, lock NOT applied.");
    return;
  }
  m_gameControlLocked.store(lock, std::memory_order_relaxed);
  logger->Info("Game control lock = {}", lock);
}

// ============================================================================
// Money hooks (SetGameControl)
// ============================================================================

int64_t EconomyService::HookedProcessBankDeposit(uintptr_t bank, uintptr_t mailCtx, int64_t amount, char playSound) {
  auto& svc = GetInstance();
  if (svc.m_gameControlLocked.load(std::memory_order_relaxed) && !t_bypassMoneyCall) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");
    logger->Info("Blocked game ProcessBankDeposit (amount={}, playSound={})", amount, static_cast<int>(playSound));
    return 0;
  }
  return svc.m_depositTrampoline(bank, mailCtx, amount, playSound);
}

int64_t EconomyService::HookedProcessBankWithdrawal(uintptr_t bank, uintptr_t mailCtx, int64_t amount, char playSound) {
  auto& svc = GetInstance();
  if (svc.m_gameControlLocked.load(std::memory_order_relaxed) && !t_bypassMoneyCall) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");
    logger->Info("Blocked game ProcessBankWithdrawal (amount={}, playSound={})", amount, static_cast<int>(playSound));
    return 0;
  }
  return svc.m_withdrawTrampoline(bank, mailCtx, amount, playSound);
}

bool EconomyService::InstallMoneyHooks() {
  if (m_hooksInstalled) return true;
  if (!IsReady()) return false;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");
  MH_STATUS initStatus = MH_Initialize();
  if (initStatus != MH_OK && initStatus != MH_ERROR_ALREADY_INITIALIZED) {
    logger->Error("Money hooks: MH_Initialize failed: {}", static_cast<int>(initStatus));
    return false;
  }

  MH_STATUS stDep = MH_CreateHook(reinterpret_cast<LPVOID>(m_processBankDepositAddr), reinterpret_cast<LPVOID>(&EconomyService::HookedProcessBankDeposit), reinterpret_cast<LPVOID*>(&m_depositTrampoline));
  if (stDep != MH_OK) {
    logger->Error("MH_CreateHook(ProcessBankDeposit) failed: {}", static_cast<int>(stDep));
    m_depositTrampoline = nullptr;
    return false;
  }
  MH_STATUS stWdr = MH_CreateHook(reinterpret_cast<LPVOID>(m_processBankWithdrawalAddr), reinterpret_cast<LPVOID>(&EconomyService::HookedProcessBankWithdrawal), reinterpret_cast<LPVOID*>(&m_withdrawTrampoline));
  if (stWdr != MH_OK) {
    logger->Error("MH_CreateHook(ProcessBankWithdrawal) failed: {}", static_cast<int>(stWdr));
    m_withdrawTrampoline = nullptr;
    MH_RemoveHook(reinterpret_cast<LPVOID>(m_processBankDepositAddr));
    return false;
  }

  MH_STATUS enDep = MH_EnableHook(reinterpret_cast<LPVOID>(m_processBankDepositAddr));
  MH_STATUS enWdr = MH_EnableHook(reinterpret_cast<LPVOID>(m_processBankWithdrawalAddr));
  if (enDep != MH_OK || enWdr != MH_OK) {
    logger->Error("MH_EnableHook(money) failed: deposit={} withdrawal={}", static_cast<int>(enDep), static_cast<int>(enWdr));
    MH_DisableHook(reinterpret_cast<LPVOID>(m_processBankDepositAddr));
    MH_DisableHook(reinterpret_cast<LPVOID>(m_processBankWithdrawalAddr));
    MH_RemoveHook(reinterpret_cast<LPVOID>(m_processBankDepositAddr));
    MH_RemoveHook(reinterpret_cast<LPVOID>(m_processBankWithdrawalAddr));
    m_depositTrampoline = nullptr;
    m_withdrawTrampoline = nullptr;
    return false;
  }

  m_hooksInstalled = true;
  logger->Info("Money hooks installed (deposit=0x{:X}, withdrawal=0x{:X})", m_processBankDepositAddr, m_processBankWithdrawalAddr);
  return true;
}

void EconomyService::RemoveMoneyHooks() {
  if (!m_hooksInstalled) return;

  m_gameControlLocked.store(false, std::memory_order_relaxed);
  MH_DisableHook(reinterpret_cast<LPVOID>(m_processBankDepositAddr));
  MH_DisableHook(reinterpret_cast<LPVOID>(m_processBankWithdrawalAddr));
  MH_RemoveHook(reinterpret_cast<LPVOID>(m_processBankDepositAddr));
  MH_RemoveHook(reinterpret_cast<LPVOID>(m_processBankWithdrawalAddr));
  m_depositTrampoline = nullptr;
  m_withdrawTrampoline = nullptr;
  m_hooksInstalled = false;
}

}  // namespace SPF::Data::GameData
