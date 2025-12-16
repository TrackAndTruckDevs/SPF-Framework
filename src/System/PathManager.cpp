#include "SPF/System/PathManager.hpp"
#include <stdexcept>

SPF_NS_BEGIN
namespace System {
// Definition of static class members
std::filesystem::path PathManager::m_basePath;

std::filesystem::path PathManager::m_pluginsPath;
std::filesystem::path PathManager::m_logsPath;
std::filesystem::path PathManager::m_configPath;
std::filesystem::path PathManager::m_fontsPath;
std::filesystem::path PathManager::m_localizationPath;

void PathManager::Init(HMODULE module) {
  wchar_t path[MAX_PATH];
  if (GetModuleFileNameW(module, path, MAX_PATH) == 0) {
    // In case of error, we cannot continue because we don't know our location.
    // Throwing an exception is a reliable way to stop initialization.
    throw std::runtime_error("PathManager Error: Failed to get module file name.");
  }

  // 1. Determine the base path: the folder where our DLL is located, plus /spf
  m_basePath = std::filesystem::path(path).parent_path() / "spfAssets";

  // 2. Create the base directory.
  // std::filesystem::create_directories does not throw an error if the directory already exists.
  std::filesystem::create_directories(m_basePath);

  // 3. Define and create subdirectories
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
}  // namespace System
SPF_NS_END
