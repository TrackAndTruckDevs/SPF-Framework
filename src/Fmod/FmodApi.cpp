#include "SPF/Fmod/FmodApi.hpp"

#include "SPF/Logging/LoggerFactory.hpp"

#include <cstdint>
#include <cstring>
#include <libloaderapi.h>
#include <minwindef.h>
#include <string>
#include <unordered_map>
#include <winnt.h>

namespace SPF::Fmod {

FmodApi& FmodApi::GetInstance() {
  static FmodApi instance;
  return instance;
}

std::string FmodApi::StripFMODMangling(const char* mangled) {
  if (!mangled || mangled[0] != '?') return {};

  const char* methodStart = mangled + 1;
  const char* methodEnd = std::strchr(methodStart, '@');
  if (!methodEnd) return {};

  const char* classStart = methodEnd + 1;
  const char* classEnd = std::strchr(classStart, '@');
  if (!classEnd) return {};

  if (std::strncmp(classEnd + 1, "Studio@FMOD", 11) != 0) return {};

  return std::string(classStart, classEnd - classStart) + "::" + std::string(methodStart, methodEnd - methodStart);
}

void FmodApi::ParseExportTable(uintptr_t base, const char* dllName, std::unordered_map<std::string, void*>& out) {
  auto dosHdr = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
  if (dosHdr->e_magic != IMAGE_DOS_SIGNATURE) return;

  auto ntHdr = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dosHdr->e_lfanew);
  if (ntHdr->Signature != IMAGE_NT_SIGNATURE) return;

  auto exportDir = ntHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
  if (exportDir.Size == 0) return;

  auto exportTable = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(base + exportDir.VirtualAddress);

  auto names = reinterpret_cast<DWORD*>(base + exportTable->AddressOfNames);
  auto functions = reinterpret_cast<DWORD*>(base + exportTable->AddressOfFunctions);
  auto ordinals = reinterpret_cast<WORD*>(base + exportTable->AddressOfNameOrdinals);

  int discovered = 0;
  for (DWORD i = 0; i < exportTable->NumberOfNames; i++) {
    const char* mangledName = reinterpret_cast<const char*>(base + names[i]);
    void* funcAddr = reinterpret_cast<void*>(base + functions[ordinals[i]]);

    std::string cleanName = StripFMODMangling(mangledName);
    if (!cleanName.empty()) {
      out[cleanName] = funcAddr;
      discovered++;
    }
  }
}

bool FmodApi::Install() {
  if (m_ready) return true;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("FmodApi");

  HMODULE hStudio = GetModuleHandleA("fmodstudio.dll");
  HMODULE hFmod = GetModuleHandleA("fmod.dll");
  if (!hStudio && !hFmod) {
    logger->Error("neither fmodstudio.dll nor fmod.dll loaded");
    return false;
  }

  logger->Info("fmodstudio.dll=0x{:X} fmod.dll=0x{:X}", reinterpret_cast<uintptr_t>(hStudio), reinterpret_cast<uintptr_t>(hFmod));

  if (hStudio) ParseExportTable(reinterpret_cast<uintptr_t>(hStudio), "fmodstudio.dll", m_exports);
  if (hFmod) ParseExportTable(reinterpret_cast<uintptr_t>(hFmod), "fmod.dll", m_exports);

  m_ready = !m_exports.empty();
  logger->Info("Discovered {} FMOD functions, ready={}", m_exports.size(), m_ready);

  for (const auto& [name, addr] : m_exports) {
    logger->Debug("  {} = 0x{:X}", name, reinterpret_cast<uintptr_t>(addr));
  }

  return m_ready;
}

void FmodApi::Uninstall() {
  m_ready = false;
  m_exports.clear();

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("FmodApi");
  logger->Info("shut down");
}

void* FmodApi::Find(const char* demangledName) const {
  auto it = m_exports.find(demangledName);
  return it != m_exports.end() ? it->second : nullptr;
}

bool FmodApi::HasFunction(const char* demangledName) const { return m_exports.count(demangledName) > 0; }

}  // namespace SPF::Fmod
