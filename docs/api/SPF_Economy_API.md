# SPF Economy API

The SPF Economy API provides access to the game's banking system: read the player's bank balance and debt flag, add or subtract money, set the balance to an exact amount, block game-initiated money operations (deposit/withdraw), and work with loans — read active loans and offers, take new loans (bank offers or custom ones), repay, edit loan fields, and adjust the credit limit.

Money is never written into game memory directly. `ECO_AddMoney` / `ECO_SetMoney` call the game's own deposit/withdraw functions, so the game handles everything itself — debt flags, timers, mail notifications. The framework only decides how much and in which direction.

## Getting the API

Access the Economy API during your plugin's `OnActivated` callback via the core API.

**Example: OnActivated callback (Recommended)**
```cpp
SPF_Economy_API* s_economyAPI = NULL;

void OnActivated(const SPF_Core_API* core_api) {
    s_economyAPI = core_api->economy;
}
```

## Key Concepts

1. **World-Scoped Lifecycle**: The economy system initializes when the game world loads. Always check `ECO_IsReady()` before using any other function. After loading into the game, call `ECO_RefreshOffsets()` if the service was not ready at startup.
2. **Game-Handled Money Semantics**: Positive deltas are processed by the game's bank deposit function, negative deltas by its bank withdraw function. The game itself updates the balance, debt flag and related game state — the framework never patches these fields directly.
3. **Game Control Lock**: `ECO_SetGameControl(true)` detours the game's deposit/withdraw entry points. Every call coming from the game is blocked (returns `0` and is logged). Framework `ECO_AddMoney` / `ECO_SetMoney` calls bypass the lock through a service-local flag — the lock never blocks the plugin.
4. **Int64 Amounts**: All monetary values are `int64_t` in game currency units. `ECO_SetMoney` computes the delta from the current balance internally; setting the balance to its current value is a no-op that returns `true`.
5. **Game Thread**: Money functions execute game banking logic (mail context, sound). Call them from the game thread.
6. **Loan Indexes**: Loan and offer indexes are 0-based and can shift after take/repay (the active loans array reindexes). Re-read counts and indexes before every operation.
7. **Two Loan Creation Paths**: `ECO_TakeOffer(idx)` follows the game's own offer-accept branch. `ECO_TakeLoan(amount, rate, weeks)` builds a custom loan through the game's factory (vtable/refcount handled by the game's object pool) — both deposit money through the game's bank function.
8. **Direct Loan Editing**: `ECO_SetLoan*` functions write verified structure fields directly (no validation by the game). Repay and take always go through game functions — use them for lifecycle changes.
9. **Rates Are Fractions**: Interest rate is an annual fraction, not a percentage — `0.05` means 5% per year (the game divides by 52 for weekly payments; values ≤ `0.001` mean zero interest). Pass `0.035`, not `3.5`, to `ECO_TakeLoan` / `ECO_SetLoanRate`.

## Usage Example

```c
if (!s_economyAPI || !s_economyAPI->ECO_IsReady()) return;

// Read balance and debt state
int64_t balance = s_economyAPI->ECO_GetBalance();
bool inDebt = s_economyAPI->ECO_GetDebtFlag();
printf("Balance: %lld, debt: %s\n", (long long)balance, inDebt ? "YES" : "NO");

// Add and subtract money
s_economyAPI->ECO_AddMoney(5000);      // deposit
s_economyAPI->ECO_AddMoney(-1500);     // withdraw

// Set exact balance
s_economyAPI->ECO_SetMoney(1000000);

// Block all game-initiated money operations (framework calls still pass)
s_economyAPI->ECO_SetGameControl(true);

// ... later, restore normal game control:
s_economyAPI->ECO_SetGameControl(false);
```

### Loan Example

```c
if (!s_economyAPI || !s_economyAPI->ECO_IsReady()) return;

// Read active loans
int32_t count = s_economyAPI->ECO_GetActiveLoanCount();
for (int32_t i = 0; i < count; ++i) {
    SPF_ECO_ActiveLoanInfo loan;
    if (s_economyAPI->ECO_GetActiveLoan(i, &loan)) {
        printf("loan %d: remaining=%lld rate=%.2f%% weeks=%u\n",
               i, (long long)loan.remaining, loan.rate * 100.0f, loan.weeks);
    }
}

// Read bank offers and take the first one
int32_t offers = s_economyAPI->ECO_GetOfferCount();
if (offers > 0) s_economyAPI->ECO_TakeOffer(0);

// Or create a custom loan: 50,000 at 3.5% per year for 12 weeks (rate = annual fraction!)
s_economyAPI->ECO_TakeLoan(50000, 0.035f, 12);

// Edit a loan field directly (loan 0): zero interest rate
s_economyAPI->ECO_SetLoanRate(0, 0.0f);

// Repay loan 0 (game validates: permission + repayment rules)
if (s_economyAPI->ECO_CanRepayLoan(0)) s_economyAPI->ECO_RepayLoan(0);
```

## Function Reference

### Service Lifecycle

| Function | Return Type | Description |
|---|---|---|
| **`ECO_IsReady()`** | `bool` | Checks if the economy service is initialized (deposit, withdraw, and transaction entry points resolved plus economy global present). Always call first. |
| **`ECO_AreAllOffsetsFound()`** | `bool` | Checks if all economy memory patterns (bank entry points and structure offsets) were resolved. Stricter than `ECO_IsReady`. |
| **`ECO_RefreshOffsets()`** | `bool` | Forces a rescan of economy memory patterns. Useful if the service was not ready when the plugin activated. |

### Balance

| Function | Return Type | Description |
|---|---|---|
| **`ECO_GetBalance()`** | `int64_t` | Returns the player's current bank balance. Returns `0` if the service is not ready. |
| **`ECO_GetDebtFlag()`** | `bool` | Returns the bank's debt flag state (diagnostic). Returns `false` if the service is not ready. |

### Money Operations

| Function | Return Type | Description |
|---|---|---|
| **`ECO_AddMoney(delta)`** | `bool` | Adds `delta` to the balance: positive → game deposit path, negative → game withdraw path. Processed by the game (debt flag, mail, sound). Returns `false` if the service is not ready or the amount is zero. |
| **`ECO_SetMoney(amount)`** | `bool` | Sets the balance to `amount` by computing `amount - current balance` and applying it through the same deposit/withdraw path. Returns `true` as a no-op if the balance already equals `amount`. |

### Game Control

| Function | Return Type | Description |
|---|---|---|
| **`ECO_SetGameControl(lock)`** | `bool` | `true`: all game-initiated deposit/withdraw calls are blocked (return `0`, logged). Framework `ECO_AddMoney` / `ECO_SetMoney` always pass. `false`: the game manages money as before. Returns `false` if the service is not ready. |
| **`ECO_GetGameControlLocked()`** | `bool` | Returns whether the game control lock is currently active. |

### Credit Limit

| Function | Return Type | Description |
|---|---|---|
| **`ECO_GetCreditLimit()`** | `int64_t` | Returns the credit limit the game enforces on loan amounts. `0` if not ready. |
| **`ECO_GetLoanLimit()`** | `int64_t` | Returns the `loan_limit` config value. `0` if not ready. |
| **`ECO_SetCreditLimit(limit)`** | `bool` | Overwrites the credit limit config value. `false` if not ready. |

### Active Loans

| Function | Return Type | Description |
|---|---|---|
| **`ECO_GetActiveLoanCount()`** | `int32_t` | Number of active loans. `0` if not ready. |
| **`ECO_GetActiveLoan(idx, out)`** | `bool` | Fills `SPF_ECO_ActiveLoanInfo` for loan `idx`. `false` if not ready, index invalid, or `out` is NULL. |

`SPF_ECO_ActiveLoanInfo`: `remaining` (int64, debt), `amount` (int64, initial), `startDay` (uint32), `rate` (float, annual fraction — `0.05` = 5%), `weeks` (uint32).

### Loan Offers

| Function | Return Type | Description |
|---|---|---|
| **`ECO_GetOfferCount()`** | `int32_t` | Number of bank loan offers. `0` if not ready. |
| **`ECO_GetOffer(idx, out)`** | `bool` | Fills `SPF_ECO_LoanOfferInfo` for offer `idx`. `false` if not ready, index invalid, or `out` is NULL. |

`SPF_ECO_LoanOfferInfo`: `amount` (uint32), `rate` (float, annual fraction — `0.05` = 5%), `weeks` (uint32).

### Loan Take / Repay

| Function | Return Type | Description |
|---|---|---|
| **`ECO_CanRepayLoan(idx)`** | `bool` | Returns whether the game would allow repaying loan `idx` (permission + repayment rules). |
| **`ECO_RepayLoan(idx)`** | `bool` | Repays loan `idx` through the game's withdraw + remove path. `false` if not ready or the game rejects it. |
| **`ECO_TakeLoan(amount, rate, weeks)`** | `bool` | Creates a custom loan via the game's factory and deposits `amount` to the bank. Handles refcount and array placement. `false` if not ready or guards fail. |
| **`ECO_TakeOffer(idx)`** | `bool` | Accepts bank offer `idx` through the game's own take branch (credit limit + permission checks done by the game). |

### Loan Field Editing

| Function | Return Type | Description |
|---|---|---|
| **`ECO_SetLoanRemaining(idx, value)`** | `bool` | Direct write: remaining debt of loan `idx`. |
| **`ECO_SetLoanAmount(idx, value)`** | `bool` | Direct write: initial amount of loan `idx`. |
| **`ECO_SetLoanRate(idx, value)`** | `bool` | Direct write: annual rate as fraction (`0.05` = 5%) of loan `idx`. |
| **`ECO_SetLoanWeeks(idx, value)`** | `bool` | Direct write: term in weeks of loan `idx`. |
| **`ECO_SetLoanStartDay(idx, value)`** | `bool` | Direct write: start game day of loan `idx`. |

All setters return `false` if the service is not ready or `idx` is invalid. Direct writes bypass game validation — use them for editing, not for lifecycle changes.

---

## ABI Stability

To ensure compatibility with future framework versions without recompilation:
1. The order of existing function pointers will NEVER change.
2. Fields will NEVER be removed from this structure.
3. New functionality is only added by appending to the END of this structure.
