#include <SPF/Logging/Sinks/FileSink.hpp>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <ctime> // For std::localtime

SPF_NS_BEGIN

namespace Logging::Sinks {

FileSink::FileSink(const std::filesystem::path& filename, const std::string& name) {
  m_name = name;
  m_file.open(filename, std::ios::out | std::ios::trunc);
  if (!m_file.is_open()) {
    throw std::runtime_error(fmt::format("Failed to open log file: {}", filename.string()));
  }
}

FileSink::~FileSink() {
  if (m_file.is_open()) {
    m_file.close();
  }
}

fmt::string_view FileSink::GetName() const {
    return m_name;
}

void FileSink::Log(const LogMessage& msg) {
  if (!m_file.is_open()) {
    return;
  }

  // Pass the time_point directly to fmt::format for correct millisecond formatting.
  // The fmt/chrono.h header handles the %e specifier.
  std::string formatted_log = fmt::format(fmt::runtime(m_formatter_pattern),
                                          fmt::arg("timestamp", msg.timestamp),
                                          fmt::arg("level", LogLevelToString(msg.level)),
                                          fmt::arg("logger_name", msg.logger_name),
                                          fmt::arg("message", fmt::to_string(msg.formatted_message)));

  m_file << formatted_log << std::endl;
}

}  // namespace Logging::Sinks

SPF_NS_END
