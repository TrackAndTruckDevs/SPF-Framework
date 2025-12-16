#pragma once

#include "SPF/Hooks/BaseHook.hpp" // Change from IHook.hpp
#include <string>

SPF_NS_BEGIN
namespace Hooks {
/**
 * @class GameLogHook
 * @brief A manageable hook for capturing game log messages.
 */
class GameLogHook : public BaseHook { // Inherit from BaseHook
 public:
  static GameLogHook& GetInstance();

  GameLogHook(const GameLogHook&) = delete;
  void operator=(const GameLogHook&) = delete;

 private:
  // Constructor now takes arguments for BaseHook
  GameLogHook(); // Default constructor for singleton

  // --- Pure Virtual Functions from BaseHook ---
  void* GetDetourFunc() override;
  void** GetOriginalFuncPtr() override;
};
}  // namespace Hooks
SPF_NS_END