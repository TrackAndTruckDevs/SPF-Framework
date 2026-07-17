#include "SPF/Utils/FinderLog.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Logging/Logger.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include "fmt/format.h"

#include <cctype>
#include <chrono>
#include <cstdint>
#include <fileapi.h>
#include <libloaderapi.h>
#include <minwindef.h>
#include <string>
#include <string_view>
#include <utility>

SPF_NS_BEGIN
namespace Utils {

// =========================================================================
// FinderLog
// =========================================================================

FinderLog::FinderLog(std::string_view finderName) : m_name(finderName), m_logger(Logging::LoggerFactory::GetInstance().GetLogger(m_name)), m_start(std::chrono::steady_clock::now()) {
  InitModuleInfo();
  m_logger->Info("── Starting {} search... ──", m_name);
}

void FinderLog::InitModuleInfo() {
  HMODULE hMod = GetModuleHandleA(nullptr);
  if (!hMod) return;
  m_moduleBase = reinterpret_cast<uintptr_t>(hMod);

  char path[MAX_PATH];
  DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
  if (len == 0) return;

  std::string_view sv(path, len);
  auto pos = sv.find_last_of("/\\");
  if (pos != std::string_view::npos) sv = sv.substr(pos + 1);

  m_moduleName.reserve(sv.size());
  for (char c : sv) {
    if (c >= 'A' && c <= 'Z')
      m_moduleName.push_back(static_cast<char>(c - 'A' + 'a'));
    else
      m_moduleName.push_back(c);
  }
}

std::string FinderLog::Rel(uintptr_t addr) const {
  if (addr == 0) return "(null)";
  if (m_moduleBase == 0) {
    return fmt::format("0x{:X}", addr);
  }
  uintptr_t offset = addr - m_moduleBase;
  return fmt::format("0x{:X} (\"{}\"+{:X})", addr, m_moduleName, offset);
}

bool FinderLog::ValidateAddr(uintptr_t addr) const { return PatternFinder::IsValidAddress(addr); }

FinderLog::Phase FinderLog::MakePhase(std::string_view name) { return Phase(*this, name); }

bool FinderLog::Finish(bool isReady) {
  if (m_finished) return isReady;
  m_finished = true;

  auto end = std::chrono::steady_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();

  if (isReady) {
    m_logger->Info("── Finished {} search: [✓]READY ({}/{}, {}ms) ──", m_name, m_okSteps, m_totalSteps, ms);
  } else {
    m_logger->Error("── Finished {} search: [x]FAILED ({}/{}, {}ms) ──", m_name, m_okSteps, m_totalSteps, ms);
  }
  return isReady;
}

// =========================================================================
// Phase
// =========================================================================

FinderLog::Phase::Phase(FinderLog& log, std::string_view name) : m_log(&log), m_name(name), m_start(std::chrono::steady_clock::now()) { log.m_logger->Debug("── {} ──", m_name); }

FinderLog::Phase::Phase(Phase&& other) noexcept : m_log(other.m_log), m_name(std::move(other.m_name)), m_start(other.m_start), m_stepNum(other.m_stepNum), m_stepOk(other.m_stepOk), m_hasFailure(other.m_hasFailure) { other.m_log = nullptr; }

FinderLog::Phase& FinderLog::Phase::operator=(Phase&& other) noexcept {
  if (this == &other) return *this;
  if (m_log) {
    EmitTailSummary();
    m_log->m_totalSteps += m_stepNum;
    m_log->m_okSteps += m_stepOk;
  }
  m_log = other.m_log;
  m_name = std::move(other.m_name);
  m_start = other.m_start;
  m_stepNum = other.m_stepNum;
  m_stepOk = other.m_stepOk;
  m_hasFailure = other.m_hasFailure;
  other.m_log = nullptr;
  return *this;
}

FinderLog::Phase::~Phase() {
  if (!m_log) return;
  EmitTailSummary();
  m_log->m_totalSteps += m_stepNum;
  m_log->m_okSteps += m_stepOk;
}

void FinderLog::Phase::EmitTailSummary() {
  auto end = std::chrono::steady_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();
  if (m_hasFailure) {
    m_log->m_logger->Debug("  [x] {}: {}/{} OK ({}ms)", m_name, m_stepOk, m_stepNum, ms);
  } else {
    m_log->m_logger->Debug("  [✓] {}: {}/{} OK ({}ms)", m_name, m_stepOk, m_stepNum, ms);
  }
}

void FinderLog::Phase::EmitStep(int num, uintptr_t addr, bool valid, std::string_view desc, std::string_view tag, bool critical) {
  auto level = critical ? Logging::LogLevel::Error : Logging::LogLevel::Warn;
  auto marker = critical ? "[x]" : "[!]";
  if (!tag.empty()) {
    if (valid) {
      m_log->m_logger->Debug("   {:2d}. [{}] {} → {} ", num, tag, desc, m_log->Rel(addr));
    } else {
      m_log->m_logger->Log(level, "   {:2d}. [{}] {} → NOT FOUND {}", num, tag, desc, marker);
    }
  } else {
    if (valid) {
      m_log->m_logger->Debug("   {:2d}. {} → {} ", num, desc, m_log->Rel(addr));
    } else {
      m_log->m_logger->Log(level, "   {:2d}. {} → NOT FOUND {}", num, desc, marker);
    }
  }
}

bool FinderLog::Phase::Step(uintptr_t addr, std::string_view desc, std::string_view tag) {
  ++m_stepNum;
  bool valid = m_log->ValidateAddr(addr);
  if (valid) {
    ++m_stepOk;
  } else {
    m_hasFailure = true;
  }
  EmitStep(m_stepNum, addr, valid, desc, tag, true);
  return valid;
}

bool FinderLog::Phase::StepOptional(uintptr_t addr, std::string_view desc, std::string_view tag) {
  ++m_stepNum;
  bool valid = m_log->ValidateAddr(addr);
  if (valid) {
    ++m_stepOk;
  }
  EmitStep(m_stepNum, addr, valid, desc, tag, false);
  return valid;
}

bool FinderLog::Phase::StepOffset(int32_t offset, std::string_view desc, std::string_view tag) {
  ++m_stepNum;
  bool valid = PatternFinder::IsSaneOffset(offset);
  if (valid) {
    ++m_stepOk;
  } else {
    m_hasFailure = true;
  }

  if (!tag.empty()) {
    if (valid) {
      m_log->m_logger->Debug("   {:2d}. [{}] {} → 0x{:X} ", m_stepNum, tag, desc, offset);
    } else {
      m_log->m_logger->Error("   {:2d}. [{}] {} → INVALID [x]", m_stepNum, tag, desc);
    }
  } else {
    if (valid) {
      m_log->m_logger->Debug("   {:2d}. {} → 0x{:X} ", m_stepNum, desc, offset);
    } else {
      m_log->m_logger->Error("   {:2d}. {} → INVALID [x]", m_stepNum, desc);
    }
  }
  return valid;
}

bool FinderLog::Phase::StepOffsetOptional(int32_t offset, std::string_view desc, std::string_view tag) {
  ++m_stepNum;
  bool valid = PatternFinder::IsSaneOffset(offset);
  if (valid) {
    ++m_stepOk;
  }

  if (!tag.empty()) {
    if (valid) {
      m_log->m_logger->Debug("   {:2d}. [{}] {} → 0x{:X} ", m_stepNum, tag, desc, offset);
    } else {
      m_log->m_logger->Warn("   {:2d}. [{}] {} → INVALID [!]", m_stepNum, tag, desc);
    }
  } else {
    if (valid) {
      m_log->m_logger->Debug("   {:2d}. {} → 0x{:X} ", m_stepNum, desc, offset);
    } else {
      m_log->m_logger->Warn("   {:2d}. {} → INVALID [!]", m_stepNum, desc);
    }
  }
  return valid;
}

}  // namespace Utils
SPF_NS_END
