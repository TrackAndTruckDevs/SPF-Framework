/**
 * @file SPF_Economy_API.h
 * @brief API for reading and controlling the game's money/bank economy.
 *
 * @details This API provides plugins with access to the player's bank balance and
 *          money operations. AddMoney/SetMoney go through the game's own
 *          ProcessBankDeposit/ProcessBankWithdrawal functions, so the game handles
 *          debt flags and mail notifications itself.
 *
 *          SetGameControl(true) blocks all money operations initiated by the game
 *          (deposit/withdraw hooks return 0 and log). Framework AddMoney/SetMoney
 *          calls always pass through the lock.
 *
 * ================================================================================================
 * USAGE EXAMPLE (C++)
 * ================================================================================================
 * @code
 * void MyPlugin_OnActivated(const SPF_Core_API* api) {
 *     if (!api->economy->ECO_IsReady()) return;
 *
 *     int64_t balance = api->economy->ECO_GetBalance();
 *     api->economy->ECO_AddMoney(1000);
 *     api->economy->ECO_SetGameControl(true);   // block game money ops
 * }
 * @endcode
 *
 * ================================================================================================
 * ABI RULE
 * ================================================================================================
 * New function pointers are only APPENDED to the end of the SPF_Economy_API structure.
 * Never reorder or remove existing entries — plugins compiled against older headers
 * must keep working.
 *
 * ================================================================================================
 * THREAD SAFETY
 * ================================================================================================
 * All functions must be called from the game thread. Calling from other threads may
 * crash due to unsynchronized access to game memory.
 */

#ifndef SPF_ECONOMY_API_H
#define SPF_ECONOMY_API_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
// 1. FUNCTION POINTER TYPES
// =================================================================================================

/** @brief Returns true if the economy service is fully initialized and all offsets are resolved. */
typedef bool (*SPF_ECO_IsReady_t)(void);

/** @brief Returns true if all economy function addresses and offsets are resolved. */
typedef bool (*SPF_ECO_AreAllOffsetsFound_t)(void);

/** @brief Re-runs pattern discovery for economy functions/offsets. Returns true on success. */
typedef bool (*SPF_ECO_RefreshOffsets_t)(void);

/** @brief Reads the current bank balance. Returns 0 if not ready. */
typedef int64_t (*SPF_ECO_GetBalance_t)(void);

/** @brief Reads the bank debt flag (true = in debt). Returns false if not ready. */
typedef bool (*SPF_ECO_GetDebtFlag_t)(void);

/**
 * @brief Adds (delta > 0) or subtracts (delta < 0) money via game's ProcessBankDeposit/Withdrawal.
 * @param delta Signed amount to change the balance by.
 * @return true on success, false on failure (not ready, invalid amount, game rejected the call).
 */
typedef bool (*SPF_ECO_AddMoney_t)(int64_t delta);

/**
 * @brief Sets the bank balance to an absolute amount (delta = amount - current balance).
 * @param amount Desired absolute balance.
 * @return true on success, false on failure.
 */
typedef bool (*SPF_ECO_SetMoney_t)(int64_t amount);

/**
 * @brief Blocks (lock=true) or unblocks (lock=false) game-initiated money operations.
 * @param lock true = block game deposit/withdraw, false = let game operate normally.
 * @return true on success, false if service not ready.
 */
typedef bool (*SPF_ECO_SetGameControl_t)(bool lock);

/** @brief Returns true if game money operations are currently blocked. */
typedef bool (*SPF_ECO_GetGameControlLocked_t)(void);

/** @brief Snapshot of one active loan. */
typedef struct SPF_ECO_ActiveLoanInfo {
  int64_t remaining;  /**< Remaining debt. */
  int64_t amount;     /**< Initial loan amount. */
  uint32_t startDay;  /**< Game day the loan started. */
  float rate;         /**< Annual rate as fraction (0.05 = 5%). */
  uint32_t weeks;     /**< Loan term in weeks. */
} SPF_ECO_ActiveLoanInfo;

/** @brief Snapshot of one bank loan offer. */
typedef struct SPF_ECO_LoanOfferInfo {
  uint32_t amount; /**< Offered amount. */
  float rate;      /**< Annual rate as fraction (0.05 = 5%). */
  uint32_t weeks;  /**< Offered term in weeks. */
} SPF_ECO_LoanOfferInfo;

/** @brief Returns the credit limit the game enforces on loan amounts. Returns 0 if not ready. */
typedef int64_t (*SPF_ECO_GetCreditLimit_t)(void);

/** @brief Returns the loan limit config value (bank_data::loan_limit). Returns 0 if not ready. */
typedef int64_t (*SPF_ECO_GetLoanLimit_t)(void);

/** @brief Overwrites the credit limit config value. Returns false if not ready. */
typedef bool (*SPF_ECO_SetCreditLimit_t)(int64_t limit);

/** @brief Returns the number of active loans. Returns 0 if not ready. */
typedef int32_t (*SPF_ECO_GetActiveLoanCount_t)(void);

/** @brief Reads active loan at idx into out. Returns false if not ready or idx invalid. */
typedef bool (*SPF_ECO_GetActiveLoan_t)(int32_t idx, SPF_ECO_ActiveLoanInfo* out);

/** @brief Returns the number of loan offers. Returns 0 if not ready. */
typedef int32_t (*SPF_ECO_GetOfferCount_t)(void);

/** @brief Reads loan offer at idx into out. Returns false if not ready or idx invalid. */
typedef bool (*SPF_ECO_GetOffer_t)(int32_t idx, SPF_ECO_LoanOfferInfo* out);

/** @brief Returns true if the game would allow repaying the loan at idx. */
typedef bool (*SPF_ECO_CanRepayLoan_t)(int32_t idx);

/** @brief Repays and removes the loan at idx via the game's own withdraw/remove path. */
typedef bool (*SPF_ECO_RepayLoan_t)(int32_t idx);

/** @brief Creates a new custom loan (factory path) and deposits the amount. rate: annual fraction (0.05 = 5%). */
typedef bool (*SPF_ECO_TakeLoan_t)(uint64_t amount, float rate, uint32_t weeks);

/** @brief Takes the bank's loan offer at idx (game's take branch). */
typedef bool (*SPF_ECO_TakeOffer_t)(int32_t idx);

/** @brief Sets remaining debt of the loan at idx (direct write). */
typedef bool (*SPF_ECO_SetLoanRemaining_t)(int32_t idx, int64_t value);

/** @brief Sets initial amount of the loan at idx (direct write). */
typedef bool (*SPF_ECO_SetLoanAmount_t)(int32_t idx, int64_t value);

/** @brief Sets annual rate as fraction (0.05 = 5%) of the loan at idx (direct write). */
typedef bool (*SPF_ECO_SetLoanRate_t)(int32_t idx, float value);

/** @brief Sets term in weeks of the loan at idx (direct write). */
typedef bool (*SPF_ECO_SetLoanWeeks_t)(int32_t idx, uint32_t value);

/** @brief Sets start day of the loan at idx (direct write). */
typedef bool (*SPF_ECO_SetLoanStartDay_t)(int32_t idx, uint32_t value);

// =================================================================================================
// 2. API STRUCTURE
// =================================================================================================

/**
 * @struct SPF_Economy_API
 * @brief Economy/money API for plugins.
 *
 * @details A pointer to this structure is available in the SPF_Core_API::economy field
 *          after OnActivated().
 *
 *          All functions require ECO_IsReady() to return true. Using functions before
 *          the service is ready will return safe default values (0 / false).
 *
 * **ABI Rule**: New function pointers are only appended to the end of this structure.
 */
typedef struct SPF_Economy_API {
  /** @brief Service lifecycle. @{ */
  SPF_ECO_IsReady_t ECO_IsReady;
  SPF_ECO_AreAllOffsetsFound_t ECO_AreAllOffsetsFound;
  SPF_ECO_RefreshOffsets_t ECO_RefreshOffsets;
  /** @} */

  /** @brief Balance read. @{ */
  SPF_ECO_GetBalance_t ECO_GetBalance;
  SPF_ECO_GetDebtFlag_t ECO_GetDebtFlag;
  /** @} */

  /** @brief Money operations (via game's own deposit/withdraw). @{ */
  SPF_ECO_AddMoney_t ECO_AddMoney;
  SPF_ECO_SetMoney_t ECO_SetMoney;
  /** @} */

  /** @brief Game money-operation lock. @{ */
  SPF_ECO_SetGameControl_t ECO_SetGameControl;
  SPF_ECO_GetGameControlLocked_t ECO_GetGameControlLocked;
  /** @} */

  /** @brief Credit limit. @{ */
  SPF_ECO_GetCreditLimit_t ECO_GetCreditLimit;
  SPF_ECO_GetLoanLimit_t ECO_GetLoanLimit;
  SPF_ECO_SetCreditLimit_t ECO_SetCreditLimit;
  /** @} */

  /** @brief Active loans read. @{ */
  SPF_ECO_GetActiveLoanCount_t ECO_GetActiveLoanCount;
  SPF_ECO_GetActiveLoan_t ECO_GetActiveLoan;
  /** @} */

  /** @brief Loan offers read. @{ */
  SPF_ECO_GetOfferCount_t ECO_GetOfferCount;
  SPF_ECO_GetOffer_t ECO_GetOffer;
  /** @} */

  /** @brief Loan take/repay (game paths). @{ */
  SPF_ECO_CanRepayLoan_t ECO_CanRepayLoan;
  SPF_ECO_RepayLoan_t ECO_RepayLoan;
  SPF_ECO_TakeLoan_t ECO_TakeLoan;
  SPF_ECO_TakeOffer_t ECO_TakeOffer;
  /** @} */

  /** @brief Loan field editing (direct writes). @{ */
  SPF_ECO_SetLoanRemaining_t ECO_SetLoanRemaining;
  SPF_ECO_SetLoanAmount_t ECO_SetLoanAmount;
  SPF_ECO_SetLoanRate_t ECO_SetLoanRate;
  SPF_ECO_SetLoanWeeks_t ECO_SetLoanWeeks;
  SPF_ECO_SetLoanStartDay_t ECO_SetLoanStartDay;
  /** @} */
} SPF_Economy_API;

#ifdef __cplusplus
}
#endif

#endif  // SPF_ECONOMY_API_H
