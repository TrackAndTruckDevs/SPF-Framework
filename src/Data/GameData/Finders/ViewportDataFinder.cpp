#include "SPF/Data/GameData/Finders/ViewportDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
bool ViewportDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Viewport data...");

  bool all_found = true;

  // Find Camera Params Object
  {
    const char* params_sig = "83 78 10 0B 0F 84 ? ? ? ? 48 8B 05 ? ? ? ? 83 B8";
    uintptr_t signature_start = Utils::PatternFinder::Find(params_sig);
    if (!signature_start) {
      logger->Warn("FAILED to find signature for Camera Params Object. Will retry...");
      all_found = false;
    } else {
      uintptr_t mov_instruction_address = signature_start + 10;
      uintptr_t rip = mov_instruction_address + 7;
      int32_t offset = *(int32_t*)(mov_instruction_address + 3);
      uintptr_t pointer_to_global_ptr = rip + offset;
      uintptr_t pCameraParamsObject = *(uintptr_t*)pointer_to_global_ptr;
      owner.SetCameraParamsObjectPtr(pCameraParamsObject);
      logger->Info("Found Camera Params Object at: {:#x}", pCameraParamsObject);
    }
  }

  // Find Viewport Offsets
  {
    const char* signature = "F3 0F 10 83 CC 05 00 00 F3 0F 5C 83 D0 05 00 00 F3 0F 10 8B C8 05 00 00 F3 0F 5C 8B C4 05 00 00 F3 0F 5E C8";
    uintptr_t anchor = Utils::PatternFinder::Find(signature);
    if (!anchor) {
      logger->Warn("FAILED to find signature for Viewport offsets. Will retry...");
      all_found = false;
    } else {
      owner.SetViewportY2Offset(*(int32_t*)(anchor + 4));
      owner.SetViewportY1Offset(*(int32_t*)(anchor + 12));
      owner.SetViewportX2Offset(*(int32_t*)(anchor + 20));
      owner.SetViewportX1Offset(*(int32_t*)(anchor + 28));
      logger->Info("Found Viewport offsets: x1={:#x}, x2={:#x}, y1={:#x}, y2={:#x}",
                   owner.GetViewportX1Offset(),
                   owner.GetViewportX2Offset(),
                   owner.GetViewportY1Offset(),
                   owner.GetViewportY2Offset());
    }
  }

  if (all_found) {
    m_isReady = true;
    logger->Info("Successfully found all viewport data.");
  }
  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
