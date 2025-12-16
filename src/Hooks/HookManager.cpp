#include "SPF/Hooks/HookManager.hpp"

// --- Framework Includes ---
#include "SPF/Logging/LoggerFactory.hpp"

// --- System Hook Includes (Critical) ---
#include "SPF/Hooks/D3D11Hook.hpp"
#include "SPF/Hooks/D3D12Hook.hpp"
#include "SPF/Hooks/OpenGLHook.hpp"
#include "SPF/Hooks/DInput8Hook.hpp"
#include "SPF/Hooks/User32Hook.hpp"
#include "SPF/Hooks/XInputHook.hpp"

// --- Feature Hook Includes (Configurable) ---
#include "SPF/Hooks/GameLogHook.hpp"
#include "SPF/GameConsole/GameConsole.hpp"

#include <algorithm>  // For std::find

SPF_NS_BEGIN
namespace Hooks {
HookManager& HookManager::GetInstance() {
  static HookManager instance;
  return instance;
}

void HookManager::RegisterFeatureHook(IHook* hook) {
  if (hook && std::find(m_featureHooks.begin(), m_featureHooks.end(), hook) == m_featureHooks.end()) {
    m_featureHooks.push_back(hook);
  }
}

void HookManager::UnregisterFeatureHook(IHook* hook) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("HookManager");
  logger->Info("--> [1.1/3] Unregistering hook '''{}'''...", hook->GetDisplayName());
  m_featureHooks.erase(std::remove(m_featureHooks.begin(), m_featureHooks.end(), hook), m_featureHooks.end());
  logger->Info("--> [1.2/3] Hook '''{}''' erased from feature hooks vector.", hook->GetDisplayName());
}

const std::vector<IHook*>& HookManager::GetFeatureHooks() const { return m_featureHooks; }

void HookManager::RequestEnableHook(const std::string& hookName, const std::string& pluginName) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("HookManager");
  logger->Info("Plugin '''{}''' requests hook '''{}'''.", pluginName, hookName);
  m_hookRequests[hookName].insert(pluginName);
}

void HookManager::ReleaseEnableRequest(const std::string& hookName, const std::string& pluginName) {
  if (m_hookRequests.count(hookName)) {
    m_hookRequests[hookName].erase(pluginName);
    if (m_hookRequests[hookName].empty()) {
      m_hookRequests.erase(hookName);
    }
  }
}

bool HookManager::IsHookRequired(const std::string& hookName) const { return m_hookRequests.count(hookName) && !m_hookRequests.at(hookName).empty(); }

bool HookManager::InstallGraphicsHooks(Rendering::RenderAPI api) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("HookManager");
  logger->Info("Installing graphics hooks for the detected API...");

  switch (api) {
    case Rendering::RenderAPI::D3D11:
      if (!D3D11Hook::Install()) {
        logger->Critical("Failed to install D3D11 hooks. Framework will not function.");
        return false;
      }
      break;
    case Rendering::RenderAPI::D3D12:
      if (!D3D12Hook::Install()) {
        logger->Critical("Failed to install D3D12 hooks. Framework will not function.");
        return false;
      }
      break;
    case Rendering::RenderAPI::OpenGL:
      if (!OpenGLHook::Install()) {
        logger->Critical("Failed to install OpenGL hooks. Framework will not function.");
        return false;
      }
      break;
    case Rendering::RenderAPI::Unknown:
    default:
      logger->Critical("No supported graphics API was detected. Cannot install graphics hooks.");
      return false;
  }

  logger->Info("Graphics hooks installed successfully.");
  return true;
}

bool HookManager::InstallSystemAndFeatureHooks() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("HookManager");
  logger->Info("Installing system and feature hooks...");
  bool systemHooksSuccess = true;

  // --- 1. Install non-graphics SYSTEM hooks ---
  logger->Info("Installing non-graphics system hooks...");
  if (!DInput8Hook::Install()) {
    logger->Critical("Failed to install DInput8 hooks.");
    systemHooksSuccess = false;
  }
  if (!User32Hook::Install()) {
    logger->Critical("Failed to install User32 hooks.");
    systemHooksSuccess = false;
  }
  if (!XInputHook::Install()) {
    logger->Warn("Failed to install XInput hooks. This may be normal if the game doesn't use it.");
  }
  logger->Info("Non-graphics system hooks installation finished.");

  // --- 2. Install registered FEATURE hooks ---
  logger->Info("Installing feature hooks...");
  if (m_featureHooks.empty()) {
    logger->Info("No feature hooks registered.");
  } else {
    for (auto* hook : m_featureHooks) {
      bool requestedByPlugin = IsHookRequired(hook->GetName());
      if (hook->IsEnabled() || requestedByPlugin) {
        if (hook->IsInstalled()) {
          logger->Debug("Skipping already installed hook: {}", hook->GetDisplayName());
        } else {
          logger->Info("Installing hook: {} (Enabled: {}, Required: {})...", hook->GetDisplayName(), hook->IsEnabled(), requestedByPlugin);
          if (!hook->Install()) {
            logger->Error("Failed to install feature hook: {}", hook->GetDisplayName());
          }
        }
      } else {
        logger->Info("Skipping disabled feature hook: {}", hook->GetDisplayName());
      }
    }
  }
  logger->Info("Feature hooks installation finished.");

  if (systemHooksSuccess) {
    logger->Info("All system and feature hooks processed successfully.");
  } else {
    logger->Error("Critical system hook installation failed. Framework may be unstable.");
  }

  return systemHooksSuccess;
}

void HookManager::InstallFeatureHook(IHook* hook) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("HookManager");
    if (!hook) return;

    bool requestedByPlugin = IsHookRequired(hook->GetName());
    if (hook->IsEnabled() || requestedByPlugin) {
        if (hook->IsInstalled()) {
            logger->Debug("Skipping already installed dynamic hook: {}", hook->GetDisplayName());
        } else {
            logger->Info("Dynamically installing hook: {} (Enabled: {}, Required: {})...", hook->GetDisplayName(), hook->IsEnabled(), requestedByPlugin);
            if (!hook->Install()) {
                logger->Error("Failed to install feature hook: {}", hook->GetDisplayName());
            }
        }
    } else {
        logger->Info("Skipping installation of disabled dynamic hook: {}", hook->GetDisplayName());
    }
}

IHook* HookManager::GetHook(const std::string& name) {
    for (auto* hook : m_featureHooks) {
        if (hook->GetName() == name) {
            return hook;
        }
    }
    return nullptr;
}

void HookManager::ReconcileHookState(IHook* hook, bool configuredEnabledState) {
    if (!hook) return;

    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("HookManager");
    // A hook should be active if it's explicitly enabled in config OR if any plugin requires it.
    bool shouldBeActive = configuredEnabledState || IsHookRequired(hook->GetName());

    if (shouldBeActive) {
        if (!hook->IsInstalled()) {
            logger->Info("Reconciling hook '{}': Installing...", hook->GetDisplayName());
            hook->Install();
        }
        // Ensure the hook is enabled if it should be active
        if (!hook->IsEnabled()) { // Check runtime state
            logger->Info("Reconciling hook '{}': Enabling...", hook->GetDisplayName());
            hook->SetEnabled(true); // Set runtime state
        }
    } else { // shouldBeActive is false
        if (hook->IsInstalled()) {
            logger->Info("Reconciling hook '{}': Uninstalling...", hook->GetDisplayName());
            hook->Uninstall();
        }
        // Ensure the hook is disabled if it should not be active
        if (hook->IsEnabled()) { // Check runtime state
            logger->Info("Reconciling hook '{}': Disabling...", hook->GetDisplayName());
            hook->SetEnabled(false); // Set runtime state
        }
    }
}

void HookManager::UninstallAllHooks() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("HookManager");
  logger->Info("Disabling all hooks for reload...");

  // Uninstall feature hooks first, in reverse order of registration
  for (auto it = m_featureHooks.rbegin(); it != m_featureHooks.rend(); ++it) {
    (*it)->Uninstall();
  }

  // Then uninstall system hooks, in reverse order of their installation
  XInputHook::Uninstall();
  User32Hook::Uninstall();
  DInput8Hook::Uninstall();
  D3D12Hook::Uninstall();
  D3D11Hook::Uninstall();
  OpenGLHook::Uninstall();

  logger->Info("All hooks disabled.");
}

void HookManager::RemoveAllHooks() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("HookManager");
  logger->Info("Completely removing all hooks for shutdown...");

  // Remove feature hooks first, in reverse order of registration
  for (auto it = m_featureHooks.rbegin(); it != m_featureHooks.rend(); ++it) {
    (*it)->Remove();
  }

  // Then remove system hooks, in reverse order of their installation
  XInputHook::Remove();
  User32Hook::Remove();
  DInput8Hook::Remove();
  D3D12Hook::Remove();
  D3D11Hook::Remove();
  OpenGLHook::Remove();

  logger->Info("All hooks removed.");
}
}  // namespace Hooks
SPF_NS_END
