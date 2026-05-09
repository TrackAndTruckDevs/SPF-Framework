#include "SPF/GameCamera/GameCameraDebug.hpp"
// #include <Windows.h>
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

// uintptr_t GameCameraDebug::GetPipTextureSrv() const {
//   uintptr_t baseAddr = (uintptr_t)GetModuleHandle(NULL);
//   auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");

//   // 1. Resolve Device (amtrucks.exe + 0x333CEF0)
//   uintptr_t devicePtrAddr = baseAddr + 0x333CEF0;
//   if (IsBadReadPtr((void*)devicePtrAddr, 8)) return 0;
//   uintptr_t device = *(uintptr_t*)devicePtrAddr;
//   if (!device) return 0;

//   // 2. Resolve Image Array (device + 0x34A6C70) and Count (device + 0x34A6C78)
//   uintptr_t arrayPtrAddr = device + 0x34A6C70;
//   uintptr_t countAddr = device + 0x34A6C78;
  
//   if (IsBadReadPtr((void*)arrayPtrAddr, 8)) return 0;
//   uintptr_t arrayBase = *(uintptr_t*)arrayPtrAddr;
//   uintptr_t count = *(uintptr_t*)countAddr;

//   if (!arrayBase || count == 0) return 0;

//   // 3. Check if selected ID is valid
//   if (m_selectedTextureId < 0 || (uintptr_t)m_selectedTextureId >= count) {
//     return 0;
//   }

//   // 4. Calculate Texture Object address: arrayBase + (ID * 0x78)
//   // Each element is 0x78 bytes
//   uintptr_t imageObjAddr = arrayBase + (m_selectedTextureId * 0x78);
//   if (IsBadReadPtr((void*)imageObjAddr, 0x78)) return 0;

//   // 5. Final SRV is at ImageObject + 0x50
//   uintptr_t srv = *(uintptr_t*)(imageObjAddr + 0x50);

//   // DIAGNOSTIC LOGGING
//   static int lastLoggedId = -1;
//   static uintptr_t lastLoggedSrv = 0;
//   if (m_selectedTextureId != lastLoggedId || srv != lastLoggedSrv) {
//     logger->Info("[DIAG] Device: 0x{:X}", device);
//     logger->Info("[DIAG] Array: 0x{:X}, Count: {}", arrayBase, count);
//     logger->Info("[DIAG] Selected ID: {}, ImageObj: 0x{:X}", m_selectedTextureId, imageObjAddr);
//     logger->Info("[DIAG] SRV: 0x{:X}", srv);
    
//     lastLoggedId = m_selectedTextureId;
//     lastLoggedSrv = srv;
//   }

//   return srv;
// }

// uintptr_t GameCameraDebug::GetGpsTextureSrv() const {
//   uintptr_t baseAddr = (uintptr_t)GetModuleHandle(NULL);
//   auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");

//   // 1. RenderManager (amtrucks.exe + 0x333B5B0)
//   uintptr_t renderManagerPtrAddr = baseAddr + 0x333B5B0;
//   if (IsBadReadPtr((void*)renderManagerPtrAddr, 8)) return 0;
//   uintptr_t renderManager = *(uintptr_t*)renderManagerPtrAddr;
//   if (!renderManager) return 0;

//   // 2. GPS Drawable (RenderManager + 0x5E8)
//   uintptr_t gpsDrawablePtrAddr = renderManager + 0x5E8;
//   if (IsBadReadPtr((void*)gpsDrawablePtrAddr, 8)) return 0;
//   uintptr_t gpsDrawable = *(uintptr_t*)gpsDrawablePtrAddr;
//   if (!gpsDrawable) return 0;

//   // 3. Handle (Drawable + 0x14C)
//   if (IsBadReadPtr((void*)(gpsDrawable + 0x14C), 2)) return 0;
//   uint16_t handle = *(uint16_t*)(gpsDrawable + 0x14C);
//   int handleIdx = handle & 0x3FFF;

//   // 4. Resolve to StableID via Texture Manager (base + 0x333CBF8)
//   uintptr_t texManagerPtr = baseAddr + 0x333CBF8;
//   uintptr_t texManager = (texManagerPtr) ? *(uintptr_t*)texManagerPtr : 0;
//   if (!texManager) return 0;

//   uintptr_t texArray = *(uintptr_t*)(texManager + 0x1994770);
//   int texLimit = *(int*)(texManager + 0x1994778);
//   if (handleIdx >= texLimit) return 0;

//   uintptr_t texObj = texArray + (handleIdx * 0x78);
//   unsigned short stableId = *(unsigned short*)(texObj + 0x74);

//   // DIAGNOSTIC LOGGING (Only on change)
//   static uintptr_t lastGpsDrawable = 0;
//   static uint16_t lastHandle = 0;
//   static unsigned short lastStableId = 0;
//   if (gpsDrawable != lastGpsDrawable || handle != lastHandle || stableId != lastStableId) {
//     logger->Info("[DIAG-GPS] Drawable: 0x{:X}, Handle: 0x{:X}, StableID: {}", gpsDrawable, handle, stableId);
//     lastGpsDrawable = gpsDrawable;
//     lastHandle = handle;
//     lastStableId = stableId;
//   }

//   if (stableId <= 0 || stableId >= 0xFFFF) return 0;

//   // 5. Final SRV via DX11 Pool (base + 0x333CEF0)
//   uintptr_t device = *(uintptr_t*)(baseAddr + 0x333CEF0);
//   if (!device) return 0;
  
//   uintptr_t dx11Array = *(uintptr_t*)(device + 0x34A6C70);
//   uintptr_t dx11Count = *(uintptr_t*)(device + 0x34A6C78);
//   if (stableId >= (int)dx11Count) return 0;

//   uintptr_t imageObj = dx11Array + (stableId * 0x78);
//   return *(uintptr_t*)(imageObj + 0x50);
// }

// uintptr_t GameCameraDebug::GetMirrorTextureSrv(int index) const {
//   if (index < 0 || index >= 7) return 0;
//   uintptr_t baseAddr = (uintptr_t)GetModuleHandle(NULL);
//   auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");

//   // 1. RenderManager
//   uintptr_t renderManagerPtrAddr = baseAddr + 0x333B5B0;
//   if (IsBadReadPtr((void*)renderManagerPtrAddr, 8)) return 0;
//   uintptr_t renderManager = *(uintptr_t*)renderManagerPtrAddr;
//   if (!renderManager) return 0;

//   // 2. Mirrors Array (RenderManager + 0x598)
//   uintptr_t mirrorsArrayPtrAddr = renderManager + 0x598;
//   if (IsBadReadPtr((void*)mirrorsArrayPtrAddr, 8)) return 0;
//   uintptr_t mirrorsArray = *(uintptr_t*)mirrorsArrayPtrAddr;
//   if (!mirrorsArray) return 0;

//   uintptr_t mirrorDrawablePtrAddr = mirrorsArray + (index * 8);
//   if (IsBadReadPtr((void*)mirrorDrawablePtrAddr, 8)) return 0;
//   uintptr_t mirrorDrawable = *(uintptr_t*)mirrorDrawablePtrAddr;
//   if (!mirrorDrawable) return 0;

//   // 3. Handle (Drawable + 0x14C)
//   if (IsBadReadPtr((void*)(mirrorDrawable + 0x14C), 2)) return 0;
//   uint16_t handle = *(uint16_t*)(mirrorDrawable + 0x14C);
//   int handleIdx = handle & 0x3FFF;

//   // 4. Resolve via Texture Manager
//   uintptr_t texManagerPtr = baseAddr + 0x333CBF8;
//   uintptr_t texManager = (texManagerPtr) ? *(uintptr_t*)texManagerPtr : 0;
//   if (!texManager) return 0;

//   uintptr_t texArray = *(uintptr_t*)(texManager + 0x1994770);
//   int texLimit = *(int*)(texManager + 0x1994778);
//   if (handleIdx >= texLimit) return 0;

//   uintptr_t texObj = texArray + (handleIdx * 0x78);
//   unsigned short stableId = *(unsigned short*)(texObj + 0x74);

//   // DIAGNOSTIC LOGGING
//   static uintptr_t lastMirrorDrawables[7] = {0};
//   static uint16_t lastMirrorHandles[7] = {0};
//   if (mirrorDrawable != lastMirrorDrawables[index] || handle != lastMirrorHandles[index]) {
//     logger->Info("[DIAG-Mirror {}] Drawable: 0x{:X}, Handle: 0x{:X}, StableID: {}", index, mirrorDrawable, handle, stableId);
//     lastMirrorDrawables[index] = mirrorDrawable;
//     lastMirrorHandles[index] = handle;
//   }

//   // 5. Final SRV
//   uintptr_t device = *(uintptr_t*)(baseAddr + 0x333CEF0);
//   if (!device) return 0;
  
//   uintptr_t dx11Array = *(uintptr_t*)(device + 0x34A6C70);
//   uintptr_t dx11Count = *(uintptr_t*)(device + 0x34A6C78);
//   if (stableId >= (int)dx11Count) return 0;

//   uintptr_t imageObj = dx11Array + (stableId * 0x78);
//   return *(uintptr_t*)(imageObj + 0x50);
// }

// uintptr_t GameCameraDebug::GetTextureSrvByPath(const std::string& path) const {
//   uintptr_t baseAddr = (uintptr_t)GetModuleHandle(NULL);
//   auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");
  
//   // 1. Texture Manager (base + 0x333CBF8)
//   uintptr_t texManagerPtr = baseAddr + 0x333CBF8;
//   uintptr_t texManager = (texManagerPtr) ? *(uintptr_t*)texManagerPtr : 0;
//   if (!texManager) return 0;

//   uintptr_t texArray = *(uintptr_t*)(texManager + 0x1994770);
//   int texLimit = *(int*)(texManager + 0x1994778);

//   for (int i = 0; i < texLimit; i++) {
//     uintptr_t texObj = texArray + (i * 0x78);
//     char* namePtr = *(char**)(texObj + 0x18);
    
//     if (namePtr && !IsBadReadPtr(namePtr, 1)) {
//        std::string currentPath(namePtr);
//        if (currentPath == path || currentPath == ("aliases:" + path)) {
//          unsigned short stableId = *(unsigned short*)(texObj + 0x74);
         
//          static unsigned short lastPathStableIds[32] = {0}; // Cache for a few paths
//          // Simplistic path-to-index mapping for diagnostic logging
//          size_t pathHash = std::hash<std::string>{}(path) % 32;
//          if (stableId != lastPathStableIds[pathHash]) {
//            logger->Info("[DIAG-Path] Found '{}' at Obj: 0x{:X}, StableID: {}", path, texObj, stableId);
//            lastPathStableIds[pathHash] = stableId;
//          }

//          if (stableId <= 0 || stableId >= 0xFFFF) continue;

//          // 2. DX11 Pool
//          uintptr_t device = *(uintptr_t*)(baseAddr + 0x333CEF0);
//          if (!device) return 0;
//          uintptr_t dx11Array = *(uintptr_t*)(device + 0x34A6C70);
//          uintptr_t dx11Count = *(uintptr_t*)(device + 0x34A6C78);
//          if (stableId >= (int)dx11Count) continue;

//          uintptr_t imageObj = dx11Array + (stableId * 0x78);
//          return *(uintptr_t*)(imageObj + 0x50);
//        }
//     }
//   }
//   return 0;
// }

// void GameCameraDebug::SetSelectedTextureId(int id) {
//   m_selectedTextureId = id;
// }

// int GameCameraDebug::GetSelectedTextureId() const {
//   return m_selectedTextureId;
// }

// int GameCameraDebug::GetTextureCount() const {
//   uintptr_t baseAddr = (uintptr_t)GetModuleHandle(NULL);
//   uintptr_t devicePtrAddr = baseAddr + 0x333CEF0;
//   if (IsBadReadPtr((void*)devicePtrAddr, 8)) return 0;
//   uintptr_t device = *(uintptr_t*)devicePtrAddr;
//   if (!device) return 0;

//   return (int)*(uintptr_t*)(device + 0x34A6C78);
// }

// bool GameCameraDebug::IsRenderTargetImage(int id) const {
//   uintptr_t baseAddr = (uintptr_t)GetModuleHandle(NULL);
//   uintptr_t devicePtrAddr = baseAddr + 0x333CEF0;
//   if (IsBadReadPtr((void*)devicePtrAddr, 8)) return false;
//   uintptr_t device = *(uintptr_t*)devicePtrAddr;
//   if (!device) return false;

//   uintptr_t arrayPtrAddr = device + 0x34A6C70;
//   uintptr_t count = *(uintptr_t*)(device + 0x34A6C78);
//   if (id < 0 || (uintptr_t)id >= count) return false;

//   uintptr_t arrayBase = *(uintptr_t*)arrayPtrAddr;
//   if (!arrayBase) return false;

//   uintptr_t imageObjAddr = arrayBase + (id * 0x78);
//   if (IsBadReadPtr((void*)imageObjAddr, 0x78)) return false;

//   uintptr_t srv = *(uintptr_t*)(imageObjAddr + 0x50);
//   uintptr_t rtv = *(uintptr_t*)(imageObjAddr + 0x58);

//   return (srv != 0 && rtv != 0);
// }

// std::vector<TextureResource> GameCameraDebug::GetAvailableTextures() {
//   static std::vector<TextureResource> cachedTextures;
//   static int lastCount = -1;
//   static uint32_t lastScanTime = 0;

//   uintptr_t baseAddr = (uintptr_t)GetModuleHandle(NULL);
//   auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebug");

//   // Scan only once per second or if specifically requested
//   uint32_t currentTime = GetTickCount();
//   if (currentTime - lastScanTime < 1000 && !cachedTextures.empty()) {
//       return cachedTextures;
//   }
//   lastScanTime = currentTime;

//   std::vector<TextureResource> currentTextures;
//   // 1. RenderGraph Manager (Verified: base + 0x2D0F9C0)
//   uintptr_t rgManagerAddr = baseAddr + 0x2D0F9C0;
//   uintptr_t rgArray = *(uintptr_t*)(rgManagerAddr + 0x08);
//   int rgCount = *(int*)(rgManagerAddr + 0x10);

//   // 2. Texture Manager (Verified by user: base + 0x333CBF8)
//   uintptr_t texManagerPtr = baseAddr + 0x333CBF8;
//   uintptr_t texManager = (texManagerPtr) ? *(uintptr_t*)texManagerPtr : 0;
//   uintptr_t texArray = (texManager) ? *(uintptr_t*)(texManager + 0x1994770) : 0;
//   int texLimit = (texManager) ? *(int*)(texManager + 0x1994778) : 0;

//   // 3. DX11 Pool для розмірів (Verified: base + 0x333CEF0)
//   uintptr_t device = *(uintptr_t*)(baseAddr + 0x333CEF0);
//   uintptr_t dx11Array = (device) ? *(uintptr_t*)(device + 0x34A6C70) : 0;
//   uintptr_t dx11Count = (device) ? *(uintptr_t*)(device + 0x34A6C78) : 0;

//   if (!rgArray || !texArray) {
//     return currentTextures;
//   }

//   for (int i = 0; i < rgCount; i++) {
//     uintptr_t rgItem = rgArray + (i * 0x7B0);
//     char* namePtr = *(char**)(rgItem + 0x08);
//     if (!namePtr) continue;
//     std::string nameStr(namePtr);
//     if (nameStr.length() < 5) continue;

//     unsigned short handle = *(unsigned short*)(rgItem + 0x142);
//     int handleIdx = handle & 0x3FFF;
//     if (handleIdx >= texLimit) continue;

//     uintptr_t texObj = texArray + (handleIdx * 0x78);
//     unsigned short stableId = *(unsigned short*)(texObj + 0x74);
//     if (stableId <= 0 || stableId >= 0xFFFF) continue;

//     bool isRT = false;
//     int w = 0, h = 0;
//     if (dx11Array && stableId < (int)dx11Count) {
//         uintptr_t imgObj = dx11Array + (stableId * 0x78);
//         isRT = (*(uintptr_t*)(imgObj + 0x58) != 0);
//         w = *(int*)(imgObj + 0x18);
//         h = *(int*)(imgObj + 0x1C);
//     }

//     if (isRT || nameStr.find("persistent") != std::string::npos) {
//         currentTextures.push_back({ (int)stableId, nameStr, w, h });
//     }
//   }

//   // Only log if the count or names change
//   if (currentTextures.size() != cachedTextures.size()) {
//       logger->Info("[GetAvailableTextures] Scan complete. Found {} target textures.", currentTextures.size());
//       for (const auto& t : currentTextures) {
//           logger->Info(" - ID {}: {} ({}x{})", t.dx11Id, t.name, t.width, t.height);
//       }
//   }

//   cachedTextures = currentTextures;
//   return cachedTextures;
// }
}  // namespace GameCamera
SPF_NS_END
