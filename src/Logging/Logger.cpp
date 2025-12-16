#include <SPF/Logging/Logger.hpp>
#include <algorithm>
#include <cctype>

SPF_NS_BEGIN

namespace Logging {

bool TryParseLogLevel(const std::string& levelStr, LogLevel& outLevel) {
  std::string lowerLevelStr = levelStr;
  std::transform(lowerLevelStr.begin(), lowerLevelStr.end(), lowerLevelStr.begin(), [](unsigned char c) { return std::tolower(c); });

  if (lowerLevelStr == "trace") {
    outLevel = LogLevel::Trace;
    return true;
  }
  if (lowerLevelStr == "debug") {
    outLevel = LogLevel::Debug;
    return true;
  }
  if (lowerLevelStr == "info") {
    outLevel = LogLevel::Info;
    return true;
  }
  if (lowerLevelStr == "warn") {
    outLevel = LogLevel::Warn;
    return true;
  }
  if (lowerLevelStr == "error") {
    outLevel = LogLevel::Error;
    return true;
  }
  if (lowerLevelStr == "critical") {
    outLevel = LogLevel::Critical;
    return true;
  }

  return false;
}

LogLevel LogLevelFromString(const std::string& levelStr) {
  LogLevel level;
  if (TryParseLogLevel(levelStr, level)) {
    return level;
  }
  return LogLevel::Unknown;
}

const char* LogLevelToString(LogLevel level) {
  switch (level) {
    case LogLevel::Trace:
      return "TRACE";
    case LogLevel::Debug:
      return "DEBUG";
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Warn:
      return "WARN";
    case LogLevel::Error:
      return "ERROR";
    case LogLevel::Critical:
      return "CRITICAL";
    default:
      return "UNKNOWN";
  }
}

const std::vector<LogLevel>& GetAllLogLevels() {
  static const std::vector<LogLevel> allLevels = {LogLevel::Trace, LogLevel::Debug, LogLevel::Info, LogLevel::Warn, LogLevel::Error, LogLevel::Critical};
  return allLevels;
}

Logger::Logger(std::string name) : m_name(std::move(name)) {}

std::vector<std::shared_ptr<ILogSink>> Logger::GetSinks() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_sinks;
}

void Logger::AddSink(std::shared_ptr<ILogSink> sink) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_sinks.push_back(std::move(sink));
}

void Logger::RemoveSink(const std::shared_ptr<ILogSink>& sink) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_sinks.erase(std::remove(m_sinks.begin(), m_sinks.end(), sink), m_sinks.end());
}

void Logger::AddSinks(const std::vector<std::shared_ptr<ILogSink>>& newSinks) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_sinks.insert(m_sinks.end(), newSinks.begin(), newSinks.end());
}

void Logger::SetSinks(const std::vector<std::shared_ptr<ILogSink>>& sinks) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_sinks = sinks;
}

void Logger::SetLevel(LogLevel level) { m_level.store(level, std::memory_order_relaxed); }

LogLevel Logger::GetLevel() const { return m_level.load(std::memory_order_relaxed); }

size_t Logger::SinkCount() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_sinks.size();
}

void Logger::LogV(LogLevel level, fmt::string_view format_str, fmt::format_args args) {
  // The log level is now checked per-sink, not globally at the start.

  // Format the message into a buffer using the pre-packed arguments
  fmt::memory_buffer buffer;
  fmt::vformat_to(std::back_inserter(buffer), format_str, args);

  // Create the message object
  LogMessage msg{
      .timestamp = std::chrono::system_clock::now(), .level = level, .thread_id = std::this_thread::get_id(), .logger_name = m_name, .formatted_message = std::move(buffer)};

  // Lock the mutex and dispatch the message to all sinks
  std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto& sink : m_sinks) {
    if (sink->ShouldFilterByLevel() && msg.level < m_level.load(std::memory_order_relaxed)) {
      continue;  // Skip this sink if it filters and the level is too low
    }
    sink->Log(msg);
  }
}

void Logger::LogThrottledManual(LogLevel level, const char* throttle_key, std::chrono::milliseconds duration, fmt::string_view message) {
    if (!throttle_key) return;

    // Generate a unique key from the provided string literal
    size_t key_hash = std::hash<const char*>{}(throttle_key);

    {
        std::lock_guard<std::mutex> lock(m_throttle_mutex);
        auto now = std::chrono::steady_clock::now();
        auto it = m_throttle_map.find(key_hash);

        if (it != m_throttle_map.end()) {
            if (now - it->second < duration) {
                return;
            }
            it->second = now;
        } else {
            m_throttle_map[key_hash] = now;
        }
    }

    Log(level, message);
}

}  // namespace Logging

SPF_NS_END