# SPF Economy API

The SPF Economy API provides access to the game's banking system: read the player's bank balance and debt flag, add or subtract money, set the balance to an exact amount, and block game-initiated money operations (deposit/withdraw) while always letting framework calls through.

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

---

## ABI Stability

To ensure compatibility with future framework versions without recompilation:
1. The order of existing function pointers will NEVER change.
2. Fields will NEVER be removed from this structure.
3. New functionality is only added by appending to the END of this structure.
