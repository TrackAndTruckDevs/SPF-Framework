#include "SPF/Logging/Sinks/ErrorReportSink.hpp"

SPF_NS_BEGIN

namespace Logging::Sinks {

ErrorReportSink::ErrorReportSink() {}

void ErrorReportSink::Log(const LogMessage& msg) {
    // We only care about WARN and ERROR/CRITICAL
    if (msg.level != LogLevel::Warn && msg.level != LogLevel::Error && msg.level != LogLevel::Critical) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Convert memory_buffer to string
    std::string message(msg.formatted_message.data(), msg.formatted_message.size());
    
    LogReportEntry entry;
    entry.loggerName = std::string(msg.logger_name.data(), msg.logger_name.size());
    entry.level = msg.level;
    entry.message = std::move(message);

    // Grouping: increment count if entry already exists
    m_pendingLogs[entry]++;
}

std::vector<LogReportEntry> ErrorReportSink::GetAndClearPendingLogs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<LogReportEntry> result;
    result.reserve(m_pendingLogs.size());

    for (auto& [entry, count] : m_pendingLogs) {
        LogReportEntry finalEntry = entry;
        finalEntry.count = count;
        result.push_back(std::move(finalEntry));
    }

    m_pendingLogs.clear();
    return result;
}

bool ErrorReportSink::HasPendingLogs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_pendingLogs.empty();
}

} // namespace Logging::Sinks

SPF_NS_END
