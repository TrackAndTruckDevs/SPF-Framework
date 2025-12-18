#include "SPF/GameConsole/GameConsole.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>
#include <Psapi.h>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace SPF {

GameConsole& GameConsole::GetInstance() {
  static GameConsole instance;
  return instance;
}

bool GameConsole::Install() {
  if (m_hookedAddress != 0) {
    return true;  // Already installed
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);

  uintptr_t address = Utils::PatternFinder::Find(m_signature.c_str());

  if (address) {
    m_ExecuteGameCommand = reinterpret_cast<ExecuteCommandFn>(address);
    m_hookedAddress = address;  // Save address on success
    logger->Info("Found command execution function at address: {:#x}", address);
    return true;
  } else {
    logger->Error("Could not find command execution function signature. The game might have been updated.");
    return false;
  }
}

void GameConsole::Uninstall() {
  if (m_hookedAddress != 0) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    logger->Info("Service disabled, clearing function pointer.");
    m_ExecuteGameCommand = nullptr;
    m_hookedAddress = 0;
  }
}

void GameConsole::Remove() {
  // For GameConsole, Remove is the same as Uninstall as it's non-destructive.
  Uninstall();
}

void GameConsole::Execute(const std::string& command) {
  if (m_ExecuteGameCommand) {
    const char* pCommand = command.c_str();
    m_ExecuteGameCommand(&pCommand, 0xffffffff); 
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    logger->Warn("Attempted to execute command while service is not active: {}", command);
  }
}
}  // namespace SPF