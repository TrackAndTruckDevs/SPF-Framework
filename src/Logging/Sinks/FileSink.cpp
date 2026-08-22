#include "SPF/Logging/Sinks/FileSink.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Logging/Logger.hpp"

#include "fmt/base.h"

#include "fmt/format.h"
#include "fmt/std.h"    // IWYU pragma: keep

#include <charconv>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <ios>
#include <ostream>
#include <stdexcept>
#include <string>

SPF_NS_BEGIN

namespace Logging::Sinks {

FileSink::FileSink(const std::filesystem::path& filename, const std::string& name, bool append) {
  m_name = name;
  auto mode = std::ios::out | (append ? std::ios::app : std::ios::trunc);
  m_file.open(filename, mode);
  if (!m_file.is_open()) {
    throw std::runtime_error(fmt::format("Failed to open log file: {}", filename.string()));
  }
}

FileSink::~FileSink() {
  if (m_file.is_open()) {
    m_file.close();
  }
}

fmt::string_view FileSink::GetName() const { return m_name; }

void FileSink::Close() {
  if (m_file.is_open()) {
    m_file.close();
  }
}

void FileSink::Log(const LogMessage& msg) {
  if (!m_file.is_open()) {
    return;
  }

  // Try to obtain local wall-clock time using C++20 zoned_time.
  // Fall back to UTC if the timezone database (tzdb) is unavailable on the platform.
  std::chrono::local_time<std::chrono::milliseconds> localTime;
  bool timeConverted = false;

  try {
    // Cast timestamp to milliseconds resolution so that %S formats with 3 fractional digits.
    auto msTime = std::chrono::time_point_cast<std::chrono::milliseconds>(msg.timestamp);
    localTime = std::chrono::zoned_time{std::chrono::current_zone(), msTime}.get_local_time();
    timeConverted = true;
  } catch (...) {
    auto msTime = std::chrono::time_point_cast<std::chrono::milliseconds>(msg.timestamp);
    localTime = std::chrono::local_time<std::chrono::milliseconds>{msTime.time_since_epoch()};
  }

  // Convert Windows thread ID hex string (e.g. "0xfcc") into a decimal number.
  std::string tidStr = fmt::to_string(msg.thread_id);
  unsigned int threadIdDec = 0;
  if (tidStr.starts_with("0x") || tidStr.starts_with("0X")) {
    std::from_chars(tidStr.data() + 2, tidStr.data() + tidStr.size(), threadIdDec, 16);
  } else {
    std::from_chars(tidStr.data(), tidStr.data() + tidStr.size(), threadIdDec, 10);
  }

  std::string formatted_log;
  if (timeConverted) {
    formatted_log = fmt::format(fmt::runtime(m_formatter_pattern),
                                fmt::arg("timestamp", localTime),
                                fmt::arg("thread", threadIdDec),
                                fmt::arg("level", LogLevelToString(msg.level)),
                                fmt::arg("source_type", msg.is_plugin ? "P" : "F"),
                                fmt::arg("logger_name", msg.logger_name),
                                fmt::arg("message", fmt::to_string(msg.formatted_message)));
  } else {
    // Append 'Z' to mark UTC time if timezone conversion fails
    formatted_log = fmt::format(fmt::runtime(m_formatter_pattern),
                                fmt::arg("timestamp", fmt::format("{:%Y-%m-%d %H:%M:%S}Z", localTime)),
                                fmt::arg("thread", threadIdDec),
                                fmt::arg("level", LogLevelToString(msg.level)),
                                fmt::arg("source_type", msg.is_plugin ? "P" : "F"),
                                fmt::arg("logger_name", msg.logger_name),
                                fmt::arg("message", fmt::to_string(msg.formatted_message)));
  }

  m_file << formatted_log << std::endl;
}

}  // namespace Logging::Sinks

SPF_NS_END
