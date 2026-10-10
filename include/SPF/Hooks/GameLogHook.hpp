#pragma once

#include "SPF/Hooks/BaseHook.hpp"  // Change from IHook.hpp

#include <atomic>

namespace SPF::Hooks {
/**
 * @class GameLogHook
 * @brief A manageable hook for capturing game log messages.
 */
class GameLogHook : public BaseHook {  // Inherit from BaseHook
 public:
  static GameLogHook& GetInstance();

  GameLogHook(const GameLogHook&) = delete;
  void operator=(const GameLogHook&) = delete;

  /// @brief Whether captured game messages are also written to the framework "Game" logger (framework.log).
  static bool IsLogToFrameworkEnabled() { return s_logToFramework.load(std::memory_order_relaxed); }
  static void SetLogToFrameworkEnabled(bool enabled) { s_logToFramework.store(enabled, std::memory_order_relaxed); }

 private:
  // Constructor now takes arguments for BaseHook
  GameLogHook();  // Default constructor for singleton

  static std::atomic<bool> s_logToFramework;

  // --- Pure Virtual Functions from BaseHook ---
  void* GetDetourFunc() override;
  void** GetOriginalFuncPtr() override;
};
}  // namespace SPF::Hooks
