#include "SPF/Logging/Sinks/FileSink.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Logging/Logger.hpp"

#include "fmt/base.h"

#include <ctime>  // For std::localtime
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/format.h>
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

  // Pass the time_point directly to fmt::format for correct millisecond formatting.
  // The fmt/chrono.h header handles the %e specifier.
  std::string formatted_log =
    fmt::format(fmt::runtime(m_formatter_pattern), fmt::arg("timestamp", msg.timestamp), fmt::arg("level", LogLevelToString(msg.level)), fmt::arg("logger_name", msg.logger_name), fmt::arg("message", fmt::to_string(msg.formatted_message)));

  m_file << formatted_log << std::endl;
}

}  // namespace Logging::Sinks

SPF_NS_END
