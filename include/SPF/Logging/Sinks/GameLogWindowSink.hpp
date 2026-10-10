#pragma once

#include "SPF/Logging/Logger.hpp"

#include <mutex>
#include <string>
#include <vector>

namespace SPF::Logging::Sinks {
/**
 * @brief A thread-safe sink that collects game log messages (intercepted via GameLogHook)
 * in memory for UI rendering. Separate from LoggerWindowSink so framework and game
 * logs are not mixed.
 */
class GameLogWindowSink {
 public:
  struct DisplayMessage {
    LogLevel level;
    std::string logger_name;
    std::string message;
  };

  static GameLogWindowSink& GetInstance();

  GameLogWindowSink(const GameLogWindowSink&) = delete;
  GameLogWindowSink& operator=(const GameLogWindowSink&) = delete;

  /**
   * @brief Adds a raw game log message to the internal buffer (drops oldest when the cap is reached).
   * The game's level markers (<ERROR>/<WARNING>) and [module] tags are parsed here.
   */
  void Push(const std::string& raw_message);

  /**
   * @brief Provides thread-safe access to the collected messages.
   * @return A copy of the current message buffer.
   */
  std::vector<DisplayMessage> GetMessages() const;

  /**
   * @brief Clears all stored messages.
   */
  void Clear();

 private:
  GameLogWindowSink() = default;

  static constexpr size_t kMaxMessages = 5000;
  static constexpr size_t kPruneBatch = 500;

  mutable std::mutex m_mutex;
  std::vector<DisplayMessage> m_items;
};
}  // namespace SPF::Logging::Sinks
