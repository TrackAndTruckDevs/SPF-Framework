#include "SPF/System/PathManager.hpp"
#include "SPF/Data/GameData/GameObjectFileSystemService.hpp"
#include "SPF/Data/GameData/GameObjectSessionService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include <stdexcept>

SPF_NS_BEGIN
namespace System {
// Definition of static class members
std::filesystem::path PathManager::m_basePath;
std::filesystem::path PathManager::m_frameworkDllPath;

std::filesystem::path PathManager::m_pluginsPath;
std::filesystem::path PathManager::m_logsPath;
std::filesystem::path PathManager::m_configPath;
std::filesystem::path PathManager::m_fontsPath;
std::filesystem::path PathManager::m_localizationPath;

uintptr_t PathManager::m_lastProfileAddr = 0;
std::string PathManager::m_cachedProfileName;
std::filesystem::path PathManager::m_cachedProfilePath;

namespace {
    // Simple sanity check for pointers in user-mode x64 space
    bool IsValidPointer(uintptr_t p) {
        return p >= 0x10000 && p < 0x00007FFFFFFFFFFF;
    }

    /**
     * Converts a display string to its HEX representation.
     * Used by the game to generate profile folder names.
     */
    std::string StringToHex(const std::string& input) {
        static const char hex_digits[] = "0123456789abcdef";
        std::string output;
        output.reserve(input.length() * 2);
        for (unsigned char c : input) {
            output.push_back(hex_digits[c >> 4]);
            output.push_back(hex_digits[c & 15]);
        }
        return output;
    }
}

void PathManager::Init(HMODULE module) {
  wchar_t path[MAX_PATH];
  if (GetModuleFileNameW(module, path, MAX_PATH) == 0) {
    throw std::runtime_error("PathManager Error: Failed to get module file name.");
  }

  m_frameworkDllPath = std::filesystem::path(path);
  m_basePath = m_frameworkDllPath.parent_path() / "spfAssets";
  std::filesystem::create_directories(m_basePath);

  m_configPath = m_basePath / "config";
  std::filesystem::create_directories(m_configPath);

  m_pluginsPath = std::filesystem::path(path).parent_path() / "spfPlugins";
  std::filesystem::create_directories(m_pluginsPath);

  m_logsPath = m_basePath / "logs";
  std::filesystem::create_directories(m_logsPath);

  m_localizationPath = m_basePath / "localization";
  std::filesystem::create_directories(m_localizationPath);
}

const std::filesystem::path& PathManager::GetBasePath() { return m_basePath; }
const std::filesystem::path& PathManager::GetFrameworkDllPath() { return m_frameworkDllPath; }
const std::filesystem::path& PathManager::GetPluginsPath() { return m_pluginsPath; }
const std::filesystem::path& PathManager::GetLogsPath() { return m_logsPath; }
const std::filesystem::path& PathManager::GetConfigDir() { return m_configPath; }
const std::filesystem::path& PathManager::GetFontsDir() { return m_fontsPath; }
const std::filesystem::path& PathManager::GetLocalizationDir() { return m_localizationPath; }
std::filesystem::path PathManager::GetConfigFilePath(const std::string& configFileName) { return m_configPath / configFileName; }
std::filesystem::path PathManager::GetPluginDir(const std::string& pluginName) { return m_pluginsPath / pluginName; }
std::filesystem::path PathManager::GetPluginConfigDir(const std::string& pluginName) { return GetPluginDir(pluginName) / "config"; }
std::filesystem::path PathManager::GetPluginLocalizationDir(const std::string& pluginName) { return GetPluginDir(pluginName) / "localization"; }
std::filesystem::path PathManager::GetPluginLogsDir(const std::string& pluginName) { return GetPluginDir(pluginName) / "logs"; }
std::filesystem::path PathManager::GetPluginDataDir(const std::string& pluginName) { return GetPluginDir(pluginName) / "data"; }

std::filesystem::path PathManager::GetSCSUserDir() {
  /**
   * GHIDRA ANALYSIS:
   * The '/home' directory is the root of the game's user data (Documents/ATS).
   * It is typically mounted in UFS Manager index 1.
   */
  static std::filesystem::path cachedPath;
  if (!cachedPath.empty()) return cachedPath;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PathManager");
  logger->Info("Resolving SCS User Directory (/home)...");

  std::string homePath = ResolveVirtualPath("/home");
  if (!homePath.empty()) {
    // Normalizing separators to be consistent
    cachedPath = std::filesystem::path(homePath).make_preferred();
    logger->Info("Successfully resolved /home to: '{}'", cachedPath.string());
    return cachedPath;
  }

  return "";
}

std::filesystem::path PathManager::GetSCSModsDir() {
  /**
   * SCS game mods are located in the 'mod' subdirectory of the User Directory.
   */
  std::filesystem::path home = GetSCSUserDir();
  if (home.empty()) return "";
  return (home / "mod").make_preferred();
}

std::filesystem::path PathManager::GetCurrentProfilePath() {
  /**
   * GHIDRA ANALYSIS:
   * 1. g_Game object is found via a reliable signature in 'SelectProfile'.
   * 2. Active Profile handle is at offset +0xDF8 from g_Game.
   * 3. Profile object (profile_t) is the first pointer in that handle (+0x0).
   * 4. Display Name is at +0x18 from profile_t.
   * 5. Profile Type is at +0x38 from profile_t.
   * 6. Folder name on disk is the HEX representation of the Display Name.
   */
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PathManager");
  auto& ufs = Data::GameData::GameObjectFileSystemService::GetInstance();
  
  if (!ufs.AreAllFindersReady()) return "";

  // 0. Check if active profile offsets were found during research
  if (ufs.GetGamePtrAddr() == 0 || ufs.GetProfileHandleOffset() == 0) {
      return "";
  }

  // 1. Access the global Game object
  uintptr_t* ppGame = reinterpret_cast<uintptr_t*>(ufs.GetGamePtrAddr());
  if (!ppGame || !IsValidPointer(*ppGame)) return "";
  uintptr_t g_Game = *ppGame;

  // 2. Get the Profile Handle from the Game object
  uintptr_t profileHandleAddr = g_Game + ufs.GetProfileHandleOffset();
  uintptr_t profileHandle = *reinterpret_cast<uintptr_t*>(profileHandleAddr);
  if (!IsValidPointer(profileHandle)) return "";

  // 3. The first 8 bytes of the handle point to the profile_t object
  uintptr_t profile = *reinterpret_cast<uintptr_t*>(profileHandle);
  if (!IsValidPointer(profile)) return "";

  // SMART CACHE: Re-calculate only if the profile pointer has changed
  if (profile == m_lastProfileAddr && !m_cachedProfilePath.empty()) {
      return m_cachedProfilePath;
  }

  logger->Debug("Profile change detected. Old: {:#x}, New: {:#x}", m_lastProfileAddr, profile);

  auto& sessionService = Data::GameData::GameObjectSessionService::GetInstance();

  // 4. Extract Display Name and Profile Type using dynamic offsets
  const char** ppDisplayName = reinterpret_cast<const char**>(profile + sessionService.GetProfileDisplayNameOffset());
  if (!ppDisplayName || !IsValidPointer((uintptr_t)*ppDisplayName)) return "";
  
  const char* displayName = *ppDisplayName;
  int profileType = *reinterpret_cast<int*>(profile + sessionService.GetProfileTypeOffset());

  // 5. Determine subdirectory based on profile type (Logic from SetProfileBasePath)
  std::string profilesSubDir = "profiles";
  if (profileType == 4) profilesSubDir = "steam_profiles";
  else if (profileType == 0) profilesSubDir = "preview_profiles";
  else if (profileType == 1) profilesSubDir = "academy_profiles";
  else if (profileType == 2) profilesSubDir = "demo_profiles";

  // 6. Resolve the root physical path of /home
  std::string homePhysical = ResolveVirtualPath("/home");
  if (homePhysical.empty()) return "";

  // 7. Convert Display Name to HEX and combine: Home + SubDir + HexName
  std::string hexName = StringToHex(displayName);
  m_cachedProfilePath = (std::filesystem::path(homePhysical) / profilesSubDir / hexName).make_preferred();
  m_cachedProfileName = displayName;
  m_lastProfileAddr = profile;

  logger->Info("Active profile identified: '{}' (Type: {}), Path: '{}'", displayName, profileType, m_cachedProfilePath.string());

  return m_cachedProfilePath;
}

std::string PathManager::GetCurrentProfileName() {
  if (m_cachedProfileName.empty()) {
      GetCurrentProfilePath();
  }
  return m_cachedProfileName;
}

std::string PathManager::ResolveVirtualPath(const char* virtualPath) {
  /**
   * PRISM UFS OBSERVED STRUCTURE:
   * - Manager 0: Core archives. Virtual paths are empty.
   * - Manager 1: Logical mount points (/home, /steam).
   * - Manager 3: DLC archives.
   * - Manager 4: System/Internal nodes.
   */
  static bool hasLoggedUfsTree = false;
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PathManager");
  auto& ufs = Data::GameData::GameObjectFileSystemService::GetInstance();
  
  if (!ufs.AreAllFindersReady()) return "";

  uintptr_t managersArrayPtr = *reinterpret_cast<uintptr_t*>(ufs.GetDevicesArrayAddr());
  uint32_t managersCount = *reinterpret_cast<uint32_t*>(ufs.GetManagersCountAddr());

  if (!managersArrayPtr || managersCount == 0) return "";
  uintptr_t* pManagers = reinterpret_cast<uintptr_t*>(managersArrayPtr);

  for (uint32_t m = 0; m < managersCount; ++m) {
    uintptr_t ufsManager = pManagers[m];
    if (!IsValidPointer(ufsManager)) continue;

    uintptr_t anchorAddr = ufsManager + ufs.GetMountListHeadOffset();
    uintptr_t currentNode = *reinterpret_cast<uintptr_t*>(anchorAddr);
    if (!currentNode || currentNode == anchorAddr) continue;

    for (int i = 0; i < 512; ++i) {
        if (currentNode == anchorAddr || !IsValidPointer(currentNode)) break;

        uintptr_t vpathObjAddr = currentNode + ufs.GetNodeVPathOffset();
        const char* vpathStr = *reinterpret_cast<const char**>(vpathObjAddr + ufs.GetStringBufferOffset());
        
        if (vpathStr && IsValidPointer((uintptr_t)vpathStr)) {
            uintptr_t deviceAddr = *reinterpret_cast<uintptr_t*>(currentNode + ufs.GetNodeDeviceOffset());
            const char* physPath = nullptr;
            if (deviceAddr && IsValidPointer(deviceAddr)) {
                physPath = *reinterpret_cast<const char**>(deviceAddr + ufs.GetPhysicalDevicePathOffset());
            }

            // Log the UFS tree only once for diagnostics
            if (!hasLoggedUfsTree) {
                logger->Trace("[UFS Manager {}] Node {:#x}: Virtual='{}', Physical='{}'", m, currentNode, vpathStr, physPath ? physPath : "NULL");
            }

            if (strcmp(vpathStr, virtualPath) == 0) {
                if (physPath && IsValidPointer((uintptr_t)physPath)) {
                    hasLoggedUfsTree = true; // Mark as logged after first successful match
                    return std::filesystem::path(physPath).make_preferred().string();
                }
            }
        }
        currentNode = *reinterpret_cast<uintptr_t*>(currentNode);
    }
  }

  hasLoggedUfsTree = true; // Also mark as logged if we scanned all but found nothing
  return "";
}

}  // namespace System
SPF_NS_END
