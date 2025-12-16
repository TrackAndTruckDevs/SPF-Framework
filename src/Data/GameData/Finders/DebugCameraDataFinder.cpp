#include "SPF/Data/GameData/Finders/DebugCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {

// Signature for the SetDebugCameraMode function.
const char* SET_DEBUG_MODE_SIG = "48 89 5C ? ? 57 48 83 EC 50 8B FA 48 8B D9 39 91";

// Signature for the SetHUDVisibility function.
const char* SET_HUD_VISIBILITY_SIG = "48 89 5C 24 10 55 56 57 48 83 EC 40 48 8D B1 30 05 00 00 0F B6 EA";

// Signature for the SetDebugHudPosition function.
const char* SET_DEBUG_HUD_POSITION_SIG = "48 89 5C 24 08 57 48 83 EC 20 48 8B D9 8B FA 48 8B 89 30 05 00 00 48 85 C9";

// --- Signatures for dynamic CVar value finding ---

/*
 * Signature to find the base pointer of the cacheable CVar object.
 * HOW-TO-FIND:
 * 1. Find the GetAndCacheValue function.
 * 2. Find its cross-references (XREFs).
 * 3. Land in the 'SetupAndRenderView' function.
 * 4. The signature targets the unique sequence of instructions before the call to GetAndCacheValue:
 *    - 48 8D 0D...  (lea rcx, [rip+...])  <- This loads the pointer we need.
 *    - 4C 89 88...  (mov [rax+...], r9)
 *    - E8...        (call GetAndCacheValue)
 *    - 85 C0        (test eax, eax)
 */
const char* CACHEABLE_CVAR_PTR_SIG = "48 8D 0D ? ? ? ? 4C 89 88 ? ? ? ? E8 ? ? ? ? 85 C0";

/*
 * Signature to find the dynamic offset of the value within the CVar object.
 * This signature is searched for *inside* the GetAndCacheValue function.
 * HOW-TO-FIND:
 * 1. Go to the GetAndCacheValue function.
 * 2. Find the instruction that writes the cached value: mov dword ptr [rdi+0x118], eax
 * 3. The signature targets this instruction and its neighbors for uniqueness:
 *    - 48 8B 5C 24 30  (mov rbx, [rsp+30])
 *    - 89 87...        (mov [rdi+offset], eax) <- This contains the offset we need.
 *    - C6 87...        (mov [rdi+offset-2], 1)
 */
const char* CVAR_VALUE_OFFSET_SIG = "48 8B 5C 24 30 89 87 ? ? ? ? C6 87";

// Signature for the GetAndCacheValue function itself, to provide a search range.
const char* GET_AND_CACHE_VALUE_SIG = "40 57 48 83 EC 20 48 83 B9";

// Signature for the Clean UI toggle logic.
const char* CLEAN_UI_TOGGLE_SIG = "80 BE ? ? ? ? 00 0F 94 C0 88 86 ? ? ? ? F2 0F 10 05";

// Signature for the HUD Visibility read logic.
const char* HUD_VISIBLE_READ_SIG = "38 9E ? ? ? ? 48 8B CE 0F 94 C2";

// Signature for the HUD Position read logic.
const char* HUD_POSITION_READ_SIG = "8B 96 ? ? ? ? 48 8B CE FF C2 83 E2 03";
}  // namespace

bool DebugCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  if (m_isReady) {
    return true;
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Debug Camera pointers...");

  // --- Stage 1: Find standalone pointers and functions ---

  // --- Stage 1: Find CVar object pointer and value offset dynamically ---
  bool cvar_ptr_found = (owner.GetCacheableCvarObjectPtr() != 0);
  if (!cvar_ptr_found) {
    uintptr_t sig_addr = Utils::PatternFinder::Find(CACHEABLE_CVAR_PTR_SIG);
    if (sig_addr) {
      // The signature is on the LEA instruction. We need to calculate the RIP-relative address.
      uintptr_t rip = sig_addr + 7;  // LEA is 7 bytes long (48 8D 0D XX XX XX XX)
      int32_t offset = *(int32_t*)(sig_addr + 3);
      uintptr_t base_ptr = rip + offset;
      owner.SetCacheableCvarObjectPtr(base_ptr);
      logger->Info("Found Cacheable CVar Object pointer dynamically: {:#x}", base_ptr);
      cvar_ptr_found = true;
    } else {
      logger->Warn("Signature for Cacheable CVar Object not found. Will retry...");
    }
  }

  bool cvar_offset_found = (owner.GetCvarValueOffset() != 0);
  if (!cvar_offset_found) {
    uintptr_t get_and_cache_func_addr = Utils::PatternFinder::Find(GET_AND_CACHE_VALUE_SIG);
    if (get_and_cache_func_addr) {
      // Search for the offset signature within the GetAndCacheValue function (e.g., within 256 bytes)
      uintptr_t sig_addr = Utils::PatternFinder::Find(get_and_cache_func_addr, 256, CVAR_VALUE_OFFSET_SIG);
      if (sig_addr) {
        // The signature is on `48 8B 5C 24 30 89 87 ...`
        // The offset is 7 bytes in (after `48 8B 5C 24 30 89 87`)
        int32_t offset = *(int32_t*)(sig_addr + 7);
        owner.SetCvarValueOffset(offset);
        logger->Info("Found CVar Value Offset dynamically: {:#x}", offset);
        cvar_offset_found = true;
      } else {
        logger->Warn("Signature for CVar Value Offset not found within GetAndCacheValue. Will retry...");
      }
    } else {
      logger->Warn("Signature for GetAndCacheValue function not found. Will retry...");
    }
  }

  bool set_mode_func_found = (owner.GetDebugCameraModeFunc() != nullptr);
  if (!set_mode_func_found) {
    uintptr_t pfnSetDebugCameraMode = Utils::PatternFinder::Find(SET_DEBUG_MODE_SIG);
    if (pfnSetDebugCameraMode) {
      owner.SetDebugCameraModeFunc(reinterpret_cast<void*>(pfnSetDebugCameraMode));
      logger->Info("Found SetDebugCameraMode function at: {:#x}", pfnSetDebugCameraMode);
      set_mode_func_found = true;
    } else {
      logger->Warn("Signature for SetDebugCameraMode function not found. Will retry...");
    }
  }

  bool set_hud_visibility_func_found = (owner.GetSetHudVisibilityFunc() != nullptr);
  if (!set_hud_visibility_func_found) {
    uintptr_t pfnSetHudVisibility = Utils::PatternFinder::Find(SET_HUD_VISIBILITY_SIG);
    if (pfnSetHudVisibility) {
      owner.SetSetHudVisibilityFunc(reinterpret_cast<void*>(pfnSetHudVisibility));
      logger->Info("Found SetHudVisibility function at: {:#x}", pfnSetHudVisibility);
      set_hud_visibility_func_found = true;
    } else {
      logger->Warn("Signature for SetHudVisibility function not found. Will retry...");
    }
  }

  bool set_debug_hud_pos_func_found = (owner.GetSetDebugHudPositionFunc() != nullptr);
  if (!set_debug_hud_pos_func_found) {
    uintptr_t pfnSetDebugHudPosition = Utils::PatternFinder::Find(SET_DEBUG_HUD_POSITION_SIG);
    if (pfnSetDebugHudPosition) {
      owner.SetSetDebugHudPositionFunc(reinterpret_cast<void*>(pfnSetDebugHudPosition));
      logger->Info("Found SetDebugHudPosition function at: {:#x}", pfnSetDebugHudPosition);
      set_debug_hud_pos_func_found = true;
    } else {
      logger->Warn("Signature for SetDebugHudPosition function not found. Will retry...");
    }
  }

  // --- Stage 2: Find the pDebugCamera context pointer via pointer chain ---
  bool p_debug_camera_context_found = (owner.GetDebugCameraContextPtr() != 0);
  if (!p_debug_camera_context_found) {
    uintptr_t pStandardManagerAddr = owner.GetStandardManagerPtrAddr();
    if (pStandardManagerAddr) {
      uintptr_t pStandardManager = *(uintptr_t*)pStandardManagerAddr;
      if (pStandardManager && !IsBadReadPtr((void*)(pStandardManager + 0x38), sizeof(uintptr_t))) {
        uintptr_t pDebugCameraContext = *(uintptr_t*)(pStandardManager + 0x38);
        if (pDebugCameraContext) {
          owner.SetDebugCameraContextPtr(pDebugCameraContext);
          logger->Info("Found pDebugCameraContext successfully at: {:#x}", pDebugCameraContext);
          p_debug_camera_context_found = true;
        }
      }
    }
  }
  if (!p_debug_camera_context_found) {
    logger->Warn("Could not resolve pDebugCameraContext pointer. Will retry...");
  }

  // --- Stage 3: Find offsets within DebugCamera_HandleInput using relative search ---
  bool internal_offsets_found = false;
  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pfnDebugCamera_HandleInput = cameraHooks.GetDebugCameraHandleInputFunc();

  if (!pfnDebugCamera_HandleInput) {
    logger->Warn("DebugCamera_HandleInput function not ready. Cannot find internal offsets. Will retry...");
  } else {
    constexpr size_t SEARCH_RANGE_MEDIUM = 2048;
    constexpr size_t SEARCH_RANGE_LARGE = 12288;
    constexpr size_t SEARCH_RANGE_TIGHT = 128;

    bool ui_offset_found = (owner.GetGameUiVisibleOffset() != 0);
    uintptr_t ui_offset_addr = 0;
    if (!ui_offset_found) {
      ui_offset_addr = Utils::PatternFinder::Find(pfnDebugCamera_HandleInput, SEARCH_RANGE_MEDIUM, CLEAN_UI_TOGGLE_SIG);
      if (ui_offset_addr) {
        owner.SetGameUiVisibleOffset(*(int32_t*)(ui_offset_addr + 2));
        logger->Info("Found Game UI Visible offset: {:#x}", owner.GetGameUiVisibleOffset());
        ui_offset_found = true;
      } else {
        logger->Warn("Signature for Game UI Visible offset not found. Will retry...");
      }
    }

    bool hud_vis_offset_found = (owner.GetHudVisibleOffset() != 0);
    uintptr_t hud_vis_addr = 0;
    if (!hud_vis_offset_found) {
      hud_vis_addr = Utils::PatternFinder::Find(pfnDebugCamera_HandleInput, SEARCH_RANGE_LARGE, HUD_VISIBLE_READ_SIG);
      if (hud_vis_addr) {
        owner.SetHudVisibleOffset(*(int32_t*)(hud_vis_addr + 2));
        logger->Info("Found HUD Visible offset: {:#x}", owner.GetHudVisibleOffset());
        hud_vis_offset_found = true;
      } else {
        logger->Warn("Signature for HUD Visible offset not found. Will retry...");
      }
    }

    bool hud_pos_offset_found = (owner.GetHudPositionOffset() != 0);
    if (!hud_pos_offset_found && hud_vis_addr) {  // This search is relative to the HUD visible find
      uintptr_t hud_pos_addr = Utils::PatternFinder::Find(hud_vis_addr, SEARCH_RANGE_TIGHT, HUD_POSITION_READ_SIG);
      if (hud_pos_addr) {
        owner.SetHudPositionOffset(*(int32_t*)(hud_pos_addr + 2));
        logger->Info("Found HUD Position offset: {:#x}", owner.GetHudPositionOffset());
        hud_pos_offset_found = true;
      } else {
        logger->Warn("Signature for HUD Position offset not found. Will retry...");
      }
    }

    bool mode_offset_found = (owner.GetDebugCameraModeOffset() != 0);
    if (!mode_offset_found) {
      const char* MODE_OFFSET_SIG = "8B 86 ? ? ? ? 4C 8D 35";
      uintptr_t mode_offset_addr = Utils::PatternFinder::Find(pfnDebugCamera_HandleInput, SEARCH_RANGE_LARGE, MODE_OFFSET_SIG);
      if (mode_offset_addr) {
        intptr_t offset = *(int32_t*)(mode_offset_addr + 2);
        owner.SetDebugCameraModeOffset(offset);
        logger->Info("Found Debug Camera Mode offset dynamically: {:#x}", offset);
        mode_offset_found = true;
      } else {
        logger->Warn("Signature for Debug Camera Mode offset not found. Will retry...");
      }
    }

    internal_offsets_found = ui_offset_found && hud_vis_offset_found && hud_pos_offset_found && mode_offset_found;
  }

  // --- Finalize ---
  m_isReady = cvar_ptr_found && cvar_offset_found && set_mode_func_found && set_hud_visibility_func_found && set_debug_hud_pos_func_found && p_debug_camera_context_found &&
              internal_offsets_found;

  if (m_isReady) {
    logger->Info("Successfully found all debug camera data.");
  }

  return m_isReady;
}
}  // namespace Data::GameData::Finders
SPF_NS_END
