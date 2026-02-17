#include "SPF/Data/GameData/Finders/WindowCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
/*
 * Signature for the UpdateInteriorCamera function.
 * Shared by Interior and Window cameras.
 */
const char* UPDATE_INTERIOR_CAMERA_SIG = "48 83 EC 38 F3 0F 10 2D ?? ?? ?? ?? 4C 8B C2 0F 29";

/*
 * Signature for the UpdateInteriorCameraOrientation function.
 * Shared by Interior and Window cameras.
 */
const char* UPDATE_INTERIOR_CAMERA_ORIENTATION_SIG = "40 53 48 81 EC 80 00 00 00 80 B9 3C 01 00 00 00 48 8B D9";
}  // namespace

bool WindowCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Window Camera offsets (Dynamic Search)...");

  uintptr_t pfnUpdateInteriorCamera = Utils::PatternFinder::Find(UPDATE_INTERIOR_CAMERA_SIG);
  if (!pfnUpdateInteriorCamera) {
    logger->Error("CRITICAL: Failed to find signature for UpdateInteriorCamera function.");
    return false;
  }

  uintptr_t pfnUpdateInteriorCameraOrientation = Utils::PatternFinder::Find(UPDATE_INTERIOR_CAMERA_ORIENTATION_SIG);
  if (!pfnUpdateInteriorCameraOrientation) {
    logger->Error("CRITICAL: Failed to find signature for UpdateInteriorCameraOrientation function.");
    return false;
  }

  bool all_found = true;
  const size_t SEARCH_RANGE = 4096;

  /*
   * ANCHOR #1: Initialization Block
   * Extracts: HeadX (490), LimitUp (594), LimitDown (598), HeadZ (498), DefHoriz (58C), DefVert (59C)
   */
  const char* p_init_block = "0F B6 81 ?? ?? ?? ?? F2 0F 10 81 ?? ?? ?? ?? F3 0F 10 91 ?? ?? ?? ?? F3 0F 10 A1 ?? ?? ?? ?? 88 81 ?? ?? ?? ?? 8B 81 ?? ?? ?? ?? F2 0F 11 81 ?? ?? ?? ?? F3 0F 10 81 ?? ?? ?? ?? 89 81 ?? ?? ?? ?? 8B 81 ?? ?? ?? ??";
  uintptr_t addr = Utils::PatternFinder::Find(pfnUpdateInteriorCamera, SEARCH_RANGE, p_init_block);
  if (addr) {
    int32_t headX = Utils::PatternFinder::ReadInt32(addr + 11);
    int32_t limitUp = Utils::PatternFinder::ReadInt32(addr + 19);
    int32_t limitDown = Utils::PatternFinder::ReadInt32(addr + 27);
    int32_t headZ = Utils::PatternFinder::ReadInt32(addr + 39);
    int32_t defHoriz = Utils::PatternFinder::ReadInt32(addr + 55);
    int32_t defVert = Utils::PatternFinder::ReadInt32(addr + 67);

    if (Utils::PatternFinder::IsSaneOffset(headX)) {
      owner.SetWindowHeadOffsetXOffset(headX);
      owner.SetWindowHeadOffsetYOffset(headX + 4);
      logger->Debug("Anchor #1: WindowHeadX=0x{:X}, WindowHeadY=0x{:X}", headX, headX + 4);
    } else { logger->Error("Anchor #1: WindowHeadX INVALID (0x{:X})", headX); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(limitUp)) {
      owner.SetWindowMouseUpLimitOffset(limitUp);
      logger->Debug("Anchor #1: WindowLimitUp=0x{:X}", limitUp);
    } else { logger->Error("Anchor #1: WindowLimitUp INVALID (0x{:X})", limitUp); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(limitDown)) {
      owner.SetWindowMouseDownLimitOffset(limitDown);
      logger->Debug("Anchor #1: WindowLimitDown=0x{:X}", limitDown);
    } else { logger->Error("Anchor #1: WindowLimitDown INVALID (0x{:X})", limitDown); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(headZ)) {
      owner.SetWindowHeadOffsetZOffset(headZ);
      logger->Debug("Anchor #1: WindowHeadZ=0x{:X}", headZ);
    } else { logger->Error("Anchor #1: WindowHeadZ INVALID (0x{:X})", headZ); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(defHoriz)) {
      owner.SetWindowMouseLRDefaultOffset(defHoriz);
      logger->Debug("Anchor #1: WindowDefHoriz=0x{:X}", defHoriz);
    } else { logger->Error("Anchor #1: WindowDefHoriz INVALID (0x{:X})", defHoriz); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(defVert)) {
      owner.SetWindowMouseUDDefaultOffset(defVert);
      logger->Debug("Anchor #1: WindowDefVert=0x{:X}", defVert);
    } else { logger->Error("Anchor #1: WindowDefVert INVALID (0x{:X})", defVert); all_found = false; }
  } else {
    logger->Error("FAILED to find Anchor #1 in UpdateInteriorCamera");
    all_found = false;
  }

  /*
   * ANCHOR #2: Limits Left/Right
   * Extracts: LimitRight (588), LimitLeft (584)
   */
  const char* p_limits_lr = "F3 0F 10 89 ?? ?? ?? ?? 0F 57 CD EB 08 F3 0F 10 89 ?? ?? ?? ??";
  addr = Utils::PatternFinder::Find(pfnUpdateInteriorCamera, SEARCH_RANGE, p_limits_lr);
  if (addr) {
    int32_t limitRight = Utils::PatternFinder::ReadInt32(addr + 4);
    int32_t limitLeft = Utils::PatternFinder::ReadInt32(addr + 17);

    if (Utils::PatternFinder::IsSaneOffset(limitRight)) {
      owner.SetWindowMouseRightLimitOffset(limitRight);
      logger->Debug("Anchor #2: WindowLimitRight=0x{:X}", limitRight);
    } else { logger->Error("Anchor #2: WindowLimitRight INVALID (0x{:X})", limitRight); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(limitLeft)) {
      owner.SetWindowMouseLeftLimitOffset(limitLeft);
      logger->Debug("Anchor #2: WindowLimitLeft=0x{:X}", limitLeft);
    } else { logger->Error("Anchor #2: WindowLimitLeft INVALID (0x{:X})", limitLeft); all_found = false; }
  } else {
    logger->Error("FAILED to find Anchor #2 in UpdateInteriorCamera");
    all_found = false;
  }

  /*
   * ANCHOR #3: Live Yaw/Pitch
   * Extracts: LiveYaw (578), LivePitch (57C)
   */
  const char* p_yaw_pitch = "F3 0F 59 D3 F3 0F 11 83 ?? ?? ?? ?? 0F 2F D1 ?? ?? 0F 28 CC F3 0F 5D CA 48 8D 8B ?? ?? ?? ?? F3 0F 11 8B ?? ?? ?? ??";
  addr = Utils::PatternFinder::Find(pfnUpdateInteriorCameraOrientation, SEARCH_RANGE, p_yaw_pitch);
  if (addr) {
    int32_t liveYaw = Utils::PatternFinder::ReadInt32(addr + 8);
    int32_t livePitch = Utils::PatternFinder::ReadInt32(addr + 35);

    if (Utils::PatternFinder::IsSaneOffset(liveYaw)) {
      owner.SetWindowLiveYawOffset(liveYaw);
      logger->Debug("Anchor #3: WindowLiveYaw=0x{:X}", liveYaw);
    } else { logger->Error("Anchor #3: WindowLiveYaw INVALID (0x{:X})", liveYaw); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(livePitch)) {
      owner.SetWindowLivePitchOffset(livePitch);
      logger->Debug("Anchor #3: WindowLivePitch=0x{:X}", livePitch);
    } else { logger->Error("Anchor #3: WindowLivePitch INVALID (0x{:X})", livePitch); all_found = false; }
  } else {
    logger->Error("FAILED to find Anchor #3 in UpdateInteriorCameraOrientation");
    all_found = false;
  }

  m_isReady = all_found;
  if (all_found) {
    logger->Info("Successfully found all 11 Window Camera offsets dynamically.");
  } else {
    logger->Error("Failed to find one or more Window Camera offsets.");
  }
  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
