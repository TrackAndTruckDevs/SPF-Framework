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

void GameCameraDebug::SetPosLock(bool locked) {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  auto pfnSetPosLock = reinterpret_cast<void(__fastcall*)(uintptr_t, bool)>(gameData.GetSetPositionLockFunc());

  if (context && pfnSetPosLock) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
      logger->Info("Calling DebugCamera_SetPositionLock(0x{:X}, {})", pDebugCamera, locked);
      pfnSetPosLock(pDebugCamera, locked);
    }
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
    logger->Warn("Cannot set position lock: DebugCameraContextPtr or SetPositionLockFunc is null.");
  }
}

bool GameCameraDebug::GetPosLock(bool* out_locked) const {
  if (!out_locked) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  intptr_t offset = gameData.GetDebugPosLockOffset();
  if (context && offset) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      *out_locked = (*reinterpret_cast<uint8_t*>(pDebugCamera + offset) != 0);
      return true;
    }
  }
  return false;
}

void GameCameraDebug::SetRotLock(bool locked) {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  auto pfnSetRotLock = reinterpret_cast<void(__fastcall*)(uintptr_t, bool)>(gameData.GetSetRotationLockFunc());

  if (context && pfnSetRotLock) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
      logger->Info("Calling DebugCamera_SetRotationLock(0x{:X}, {})", pDebugCamera, locked);
      pfnSetRotLock(pDebugCamera, locked);
    }
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
    logger->Warn("Cannot set rotation lock: DebugCameraContextPtr or SetRotationLockFunc is null.");
  }
}

bool GameCameraDebug::GetRotLock(bool* out_locked) const {
  if (!out_locked) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  intptr_t offset = gameData.GetDebugRotLockOffset();
  if (context && offset) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      *out_locked = (*reinterpret_cast<uint8_t*>(pDebugCamera + offset) != 0);
      return true;
    }
  }
  return false;
}

void GameCameraDebug::SetOrbitMode(bool enabled) {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  auto pfnSetOrbit = reinterpret_cast<void(__fastcall*)(uintptr_t, bool)>(gameData.GetSetOrbitModeFunc());

  if (context && pfnSetOrbit) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
      logger->Info("Calling DebugCamera_SetOrbitMode(0x{:X}, {})", pDebugCamera, enabled);
      pfnSetOrbit(pDebugCamera, enabled);
    }
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
    logger->Warn("Cannot set orbit mode: DebugCameraContextPtr or SetOrbitModeFunc is null.");
  }
}

bool GameCameraDebug::GetOrbitMode(bool* out_enabled) const {
  if (!out_enabled) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  intptr_t offset = gameData.GetDebugOrbitOffset();
  if (context && offset) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      *out_enabled = (*reinterpret_cast<uint8_t*>(pDebugCamera + offset) != 0);
      return true;
    }
  }
  return false;
}

void GameCameraDebug::SetOrbitSpeed(float speed) {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  intptr_t offset = gameData.GetDebugOrbitSpeedOffset();
  if (context && offset) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      *reinterpret_cast<float*>(pDebugCamera + offset) = speed;
    }
  }
}

bool GameCameraDebug::GetOrbitSpeed(float* out_speed) const {
  if (!out_speed) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  intptr_t offset = gameData.GetDebugOrbitSpeedOffset();
  if (context && offset) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      *out_speed = *reinterpret_cast<float*>(pDebugCamera + offset);
      return true;
    }
  }
  return false;
}

uintptr_t GameCameraDebug::GetSelectedObjectPtr() const {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  intptr_t offset = gameData.GetDebugSelectedObjectPtrOffset();
  if (context && offset) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      return *reinterpret_cast<uintptr_t*>(pDebugCamera + offset);
    }
  }
  return 0;
}

void GameCameraDebug::SetSelectedObjectPtr(uintptr_t ptr) {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  auto pfnSetSelected = reinterpret_cast<void(__fastcall*)(uintptr_t, uintptr_t)>(gameData.GetSetSelectedActorFunc());

  if (context && pfnSetSelected) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
      logger->Info("Calling DebugCamera_SetSelectedActor(0x{:X}, 0x{:X})", pDebugCamera, ptr);
      pfnSetSelected(pDebugCamera, ptr);
    }
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
    logger->Warn("Cannot set selected object: DebugCameraContextPtr or SetSelectedActorFunc is null.");
  }
}

uintptr_t GameCameraDebug::GetHoveredObjectPtr() const {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t context = gameData.GetDebugCameraContextPtr();
  intptr_t offset = gameData.GetDebugHoveredObjectPtrOffset();
  if (context && offset) {
    uintptr_t pDebugCamera = *(uintptr_t*)(context + 0);
    if (pDebugCamera) {
      return *reinterpret_cast<uintptr_t*>(pDebugCamera + offset);
    }
  }
  return 0;
}
}  // namespace GameCamera
SPF_NS_END
