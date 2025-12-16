#pragma once

#include <filesystem>
#include <Windows.h>
#include "SPF/Namespace.hpp"

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

 private:
  static std::filesystem::path m_basePath;

  static std::filesystem::path m_pluginsPath;
  static std::filesystem::path m_logsPath;
  static std::filesystem::path m_configPath;
  static std::filesystem::path m_fontsPath;
  static std::filesystem::path m_localizationPath;
};
}  // namespace System
SPF_NS_END