#include "SPF/Hooks/BaseHook.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h> // For uintptr_t
#include "MinHook.h"

SPF_NS_BEGIN
namespace Hooks {

BaseHook::BaseHook(std::string name, std::string displayName, std::string signature, std::string ownerName, bool isEnabled)
    : m_name(std::move(name)),
      m_displayName(std::move(displayName)),
      m_signature(std::move(signature)),
      m_ownerName(std::move(ownerName)), // Initialize m_ownerName
      m_isEnabled(isEnabled) {}

void BaseHook::SetEnabled(bool enabled) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    if (m_isEnabled == enabled) {
        return; // No change needed
    }

    if (!IsInstalled()) {
        logger->Warn("Attempted to set enabled state for uninstalled hook '{}'.", m_displayName);
        m_isEnabled = enabled; // Update state even if not installed, will apply on Install()
        return;
    }

    if (enabled) {
        if (MH_EnableHook(reinterpret_cast<LPVOID>(m_hookedAddress)) != MH_OK) {
            logger->Error("Failed to enable hook '{}'.", m_displayName);
            return;
        }
        logger->Info("Hook '{}' enabled.", m_displayName);
    } else {
        if (MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddress)) != MH_OK) {
            logger->Error("Failed to disable hook '{}'.", m_displayName);
            return;
        }
        logger->Info("Hook '{}' disabled.", m_displayName);
    }
    m_isEnabled = enabled;
}

bool BaseHook::Install() {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    if (IsInstalled()) {
        logger->Info("Hook '{}' already installed. Re-enabling if necessary...", m_displayName);
        SetEnabled(m_isEnabled); // Ensure enabled state is applied
        return true;
    }

    logger->Info("Installing hook '{}' for the first time...", m_displayName);

    uintptr_t address = Utils::PatternFinder::Find(m_signature.c_str());
    if (!address) {
        logger->Error("Could not find signature for hook '{}' (signature: '{}'). Hook will not be installed.", m_name, m_signature);
        return false;
    }
    logger->Info("Found signature for hook '{}' at address: {:#x}", m_name, address);

    logger->Info("Found '{}' signature at address: {:#x}", m_displayName, address);

    if (MH_CreateHook(reinterpret_cast<LPVOID>(address), GetDetourFunc(), GetOriginalFuncPtr()) != MH_OK) {
        logger->Error("Failed to create hook for '{}'.", m_displayName);
        return false;
    }

    m_hookedAddress = address; // Save the address only on full success

    // Apply initial enabled state
    if (m_isEnabled) {
        if (MH_EnableHook(reinterpret_cast<LPVOID>(m_hookedAddress)) != MH_OK) {
            logger->Error("Failed to enable hook '{}' after creation.", m_displayName);
            MH_RemoveHook(reinterpret_cast<LPVOID>(m_hookedAddress)); // Clean up created hook
            m_hookedAddress = 0;
            return false;
        }
        logger->Info("Hook '{}' installed and enabled successfully.", m_displayName);
    } else {
        logger->Info("Hook '{}' installed but left disabled (m_isEnabled is false).", m_displayName);
    }
    return true;
}

void BaseHook::Uninstall() {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    if (!IsInstalled()) {
        return; // Not installed, nothing to do
    }

    logger->Info("Attempting to uninstall hook '{}'...", m_displayName);
    auto status = MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddress));

    // Treat "already disabled" as a success case for idempotency.
    if (status != MH_OK && status != MH_ERROR_DISABLED) {
        logger->Error("Failed to disable hook '{}', status: {}", m_displayName, MH_StatusToString(status));
    } else {
        logger->Info("Hook '{}' is now disabled.", m_displayName);
        m_isEnabled = false; // Sync our state flag regardless.
    }
}

void BaseHook::Remove() {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    if (!IsInstalled()) {
        return; // Not installed, nothing to do
    }

    logger->Info("Removing hook '{}'...", m_displayName);

    // Ensure the hook is disabled before removing
    if (m_isEnabled) {
        logger->Info("Hook '{}' is enabled, disabling before removal.", m_displayName);
        if (MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddress)) != MH_OK) {
            logger->Error("Failed to disable hook '{}' before removal. Continuing anyway.", m_displayName);
        }
    }

    if (MH_RemoveHook(reinterpret_cast<LPVOID>(m_hookedAddress)) != MH_OK) {
        logger->Error("Failed to remove hook '{}'.", m_displayName);
    } else {
        logger->Info("Hook '{}' removed successfully.", m_displayName);
    }

    m_hookedAddress = 0;
    m_isEnabled = false;
    if (GetOriginalFuncPtr()) {
        *GetOriginalFuncPtr() = nullptr;
    }
}

}  // namespace Hooks
SPF_NS_END
