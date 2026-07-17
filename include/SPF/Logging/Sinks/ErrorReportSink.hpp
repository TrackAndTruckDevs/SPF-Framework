#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Logging/Logger.hpp"

#include "fmt/base.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>


SPF_NS_BEGIN

namespace Logging::Sinks {

/**
 * @brief Represents a single report entry for an error or warning.
 */
struct LogReportEntry {
  std::string loggerName;
  LogLevel level;
  std::string message;
  uint32_t count = 0;

  bool operator==(const LogReportEntry& other) const { return loggerName == other.loggerName && level == other.level && message == other.message; }
};

/**
 * @brief Hash function for LogReportEntry to allow grouping in a map.
 */
struct LogReportEntryHash {
  std::size_t operator()(const LogReportEntry& e) const {
    return std::hash<std::string>{}(e.loggerName) ^ (std::hash<int>{}(static_cast<int>(e.level)) << 1) ^ (std::hash<std::string>{}(e.message) << 2);
  }
};

/**
 * @brief A log sink that captures and groups WARN/ERROR messages for remote reporting.
 */
class ErrorReportSink : public ILogSink {
 public:
  ErrorReportSink();
  virtual ~ErrorReportSink() = default;

  // --- ILogSink Implementation ---
  fmt::string_view GetName() const override { return "ErrorReportSink"; }
  void Log(const LogMessage& msg) override;
  bool ShouldFilterByLevel() const override { return false; }  // We handle filtering internally

  /**
   * @brief Retrieves all pending logs and clears the internal buffer.
   * @return A vector of grouped log entries.
   */
  std::vector<LogReportEntry> GetAndClearPendingLogs();

  /**
   * @brief Checks if there are any logs waiting to be sent.
   */
  bool HasPendingLogs() const;

 private:
  mutable std::mutex m_mutex;
  std::unordered_map<LogReportEntry, uint32_t, LogReportEntryHash> m_pendingLogs;
};

}  // namespace Logging::Sinks

SPF_NS_END
