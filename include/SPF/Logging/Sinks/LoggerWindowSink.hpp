#pragma once

#include "SPF/Logging/Logger.hpp"  // Correct include for ILogSink
#include "SPF/Namespace.hpp"

#include <vector>
#include <mutex>
#include <string>

SPF_NS_BEGIN
namespace Logging::Sinks {
/**
 * @brief A sink that collects log messages in memory for UI rendering.
 * This class is thread-safe.
 */
class LoggerWindowSink : public ILogSink  // Use the fully qualified name
{
 public:
  // We store a simplified version of the message for display purposes.
  struct DisplayMessage {
    LogLevel level;
    std::string logger_name;
    std::string message;
  };

  LoggerWindowSink();
  ~LoggerWindowSink() override = default;

  LoggerWindowSink(const LoggerWindowSink&) = delete;
  LoggerWindowSink& operator=(const LoggerWindowSink&) = delete;

  /**
   * @brief Gets the unique name of the sink.
   */
  fmt::string_view GetName() const override;

  /**
   * @brief Adds a message to the internal buffer.
   * @param msg The full log message structure.
   *
   * @param msg The message to log.
   */
  void Log(const LogMessage& msg) override;

  // --- ILogSink Overrides ---
  bool ShouldFilterByLevel() const override { return false; }

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
  mutable std::mutex m_mutex;
  std::vector<DisplayMessage> m_items;
};
}  // namespace Logging::Sinks
SPF_NS_END