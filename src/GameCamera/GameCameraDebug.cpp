#include "SPF/GameCamera/GameCameraDebug.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace GameCamera {
// Define the function signature for the SetDebugCameraMode function
using SetDebugCameraModeFunc = void(__fastcall*)(uintptr_t, int);

GameCameraDebug::GameCameraDebug() = default;

void GameCameraDebug::SetEnabled(bool enabled) {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCvarObject = gameData.GetCacheableCvarObjectPtr();
  intptr_t valueOffset = gameData.GetCvarValueOffset();

  if (pCvarObject && valueOffset) {
    uintptr_t finalAddress = pCvarObject + valueOffset;
    *(int*)finalAddress = enabled ? 1 : 0;

    // If we are disabling the camera, also reset its state for a clean exit.
    if (!enabled) {
      SetMode(DebugCameraMode::SIMPLE);
      SetHudVisible(false);
    }
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
    logger->Warn("Cannot set enabled state: Dynamic CVar pointers are not ready.");
  }
}

bool GameCameraDebug::GetEnabled(bool* out_isEnabled) const {
  if (!out_isEnabled) {
    return false;
  }

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCvarObject = gameData.GetCacheableCvarObjectPtr();
  intptr_t valueOffset = gameData.GetCvarValueOffset();

  if (pCvarObject && valueOffset) {
    uintptr_t finalAddress = pCvarObject + valueOffset;
    *out_isEnabled = (*(int*)finalAddress != 0);
    return true;
  }

  return false;
}

void GameCameraDebug::SetMode(DebugCameraMode mode) {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  auto pfnSetMode = reinterpret_cast<SetDebugCameraModeFunc>(gameData.GetDebugCameraModeFunc());

  if (context && pfnSetMode) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);  // Resolve the object pointer from context
    if (pDebugCamera) {
      pfnSetMode(pDebugCamera, static_cast<int>(mode));
      m_currentMode = mode;  // Update cached value
    }
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
    logger->Warn("Cannot set mode: DebugCameraContextPtr or SetDebugCameraModeFunc is null.");
  }
}

bool GameCameraDebug::GetCurrentMode(DebugCameraMode* out_mode) const

{
  if (!out_mode) {
    return false;
  }

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();

  uintptr_t context = gameData.GetDebugCameraContextPtr();

  intptr_t offset = gameData.GetDebugCameraModeOffset();

  if (context && offset) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);

    if (pDebugCamera) {
      uint16_t mode_val = *reinterpret_cast<uint16_t*>(pDebugCamera + offset);

      m_currentMode = static_cast<DebugCameraMode>(mode_val);

      *out_mode = m_currentMode;

      return true;
    }
  }

  return false;
}

// --- HUD & UI ---

void GameCameraDebug::SetHudVisible(bool visible) {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  auto pfnSetHudVisibility = reinterpret_cast<void(__fastcall*)(uintptr_t, bool)>(gameData.GetSetHudVisibilityFunc());

  if (context && pfnSetHudVisibility) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      pfnSetHudVisibility(pDebugCamera, visible);
    }
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
    logger->Warn("Cannot set HUD visibility: pointers not ready.");
  }
}

bool GameCameraDebug::GetHudVisible(bool* out_isVisible) const {
  if (!out_isVisible) {
    return false;
  }

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  intptr_t offset = gameData.GetHudVisibleOffset();

  if (context && offset) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      *out_isVisible = (*reinterpret_cast<uint8_t*>(pDebugCamera + offset) != 0);
      return true;
    }
  }
  return false;
}

void GameCameraDebug::SetHudPosition(DebugHudPosition position) {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  auto pfnSetDebugHudPosition = reinterpret_cast<void(__fastcall*)(uintptr_t, uint32_t)>(gameData.GetSetDebugHudPositionFunc());

  if (context && pfnSetDebugHudPosition) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      pfnSetDebugHudPosition(pDebugCamera, static_cast<uint32_t>(position));
    }
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
    logger->Warn("Cannot set HUD position: pointers not ready.");
  }
}

bool GameCameraDebug::GetHudPosition(DebugHudPosition* out_position) const {
  if (!out_position) {
    return false;
  }

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  intptr_t offset = gameData.GetHudPositionOffset();

  if (context && offset) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      *out_position = static_cast<DebugHudPosition>(*reinterpret_cast<uint8_t*>(pDebugCamera + offset));
      return true;
    }
  }
  return false;
}

void GameCameraDebug::SetGameUiVisible(bool visible) {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  intptr_t offset = gameData.GetGameUiVisibleOffset();

  if (context && offset) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      *reinterpret_cast<uint8_t*>(pDebugCamera + offset) = visible ? 1 : 0;
    }
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
    logger->Warn("Cannot set Game UI visibility: pointers not ready.");
  }
}

bool GameCameraDebug::GetGameUiVisible(bool* out_isVisible) const {
  if (!out_isVisible) {
    return false;
  }

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  intptr_t offset = gameData.GetGameUiVisibleOffset();

  if (context && offset) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      *out_isVisible = (*reinterpret_cast<uint8_t*>(pDebugCamera + offset) != 0);
      return true;
    }
  }
  return false;
}
}  // namespace GameCamera
SPF_NS_END
