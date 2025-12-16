#include "SPF/Logging/Sinks/LoggerWindowSink.hpp"
#include <fmt/format.h>

SPF_NS_BEGIN
namespace Logging::Sinks {

LoggerWindowSink::LoggerWindowSink() {
    m_name = "ui_sink";
}

void LoggerWindowSink::Log(const LogMessage& msg) {
  std::lock_guard<std::mutex> lock(m_mutex);
  // Create a copy of the data needed for display
  m_items.push_back({.level = msg.level, .logger_name = std::string(msg.logger_name.data(), msg.logger_name.size()), .message = fmt::to_string(msg.formatted_message)});
}

fmt::string_view LoggerWindowSink::GetName() const {
    return m_name;
}

std::vector<LoggerWindowSink::DisplayMessage> LoggerWindowSink::GetMessages() const {  std::lock_guard<std::mutex> lock(m_mutex);
  return m_items;
}

void LoggerWindowSink::Clear() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_items.clear();
}
}  // namespace Logging::Sinks
SPF_NS_END