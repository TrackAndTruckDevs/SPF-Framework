#include "SPF/Logging/Sinks/GameLogWindowSink.hpp"

#include "SPF/Logging/Logger.hpp"

#include <cstddef>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace SPF::Logging::Sinks {

GameLogWindowSink& GameLogWindowSink::GetInstance() {
  static GameLogWindowSink instance;
  return instance;
}

void GameLogWindowSink::Push(const std::string& raw_message) {
  // Parse the game's own markers: "HH:MM:SS.mmm : <ERROR> [module] text"
  LogLevel level = LogLevel::Info;
  if (raw_message.find("<ERROR>") != std::string::npos) {
    level = LogLevel::Error;
  } else if (raw_message.find("<WARNING>") != std::string::npos) {
    level = LogLevel::Warn;
  }

  std::string logger_name = "Game";
  size_t modStart = raw_message.find('[');
  size_t modEnd = (modStart != std::string::npos) ? raw_message.find(']', modStart) : std::string::npos;
  if (modStart != std::string::npos && modEnd != std::string::npos && modEnd > modStart + 1) {
    logger_name = raw_message.substr(modStart + 1, modEnd - modStart - 1);
  }

  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_items.size() >= kMaxMessages) {
    m_items.erase(m_items.begin(), m_items.begin() + kPruneBatch);
  }
  m_items.push_back({.level = level, .logger_name = std::move(logger_name), .message = raw_message});
}

std::vector<GameLogWindowSink::DisplayMessage> GameLogWindowSink::GetMessages() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_items;
}

void GameLogWindowSink::Clear() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_items.clear();
}
}  // namespace SPF::Logging::Sinks
