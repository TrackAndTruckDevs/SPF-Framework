#pragma once

#include "SPF/Namespace.hpp"

#include <cstdint>
#include <filesystem>
#include <minwindef.h>
#include <string>

SPF_NS_BEGIN
namespace System {
class PathManager {
 public:
  // This class cannot be instantiated
  PathManager() = delete;

  /**
   * @brief Initializes the path manager.
   *
   * Determines the framework's base directory based on the DLL's location.
   * Creates necessary subdirectories (Settings, Plugins, Logs).
   * This method must be called once at program startup.
   *
   * @param module The handle of our framework's module (DLL).
   */
  static void Init(HMODULE module);

  /**
   * @brief Returns the base path to the framework's directory.
   * @return A constant reference to a std::filesystem::path object.
   */
  static const std::filesystem::path& GetBasePath();

  /**
   * @brief Returns the absolute path to the framework's DLL file.
   * @return A constant reference to a std::filesystem::path object.
   */
  static const std::filesystem::path& GetFrameworkDllPath();

  /**
   * @brief Returns the full path to the configuration file for the specified module.
   * @param configFileName The name of the configuration file (e.g., "framework_settings.json").
   * @return The full path to the file in the configuration directory.
   */
  static std::filesystem::path GetConfigFilePath(const std::string& configFileName);

  /**
   * @brief Returns the path to the plugins directory.
   * @return A constant reference to a std::filesystem::path object.
   */
  static const std::filesystem::path& GetPluginsPath();

  /**
   * @brief Returns the path to the logs directory.
   * @return A constant reference to a std::filesystem::path object.
   */
  static const std::filesystem::path& GetLogsPath();

  /**
   * @brief Returns the path to the configuration directory.
   * @return A constant reference to a std::filesystem::path object.
   */
  static const std::filesystem::path& GetConfigDir();

  /**
   * @brief Returns the path to the fonts directory.
   * @return A constant reference to a std::filesystem::path object.
   */
  static const std::filesystem::path& GetFontsDir();

  /**
   * @brief Returns the path to the localization directory.
   * @return A constant reference to a std::filesystem::path object.
   */
  static const std::filesystem::path& GetLocalizationDir();

  /**
   * @brief Returns the path to the root directory of the specified plugin.
   * @param pluginName The name of the plugin (must match its directory name).
   * @return The full path to the plugin's directory.
   */
  static std::filesystem::path GetPluginDir(const std::string& pluginName);

  /**
   * @brief Returns the path to the configuration directory of the specified plugin.
   * @param pluginName The name of the plugin.
   * @return The full path to the plugin's configuration directory.
   */
  static std::filesystem::path GetPluginConfigDir(const std::string& pluginName);

  /**
   * @brief Returns the path to the localization directory of the specified plugin.
   * @param pluginName The name of the plugin.
   * @return The full path to the plugin's localization directory.
   */
  static std::filesystem::path GetPluginLocalizationDir(const std::string& pluginName);

  /**
   * @brief Returns the path to the logs directory of the specified plugin.
   * @param pluginName The name of the plugin.
   * @return The full path to the plugin's logs directory.
   */
  static std::filesystem::path GetPluginLogsDir(const std::string& pluginName);

  /**
   * @brief Returns the path to the data directory of the specified plugin.
   * @param pluginName The name of the plugin.
   * @return The full path to the plugin's data directory.
   */
  static std::filesystem::path GetPluginDataDir(const std::string& pluginName);

  /**
   * @brief Returns the SCS Home directory (usually in Documents/Euro Truck Simulator 2).
   * @details This path is retrieved directly from the game's UFS memory structures.
   * @return The absolute path to the game's user directory.
   */
  static std::filesystem::path GetSCSUserDir();

  /**
   * @brief Returns the directory where game mods are located.
   * @return The path to the 'mod' folder inside the SCS User Directory.
   */
  static std::filesystem::path GetSCSModsDir();

  /**
   * @brief Returns the absolute path to the currently active profile directory.
   * @details Navigates the game's internal mount points list to find the '/home/profile' mapping.
   * @return The path to the active profile, or an empty path if not loaded.
   */
  static std::filesystem::path GetCurrentProfilePath();

  /**
   * @brief Returns the human-readable name of the currently active profile.
   * @return The profile display name (e.g., 'SPF_Test').
   */
  static std::string GetCurrentProfileName();

  /**
   * @brief Internal helper to resolve a virtual UFS path to a physical disk path.
   * @param virtualPath The virtual path string (e.g., "/home/profile").
   * @return The physical path string from the game's memory.
   */
  static std::string ResolveVirtualPath(const char* virtualPath);

 private:
  static std::filesystem::path m_basePath;
  static std::filesystem::path m_frameworkDllPath;

  static std::filesystem::path m_pluginsPath;
  static std::filesystem::path m_logsPath;
  static std::filesystem::path m_configPath;
  static std::filesystem::path m_fontsPath;
  static std::filesystem::path m_localizationPath;

  // Profile Cache
  static uintptr_t m_lastProfileAddr;
  static std::string m_cachedProfileName;
  static std::filesystem::path m_cachedProfilePath;
};
}  // namespace System
SPF_NS_END