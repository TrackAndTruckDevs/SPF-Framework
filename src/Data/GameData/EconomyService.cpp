#include "SPF/Data/GameData/EconomyService.hpp"

#include "SPF/Data/GameData/Finders/EconomyDataFinder.hpp"
#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/Data/GameData/ManagerCoreService.hpp"
#include "SPF/Data/GameData/WorldServiceRegistry.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <intrin.h>
#include <memory>
#include <MinHook.h>
#include <minwindef.h>
#include <winnt.h>

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
  m_economyToBankOffset = 0;
  m_economyToMailCtxOffset = 0;
  m_bankToBalanceOffset = 0;
  m_bankDebtFlagOffset = 0;
  m_confirmBankDialogAddr = 0;
  m_bankToOffersOffset = 0;
  m_bankCanLoanOffset = 0;
  m_bankToLoansOffset = 0;
  m_loansPtrOffset = 0;
  m_loansCountOffset = 0;
  m_loanOfferAmountOffset = 0;
  m_loanOfferRateOffset = 0;
  m_loanOfferWeeksOffset = 0;
  m_loanRemainingOffset = 0;
  m_loanAmountOffset = 0;
  m_loanStartDayOffset = 0;
  m_loanRateOffset = 0;
  m_loanWeeksOffset = 0;
  m_openNewLoanConfirmAddr = 0;
  m_getCreditLimitAddr = 0;
  m_addLoanAddr = 0;
  m_canRepayLoanAddr = 0;
  m_removeLoanAddr = 0;
  m_offersBaseAddOffset = 0;
  m_offersCountOffset = 0;
  m_offersPtrOffset = 0;
  m_addrOfferFields = 0;
  m_createLoanAddr = 0;
  m_unitSystemIndexAddr = 0;
  m_unitSystemTableAddr = 0;
  m_freeRefCountAddr = 0;
  m_attrMinAfterLoanRepay = 0;
  m_attrLoanOffers = 0;
  m_attrOverdraftDuration = 0;
  m_attrOverdraftWarnDay = 0;
  m_attrInsuranceData = 0;
  m_attrLoanLimit = 0;
  m_attrMoneyAccount = 0;
  m_attrCoinsuranceFixed = 0;
  m_attrCoinsuranceRatio = 0;
  m_attrAccidentSeverity = 0;
  m_attrLoans = 0;
  m_attrAppEnabled = 0;
  m_attrPaymentTimer = 0;
  m_attrOverdraft = 0;
  m_attrOverdraftTimer = 0;
  m_attrOverdraftWarnCount = 0;
  m_attrSellPlayersTruckLater = 0;
  m_attrSellPlayersTrailerLater = 0;
  m_attrLoanAmount = 0;
  m_attrLoanInterestRate = 0;
  m_attrLoanDuration = 0;
  m_attrCoinsuranceFixedBase = 0;
  m_attrCoinsuranceFixedMin = 0;
  m_attrCoinsuranceFixedMax = 0;
  m_attrCoinsuranceFixedInc = 0;
  m_attrCoinsuranceFixedDec = 0;
  m_attrCoinsuranceRatioBase = 0;
  m_attrCoinsuranceRatioMin = 0;
  m_attrCoinsuranceRatioMax = 0;
  m_attrCoinsuranceRatioInc = 0;
  m_attrCoinsuranceRatioDec = 0;

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

// ============================================================================
// Loan API
// ============================================================================

int64_t EconomyService::GetCreditLimit() const {
  if (!m_getCreditLimitAddr) return 0;
  uintptr_t bank = GetBankObject();
  if (!bank) return 0;
  using Fn_t = int64_t (*)(uintptr_t);
  return reinterpret_cast<Fn_t>(m_getCreditLimitAddr)(bank);
}

int32_t EconomyService::GetActiveLoanCount() const {
  uintptr_t bank = GetBankObject();
  if (!bank || m_loansCountOffset <= 0) return 0;
  return static_cast<int32_t>(*reinterpret_cast<int64_t*>(bank + m_loansCountOffset));
}

bool EconomyService::GetActiveLoan(int32_t idx, ActiveLoanInfo& out) const {
  if (idx < 0 || m_loansPtrOffset <= 0 || m_loansCountOffset <= 0) return false;
  uintptr_t bank = GetBankObject();
  if (!bank) return false;
  int64_t count = *reinterpret_cast<int64_t*>(bank + m_loansCountOffset);
  if (idx >= count) return false;
  uintptr_t data = *reinterpret_cast<uintptr_t*>(bank + m_loansPtrOffset);
  if (!data) return false;
  uintptr_t loan = *reinterpret_cast<uintptr_t*>(data + static_cast<uintptr_t>(idx) * 8);
  if (!loan) return false;
  out.remaining = *reinterpret_cast<int64_t*>(loan + m_loanRemainingOffset);
  out.amount = *reinterpret_cast<int64_t*>(loan + m_loanAmountOffset);
  out.startDay = *reinterpret_cast<uint32_t*>(loan + m_loanStartDayOffset);
  out.rate = *reinterpret_cast<float*>(loan + m_loanRateOffset);
  out.weeks = *reinterpret_cast<uint32_t*>(loan + m_loanWeeksOffset);
  return true;
}

int32_t EconomyService::GetOfferCount() const {
  uintptr_t bank = GetBankObject();
  if (!bank || m_bankToOffersOffset <= 0 || m_offersBaseAddOffset <= 0 || m_offersCountOffset <= 0) return 0;
  uintptr_t container = *reinterpret_cast<uintptr_t*>(bank + m_bankToOffersOffset);
  if (!container) return 0;
  uintptr_t base = container + m_offersBaseAddOffset;
  return static_cast<int32_t>(*reinterpret_cast<int64_t*>(base + m_offersCountOffset));
}

bool EconomyService::GetOffer(int32_t idx, LoanOfferInfo& out) const {
  if (idx < 0 || m_offersPtrOffset <= 0) return false;
  uintptr_t bank = GetBankObject();
  if (!bank) return false;
  uintptr_t container = *reinterpret_cast<uintptr_t*>(bank + m_bankToOffersOffset);
  if (!container) return false;
  uintptr_t base = container + m_offersBaseAddOffset;
  int64_t count = *reinterpret_cast<int64_t*>(base + m_offersCountOffset);
  if (idx >= count) return false;
  uintptr_t data = *reinterpret_cast<uintptr_t*>(base + m_offersPtrOffset);
  if (!data) return false;
  uintptr_t offer = *reinterpret_cast<uintptr_t*>(data + static_cast<uintptr_t>(idx) * 8);
  if (!offer) return false;
  out.amount = *reinterpret_cast<uint32_t*>(offer + m_loanOfferAmountOffset);
  out.rate = *reinterpret_cast<float*>(offer + m_loanOfferRateOffset);
  out.weeks = *reinterpret_cast<uint32_t*>(offer + m_loanOfferWeeksOffset);
  return true;
}

bool EconomyService::CanRepayLoan(int32_t idx) const {
  if (!m_canRepayLoanAddr || idx < 0) return false;
  uintptr_t bank = GetBankObject();
  if (!bank) return false;
  using Fn_t = char (*)(uintptr_t, uint64_t);
  return reinterpret_cast<Fn_t>(m_canRepayLoanAddr)(bank, static_cast<uint64_t>(idx)) != 0;
}

bool EconomyService::RepayLoan(int32_t idx) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");
  if (!IsReady() || !m_removeLoanAddr || idx < 0) {
    logger->Error("RepayLoan({}): service not ready or invalid index.", idx);
    return false;
  }

  uintptr_t economyPtrAddr = ManagerCoreService::GetInstance().GetEconomyManagerAddr();
  if (!economyPtrAddr) return false;
  uintptr_t economy = *reinterpret_cast<uintptr_t*>(economyPtrAddr);
  if (!economy) return false;
  uintptr_t bank = *reinterpret_cast<uintptr_t*>(economy + m_economyToBankOffset);
  if (!bank) return false;

  int64_t count = *reinterpret_cast<int64_t*>(bank + m_loansCountOffset);
  if (idx >= count) {
    logger->Error("RepayLoan({}): index out of range (count={}).", idx, count);
    return false;
  }
  if (*reinterpret_cast<uint8_t*>(bank + m_bankCanLoanOffset) == 0) {
    logger->Error("RepayLoan({}): bank permission denied (bank+{:X}=0).", idx, m_bankCanLoanOffset);
    return false;
  }
  if (!CanRepayLoan(idx)) {
    logger->Error("RepayLoan({}): CanRepayLoan gate failed (remaining + min_after_loan_repay > balance).", idx);
    return false;
  }

  uintptr_t data = *reinterpret_cast<uintptr_t*>(bank + m_loansPtrOffset);
  if (!data) return false;
  uintptr_t loan = *reinterpret_cast<uintptr_t*>(data + static_cast<uintptr_t>(idx) * 8);
  if (!loan) return false;
  int64_t remaining = *reinterpret_cast<int64_t*>(loan + m_loanRemainingOffset);

  uintptr_t mailCtx = *reinterpret_cast<uintptr_t*>(economy + m_economyToMailCtxOffset);
  auto withdraw = reinterpret_cast<BankFn_t>(m_processBankWithdrawalAddr);
  MoneyBypassGuard guard;
  withdraw(bank, mailCtx, remaining, 1);

  auto remove = reinterpret_cast<void (*)(uintptr_t, uint64_t)>(m_removeLoanAddr);
  remove(bank + m_bankToLoansOffset, static_cast<uint64_t>(idx));

  logger->Info("RepayLoan({}): paid remaining {} and removed loan.", idx, remaining);
  return true;
}

// ============================================================================
// Loan creation
// ============================================================================

void EconomyService::ReleaseLoanObject(uintptr_t loan) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");
  if (!loan) return;

  // vtable[1] = IRefcountable::Release (drops ref + destroys at zero)
  uintptr_t vtable = *reinterpret_cast<uintptr_t*>(loan);
  if (vtable) {
    using ReleaseFn = void (*)(uintptr_t);
    auto release = reinterpret_cast<ReleaseFn>(*reinterpret_cast<uintptr_t*>(vtable + 8));
    if (release) release(loan);
  }

  // release tail: refcount -= 1; if ((rc-1) & 0x1ffffff) == 0 → pool free
  volatile LONG* rc = reinterpret_cast<volatile LONG*>(loan + 8);
  LONG old = _InterlockedExchangeAdd(rc, -1);
  if (((old - 1) & 0x1ffffff) == 0) {
    if (m_freeRefCountAddr) {
      using FreeFn = void (*)(uintptr_t);
      reinterpret_cast<FreeFn>(m_freeRefCountAddr)(loan);
    } else {
      logger->Error("ReleaseLoanObject: refcount reached zero but pool free function not discovered — object leaked.");
    }
  }
}

bool EconomyService::TakeLoan(uint64_t amount, float rate, uint32_t weeks) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");

  if (!IsReady() || !m_createLoanAddr || !m_addLoanAddr || !m_processBankDepositAddr) {
    logger->Error("TakeLoan: service not ready.");
    return false;
  }
  if (m_unitSystemIndexAddr == 0 || m_unitSystemTableAddr == 0) {
    logger->Error("TakeLoan: unit system index/table not discovered.");
    return false;
  }
  if (amount == 0 || weeks == 0) {
    logger->Error("TakeLoan: invalid args (amount={}, weeks={}).", amount, weeks);
    return false;
  }

  uintptr_t economyPtrAddr = ManagerCoreService::GetInstance().GetEconomyManagerAddr();
  if (!economyPtrAddr) {
    logger->Error("TakeLoan: economy manager address not discovered.");
    return false;
  }
  uintptr_t economy = *reinterpret_cast<uintptr_t*>(economyPtrAddr);
  if (!economy) {
    logger->Error("TakeLoan: economy manager not initialized.");
    return false;
  }
  uintptr_t bank = *reinterpret_cast<uintptr_t*>(economy + m_economyToBankOffset);
  if (!bank) {
    logger->Error("TakeLoan: bank object is null.");
    return false;
  }
  uintptr_t mailCtx = *reinterpret_cast<uintptr_t*>(economy + m_economyToMailCtxOffset);

  intptr_t simDayOff = GameWorldService::GetInstance().GetSimulationTimeOffset();
  if (simDayOff <= 0) {
    logger->Error("TakeLoan: simulation day offset not discovered.");
    return false;
  }

  // Same guards as ConfirmBankDialogAction (checked after factory there; here
  // before so we never build an object we cannot submit).
  if (*reinterpret_cast<uint8_t*>(bank + m_bankCanLoanOffset) == 0) {
    logger->Error("TakeLoan: bank permission denied (bank+{:X} = 0).", m_bankCanLoanOffset);
    return false;
  }
  int64_t limit = GetCreditLimit();
  if (static_cast<int64_t>(amount) > limit) {
    logger->Error("TakeLoan: amount {} exceeds credit limit {}.", amount, limit);
    return false;
  }

  // unit system lookup: table[idx], idx from DAT unit-system index
  uint32_t unitIdx = *reinterpret_cast<uint32_t*>(m_unitSystemIndexAddr);
  if (unitIdx == 4) {
    logger->Error("TakeLoan: default unit system index (4) is invalid.");
    return false;
  }
  uintptr_t unit = *reinterpret_cast<uintptr_t*>(m_unitSystemTableAddr + static_cast<uintptr_t>(unitIdx) * 8);
  if (!unit) {
    logger->Error("TakeLoan: unit system table[{}] is null.", unitIdx);
    return false;
  }

  // CreateBankLoan factory (vtable + refcount init)
  using FactoryFn = uintptr_t (*)(uintptr_t);
  uintptr_t loan = reinterpret_cast<FactoryFn>(m_createLoanAddr)(unit);
  if (!loan) {
    logger->Error("TakeLoan: CreateBankLoan returned null.");
    return false;
  }

  // refcount claim: +0x20000001, then two XOR 0x60000000 toggles, then two
  // more after the field stores — exactly the ConfirmBankDialogAction sequence
  volatile LONG* rc = reinterpret_cast<volatile LONG*>(loan + 8);
  _InterlockedExchangeAdd(rc, 0x20000001);
  _InterlockedXor(rc, 0x60000000);
  _InterlockedXor(rc, 0x60000000);

  *reinterpret_cast<float*>(loan + m_loanRateOffset) = rate;
  *reinterpret_cast<uint64_t*>(loan + m_loanAmountOffset) = amount;
  *reinterpret_cast<uint64_t*>(loan + m_loanRemainingOffset) = amount;
  *reinterpret_cast<uint32_t*>(loan + m_loanWeeksOffset) = weeks;
  *reinterpret_cast<uint32_t*>(loan + m_loanStartDayOffset) = *reinterpret_cast<uint32_t*>(economy + simDayOff);

  _InterlockedXor(rc, 0x60000000);
  _InterlockedXor(rc, 0x60000000);

  // credit the money through the game path (our call; bypass lock, playSound=1)
  auto deposit = reinterpret_cast<BankFn_t>(m_processBankDepositAddr);
  {
    MoneyBypassGuard guard;
    deposit(bank, mailCtx, static_cast<int64_t>(amount), 1);
  }

  // grow bank→loans array by 1 (returns &element or null)
  auto addLoan = reinterpret_cast<char (*)(uintptr_t, int64_t)>(m_addLoanAddr);
  if (addLoan(bank + m_bankToLoansOffset, 1) == 0) {
    logger->Error("TakeLoan: AddLoan failed (array grow).");
    ReleaseLoanObject(loan);
    return false;
  }

  int64_t count = *reinterpret_cast<int64_t*>(bank + m_loansCountOffset);
  uintptr_t data = *reinterpret_cast<uintptr_t*>(bank + m_loansPtrOffset);
  uintptr_t slot = data + (static_cast<uintptr_t>(count) - 1) * 8;
  uintptr_t existing = *reinterpret_cast<uintptr_t*>(slot);
  if (existing) ReleaseLoanObject(existing);
  *reinterpret_cast<uintptr_t*>(slot) = loan;

  logger->Info("TakeLoan({} rate={:.2f} weeks={}): created, credited, loans count={}.", amount, static_cast<double>(rate), weeks, count);
  return true;
}

bool EconomyService::TakeOffer(int32_t idx) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");

  LoanOfferInfo offer{};
  if (!GetOffer(idx, offer)) {
    logger->Error("TakeOffer({}): offer not found.", idx);
    return false;
  }
  return TakeLoan(offer.amount, offer.rate, offer.weeks);
}

// ============================================================================
// Loan editing
// ============================================================================

int64_t EconomyService::GetLoanLimit() const {
  uintptr_t bank = GetBankObject();
  if (!bank || m_attrLoanLimit <= 0) return 0;
  return *reinterpret_cast<int64_t*>(bank + m_attrLoanLimit);
}

bool EconomyService::SetCreditLimit(int64_t limit) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");
  uintptr_t bank = GetBankObject();
  if (!bank || m_attrLoanLimit <= 0) {
    logger->Error("SetCreditLimit({}): bank or loan_limit offset not ready.", limit);
    return false;
  }
  int64_t old = *reinterpret_cast<int64_t*>(bank + m_attrLoanLimit);
  *reinterpret_cast<int64_t*>(bank + m_attrLoanLimit) = limit;
  logger->Info("SetCreditLimit({}) (was {}).", limit, old);
  return true;
}

uintptr_t EconomyService::GetActiveLoanPtr(int32_t idx) const {
  if (idx < 0 || m_loansPtrOffset <= 0 || m_loansCountOffset <= 0) return 0;
  uintptr_t bank = GetBankObject();
  if (!bank) return 0;
  int64_t count = *reinterpret_cast<int64_t*>(bank + m_loansCountOffset);
  if (idx >= count) return 0;
  uintptr_t data = *reinterpret_cast<uintptr_t*>(bank + m_loansPtrOffset);
  if (!data) return 0;
  return *reinterpret_cast<uintptr_t*>(data + static_cast<uintptr_t>(idx) * 8);
}

bool EconomyService::SetLoanRemaining(int32_t idx, int64_t value) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");
  uintptr_t loan = GetActiveLoanPtr(idx);
  if (!loan || m_loanRemainingOffset <= 0) {
    logger->Error("SetLoanRemaining({}, {}): loan or offset not ready.", idx, value);
    return false;
  }
  *reinterpret_cast<int64_t*>(loan + m_loanRemainingOffset) = value;
  logger->Info("SetLoanRemaining({}, {}).", idx, value);
  return true;
}

bool EconomyService::SetLoanAmount(int32_t idx, int64_t value) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");
  uintptr_t loan = GetActiveLoanPtr(idx);
  if (!loan || m_loanAmountOffset <= 0) {
    logger->Error("SetLoanAmount({}, {}): loan or offset not ready.", idx, value);
    return false;
  }
  *reinterpret_cast<int64_t*>(loan + m_loanAmountOffset) = value;
  logger->Info("SetLoanAmount({}, {}).", idx, value);
  return true;
}

bool EconomyService::SetLoanRate(int32_t idx, float value) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");
  uintptr_t loan = GetActiveLoanPtr(idx);
  if (!loan || m_loanRateOffset <= 0) {
    logger->Error("SetLoanRate({}, {}): loan or offset not ready.", idx, static_cast<double>(value));
    return false;
  }
  *reinterpret_cast<float*>(loan + m_loanRateOffset) = value;
  logger->Info("SetLoanRate({}, {}).", idx, static_cast<double>(value));
  return true;
}

bool EconomyService::SetLoanWeeks(int32_t idx, uint32_t value) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");
  uintptr_t loan = GetActiveLoanPtr(idx);
  if (!loan || m_loanWeeksOffset <= 0) {
    logger->Error("SetLoanWeeks({}, {}): loan or offset not ready.", idx, value);
    return false;
  }
  *reinterpret_cast<uint32_t*>(loan + m_loanWeeksOffset) = value;
  logger->Info("SetLoanWeeks({}, {}).", idx, value);
  return true;
}

bool EconomyService::SetLoanStartDay(int32_t idx, uint32_t value) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");
  uintptr_t loan = GetActiveLoanPtr(idx);
  if (!loan || m_loanStartDayOffset <= 0) {
    logger->Error("SetLoanStartDay({}, {}): loan or offset not ready.", idx, value);
    return false;
  }
  *reinterpret_cast<uint32_t*>(loan + m_loanStartDayOffset) = value;
  logger->Info("SetLoanStartDay({}, {}).", idx, value);
  return true;
}

// ============================================================================
// Bank config (SII reflection attributes)
// ============================================================================

uintptr_t EconomyService::GetBankObject() const {
  uintptr_t economyPtrAddr = ManagerCoreService::GetInstance().GetEconomyManagerAddr();
  if (!economyPtrAddr) return 0;
  uintptr_t economy = *(uintptr_t*)economyPtrAddr;
  if (!economy) return 0;
  return *(uintptr_t*)(economy + m_economyToBankOffset);
}

void EconomyService::LogBankConfigSnapshot() const {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyService");

  uintptr_t economyPtrAddr = ManagerCoreService::GetInstance().GetEconomyManagerAddr();
  if (!economyPtrAddr) {
    logger->Warn("Bank config snapshot: economy manager not ready.");
    return;
  }
  uintptr_t economy = *(uintptr_t*)economyPtrAddr;
  if (!economy) {
    logger->Warn("Bank config snapshot: economy object is null.");
    return;
  }
  uintptr_t bank = GetBankObject();
  if (!bank) {
    logger->Warn("Bank config snapshot: bank object is null.");
    return;
  }

  auto readI32 = [bank](int32_t off) -> int32_t { return off ? *reinterpret_cast<int32_t*>(bank + off) : 0; };
  auto readI64 = [bank](int32_t off) -> int64_t { return off ? *reinterpret_cast<int64_t*>(bank + off) : 0; };
  auto readU8 = [bank](int32_t off) -> uint32_t { return off ? *reinterpret_cast<uint8_t*>(bank + off) : 0; };
  auto readF32 = [bank](int32_t off) -> float { return off ? *reinterpret_cast<float*>(bank + off) : 0.0f; };

  logger->Info("Bank config snapshot: bank={:X}", bank);
  logger->Info("  bank_data: min_after_loan_repay(+'{:X}')={} overdraft_duration(+'{:X}')={} overdraft_warn_day(+'{:X}')={}",
               m_attrMinAfterLoanRepay,
               readI32(m_attrMinAfterLoanRepay),
               m_attrOverdraftDuration,
               readI32(m_attrOverdraftDuration),
               m_attrOverdraftWarnDay,
               readI32(m_attrOverdraftWarnDay));
  logger->Info("  bank_data: loan_offers(+'{:X}') insurance_data(+'{:X}')", m_attrLoanOffers, m_attrInsuranceData);
  logger->Info("  bank: money_account(+'{:X}')={} loan_limit(+'{:X}')={} app_enabled={}", m_attrMoneyAccount, readI64(m_attrMoneyAccount), m_attrLoanLimit, readI64(m_attrLoanLimit), readU8(m_attrAppEnabled));
  logger->Info("  bank: overdraft={} overdraft_timer={:.1f} overdraft_warn_count={} payment_timer={:.1f}", readI32(m_attrOverdraft), readF32(m_attrOverdraftTimer), readI32(m_attrOverdraftWarnCount), readF32(m_attrPaymentTimer));
  logger->Info("  bank: coinsurance_fixed={} coinsurance_ratio={:.4f} accident_severity={:.4f} loans(+'{:X}') sell_truck_later={} sell_trailer_later={}",
               readI64(m_attrCoinsuranceFixed),
               readF32(m_attrCoinsuranceRatio),
               readF32(m_attrAccidentSeverity),
               m_attrLoans,
               readU8(m_attrSellPlayersTruckLater),
               readU8(m_attrSellPlayersTrailerLater));
  logger->Info(
    "  bank_loan_data: amount(+'{:X}')={} interest_rate(+'{:X}')={:.4f} duration(+'{:X}')={}", m_attrLoanAmount, readI32(m_attrLoanAmount), m_attrLoanInterestRate, readF32(m_attrLoanInterestRate), m_attrLoanDuration, readI32(m_attrLoanDuration));
  logger->Info("  insurance_data: fixed base/min/max/inc/dec(+'{:X}'..)={}/{}/{}/{}/{} ratio base/min/max/inc/dec={:.4f}/{:.4f}/{:.4f}/{:.4f}/{:.4f}",
               m_attrCoinsuranceFixedBase,
               readI32(m_attrCoinsuranceFixedBase),
               readI32(m_attrCoinsuranceFixedMin),
               readI32(m_attrCoinsuranceFixedMax),
               readI32(m_attrCoinsuranceFixedInc),
               readI32(m_attrCoinsuranceFixedDec),
               readF32(m_attrCoinsuranceRatioBase),
               readF32(m_attrCoinsuranceRatioMin),
               readF32(m_attrCoinsuranceRatioMax),
               readF32(m_attrCoinsuranceRatioInc),
               readF32(m_attrCoinsuranceRatioDec));
}

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
