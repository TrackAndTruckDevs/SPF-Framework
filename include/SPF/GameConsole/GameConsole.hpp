#pragma once

#include "SPF/Hooks/IHook.hpp"
#include <string>

namespace SPF {
/**
 * @class GameConsole
 * @brief A manageable service for executing in-game console commands.
 *
 * @note HOW THE GAME'S CONSOLE WORKS (based on reverse engineering for game version 1.XX):
 * The game's command system is a two-step, queue-based process. Directly executing commands can
 * cause deadlocks or crashes, especially for commands that change major game states (e.g., 'exit').
 *
 * 1. THE ENQUEUER/DISPATCHER FUNCTION (hooked by this class):
 *    - Signature: bool(const char** command, int queue_id)
 *    - This is the primary "front door" for all command submissions.
 *    - It takes a command and a queue ID. Its behavior depends on the ID:
 *      - ID -1: Bypasses the queue system entirely and calls the low-level synchronous executor.
 *                THIS IS DANGEROUS and was the source of our hangs.
 *      - ID 1:  This appears to be the main queue for console commands. It safely adds the
 *               command to a queue to be processed by the game at a safe point in its main loop.
 *               THIS IS THE CORRECT ID TO USE.
 *      - ID 2:  Used by other game systems.
 *      - Other IDs might exist or be invalid. Using an unknown ID results in a game log error.
 *
 * 2. THE QUEUE PROCESSOR FUNCTION:
 *    - A separate function exists that the game calls from its main loop. Its job is to
 *      iterate through a specific queue (identified by its ID) and execute any pending commands
 *      using the low-level executor. We do not call this function.
 *
 * 3. THE LOW-LEVEL EXECUTOR FUNCTION:
 *    - This is a synchronous-only function that finds a command in a hash map and executes it
 *      immediately. This is the function we hooked initially, and calling it directly is unsafe.
 *
 * Our implementation hooks the ENQUEUER function and submits our commands to queue 1.
 */
class GameConsole : public Hooks::IHook {
 public:
  static GameConsole& GetInstance();

  GameConsole(const GameConsole&) = delete;
  void operator=(const GameConsole&) = delete;

  /**
   * @brief Executes a console command.
   * @param command The command to execute.
   */
  void Execute(const std::string& command);

  // --- IHook Implementation ---
  const std::string& GetName() const override { return m_name; }
  const std::string& GetDisplayName() const override { return m_displayName; }
  const std::string& GetOwnerName() const override { return m_ownerName; }
  bool IsEnabled() const override { return m_isEnabled; }
  void SetEnabled(bool enabled) override { m_isEnabled = enabled; }
  const std::string& GetSignature() const override { return m_signature; }
  bool IsInstalled() const override { return m_hookedAddress != 0; }

  // In this class, "Install" means finding the game function.
  bool Install() override;
  // "Uninstall" means clearing the function pointer.
  void Uninstall() override;
  void Remove() override;

 private:
  GameConsole() = default;
  ~GameConsole() = default;

  using ExecuteCommandFn = bool (*)(const char**, int);

  // --- Hook Configuration ---
  std::string m_ownerName = "framework";
  std::string m_name = "GameConsole";
  std::string m_displayName = "Game Console";  // Will be localized later
  bool m_isEnabled = true;
  std::string m_signature = "40 53 56 57 41 56 41 57 48 81 EC ? ? ? ? 45 33 F6";

  // --- Runtime State ---
  uintptr_t m_hookedAddress = 0;
  ExecuteCommandFn m_ExecuteGameCommand = nullptr;
};
}  // namespace SPF