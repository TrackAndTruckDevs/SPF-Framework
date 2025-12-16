#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <source_location>

#include <fmt/core.h>
#include <fmt/chrono.h>

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Logging {

/**
 * @brief Defines the logging verbosity levels.
 * Allows for flexible filtering of messages.
 */
enum class LogLevel {
  Trace,
  Debug,
  Info,
  Warn,
  Error,
  Critical,
  Unknown  // Added for validation
};

/**
 * @brief Converts a string to a LogLevel enum.
 * @param levelStr The string to convert (case-insensitive).
 * @return The corresponding LogLevel, or LogLevel::Unknown if not matched.
 */
LogLevel LogLevelFromString(const std::string& levelStr);

/**
 * @brief Attempts to parse a string into a LogLevel enum.
 * @param levelStr The string to parse (case-insensitive).
 * @param outLevel The output variable where the result is stored on success.
 * @return True if parsing was successful, false otherwise.
 */
bool TryParseLogLevel(const std::string& levelStr, LogLevel& outLevel);

/**
 * @brief Converts a LogLevel enum to its string representation.
 * @param level The enum value to convert.
 * @return A null-terminated string representing the log level.
 */
const char* LogLevelToString(LogLevel level);

/**
 * @brief Gets a list of all available log levels.
 * @return A constant reference to a vector of all LogLevel values.
 */
const std::vector<LogLevel>& GetAllLogLevels();

// Forward declaration to avoid circular dependencies
class ILogSink;

/**
 * @brief A struct that encapsulates all information about a single log message.
 * This allows the full log context to be passed to sinks.
 */
struct LogMessage {
  std::chrono::system_clock::time_point timestamp;  // Time the event occurred
  LogLevel level;                                   // Message level
  std::thread::id thread_id;                        // ID of the thread that sent the log
  fmt::string_view logger_name;                     // Name of the logger (e.g., "Core", "Renderer")
  fmt::memory_buffer formatted_message;             // The formatted message
};

/**
 * @brief Abstract base class (interface) for all log "sinks".
 * Any class that wants to receive and process log messages (e.g., write to a
 * file, output to the console) must inherit from this interface.
 */
class ILogSink {
 public:
  virtual ~ILogSink() = default;

  /**
   * @brief Gets the unique name of the sink.
   * @return A string view representing the sink's name.
   */
  virtual fmt::string_view GetName() const = 0;

  /**
   * @brief A pure virtual method called by the logger core.
   * @param msg The complete log message structure to be processed.
   */
  virtual void Log(const LogMessage& msg) = 0;

  /**
   * @brief Sets the formatting pattern for this sink.
   * @param pattern A string with placeholders ({timestamp}, {level}, {message}, etc.).
   */
  virtual void SetFormatter(std::string pattern) {
    // The base implementation stores the pattern.
    // Concrete sinks will use it for formatting.
    m_formatter_pattern = std::move(pattern);
  }

  /**
   * @brief Determines if this sink should be filtered by the logger's global level.
   * @return True by default. Sinks that want all messages should override this to return false.
   */
  virtual bool ShouldFilterByLevel() const { return true; }

 protected:
  std::string m_name;
  std::string m_formatter_pattern = "[{timestamp:%Y-%m-%d %H:%M:%S.%e}] [{level}] [{logger_name}] {message}";
};

/**
 * @brief The core logger class.
 * Responsible for receiving messages, formatting them, and dispatching them to sinks.
 * This class is thread-safe.
 */
class Logger {
 public:
  Logger(std::string name);
  ~Logger() = default;

  // Copying and moving are disallowed as loggers are managed via shared_ptr.
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  /**
   * @brief Sets the minimum logging level for this logger.
   * Messages with a lower level will be ignored.
   * This method is thread-safe.
   */
  void SetLevel(LogLevel level);

  /**
   * @brief Returns the current logging level.
   * This method is thread-safe.
   */
  LogLevel GetLevel() const;

  /**
   * @brief Returns the number of sinks attached to this logger.
   */
  size_t SinkCount() const;

  /**
   * @brief Returns the collection of sinks currently attached to the logger.
   */
  std::vector<std::shared_ptr<ILogSink>> GetSinks() const;

  /**
   * @brief Adds a new log sink to this logger.
   * @param sink A shared pointer to an ILogSink implementation.
   */
  void AddSink(std::shared_ptr<ILogSink> sink);

  /**
   * @brief Removes a log sink from this logger.
   * @param sink The shared pointer to the ILogSink implementation to remove.
   */
  void RemoveSink(const std::shared_ptr<ILogSink>& sink);

  /**
   * @brief Adds new sinks to the logger's existing list of sinks.
   * @param sinks A vector of spdlog sink pointers to add.
   */
  void AddSinks(const std::vector<std::shared_ptr<ILogSink>>& sinks);

  /**
   * @brief Replaces all sinks for this logger with a new set.
   * @param sinks The new vector of sinks.
   */
  void SetSinks(const std::vector<std::shared_ptr<ILogSink>>& sinks);

  /**
   * @brief Template method for logging a message with a specific level.
   * @tparam ...Args Argument types for formatting.
   * @param level The logging level.
   * @param format_str The {fmt} style format string.
   * @param ...args The arguments for formatting.
   */
  template <typename... Args>
  void Log(LogLevel level, fmt::string_view format_str, Args&&... args);

  /**
   * @brief Logs a message with pre-packed variadic arguments.
   * This is the bridge between C-style variadic functions (va_list) and
   * the C++ logging system.
   * @param level The logging level.
   * @param format_str The format string.
   * @param args The packed arguments for formatting.
   */
  void LogV(LogLevel level, fmt::string_view format_str, fmt::format_args args);

  // Convenience methods for each log level.
  template <typename... Args>
  void Trace(fmt::string_view format_str, Args&&... args);
  template <typename... Args>
  void Debug(fmt::string_view format_str, Args&&... args);
  template <typename... Args>
  void Info(fmt::string_view format_str, Args&&... args);
  template <typename... Args>
  void Warn(fmt::string_view format_str, Args&&... args);
  template <typename... Args>
  void Error(fmt::string_view format_str, Args&&... args);
  template <typename... Args>
  void Critical(fmt::string_view format_str, Args&&... args);

  // Throttling wrappers
  template <typename... Args>
  void TraceThrottled(std::chrono::nanoseconds duration, fmt::string_view format_str, Args&&... args);
  template <typename... Args>
  void DebugThrottled(std::chrono::nanoseconds duration, fmt::string_view format_str, Args&&... args);
  template <typename... Args>
  void InfoThrottled(std::chrono::nanoseconds duration, fmt::string_view format_str, Args&&... args);
  template <typename... Args>
  void WarnThrottled(std::chrono::nanoseconds duration, fmt::string_view format_str, Args&&... args);
  template <typename... Args>
  void ErrorThrottled(std::chrono::nanoseconds duration, fmt::string_view format_str, Args&&... args);
  template <typename... Args>
  void CriticalThrottled(std::chrono::nanoseconds duration, fmt::string_view format_str, Args&&... args);

  void LogThrottledManual(LogLevel level, const char* throttle_key, std::chrono::milliseconds duration, fmt::string_view message);

 private:
  /**
   * @brief Internal implementation of throttled logging.
   * @param location Information about the call site (filled in by the compiler).
   */
  template <typename... Args>
  void LogThrottledImpl(LogLevel level, std::chrono::nanoseconds throttle_duration, const std::source_location& location, fmt::string_view format_str, Args&&... args);

  std::string m_name;
  std::vector<std::shared_ptr<ILogSink>> m_sinks;
  mutable std::mutex m_mutex;
  std::atomic<LogLevel> m_level = LogLevel::Info;  // Default level

  // For throttling
  std::unordered_map<size_t, std::chrono::steady_clock::time_point> m_throttle_map;
  mutable std::mutex m_throttle_mutex;
};

// The implementation of template methods must be in the header file.
template <typename... Args>
void Logger::Log(LogLevel level, fmt::string_view format_str, Args&&... args) {
  // The log level is now checked per-sink, not globally at the start.

  // Formatting the message in the buffer
  fmt::memory_buffer buffer;
  fmt::vformat_to(std::back_inserter(buffer), format_str, fmt::make_format_args(args...));

  // Creating a message object
  LogMessage msg{
      .timestamp = std::chrono::system_clock::now(), .level = level, .thread_id = std::this_thread::get_id(), .logger_name = m_name, .formatted_message = std::move(buffer)};

  // We lock the mutex and send a message to all "sinks"
  std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto& sink : m_sinks) {
    if (sink->ShouldFilterByLevel() && msg.level < m_level.load(std::memory_order_relaxed)) {
      continue;  // Skip this sink if it filters and the level is too low
    }
    sink->Log(msg);
  }
}

template <typename... Args>
void Logger::LogThrottledImpl(LogLevel level, std::chrono::nanoseconds throttle_duration, const std::source_location& location, fmt::string_view format_str, Args&&... args) {
  // Generate a unique key for the call location
  size_t location_hash = std::hash<std::string>{}(location.file_name()) ^ (std::hash<int>{}(location.line()) << 1);

  {
    std::lock_guard<std::mutex> lock(m_throttle_mutex);
    auto now = std::chrono::steady_clock::now();
    auto it = m_throttle_map.find(location_hash);

    if (it != m_throttle_map.end()) {
      if (now - it->second < throttle_duration) {
        return;
      }
      it->second = now;
    } else {
      m_throttle_map[location_hash] = now;
    }
  }

  Log(level, format_str, std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Trace(fmt::string_view format_str, Args&&... args) {
  Log(LogLevel::Trace, format_str, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::Debug(fmt::string_view format_str, Args&&... args) {
  Log(LogLevel::Debug, format_str, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::Info(fmt::string_view format_str, Args&&... args) {
  Log(LogLevel::Info, format_str, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::Warn(fmt::string_view format_str, Args&&... args) {
  Log(LogLevel::Warn, format_str, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::Error(fmt::string_view format_str, Args&&... args) {
  Log(LogLevel::Error, format_str, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::Critical(fmt::string_view format_str, Args&&... args) {
  Log(LogLevel::Critical, format_str, std::forward<Args>(args)...);
}

// Implementing wrappers for throttling
template <typename... Args>
void Logger::TraceThrottled(std::chrono::nanoseconds duration, fmt::string_view format_str, Args&&... args) {
  LogThrottledImpl(LogLevel::Trace, duration, std::source_location::current(), format_str, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::DebugThrottled(std::chrono::nanoseconds duration, fmt::string_view format_str, Args&&... args) {
  LogThrottledImpl(LogLevel::Debug, duration, std::source_location::current(), format_str, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::InfoThrottled(std::chrono::nanoseconds duration, fmt::string_view format_str, Args&&... args) {
  LogThrottledImpl(LogLevel::Info, duration, std::source_location::current(), format_str, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::WarnThrottled(std::chrono::nanoseconds duration, fmt::string_view format_str, Args&&... args) {
  LogThrottledImpl(LogLevel::Warn, duration, std::source_location::current(), format_str, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::ErrorThrottled(std::chrono::nanoseconds duration, fmt::string_view format_str, Args&&... args) {
  LogThrottledImpl(LogLevel::Error, duration, std::source_location::current(), format_str, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::CriticalThrottled(std::chrono::nanoseconds duration, fmt::string_view format_str, Args&&... args) {
  LogThrottledImpl(LogLevel::Critical, duration, std::source_location::current(), format_str, std::forward<Args>(args)...);
}

}  // namespace Logging

SPF_NS_END
