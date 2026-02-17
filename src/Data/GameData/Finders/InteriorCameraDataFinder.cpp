#include "SPF/Data/GameData/Finders/InteriorCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
/*
 * Signature for the UpdateInteriorCamera function.
 * This function is responsible for updating head position, mouse limits, etc.
 * for interior-like cameras (window, cabin).
 */
const char* UPDATE_INTERIOR_CAMERA_SIG = "48 83 EC 38 F3 0F 10 2D ?? ?? ?? ?? 4C 8B C2 0F 29";

/*
 * Signature for the UpdateInteriorCameraOrientation function.
 * This function is responsible for updating the live yaw/pitch of the camera.
 */
const char* UPDATE_INTERIOR_CAMERA_ORIENTATION_SIG = "40 53 48 81 EC 80 00 00 00 80 B9 3C 01 00 00 00 48 8B D9";
}  // namespace

bool InteriorCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Interior Camera offsets (Dynamic Pattern Search)...");

  // Get base addresses for functions
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
   * ANCHOR #1: Initialization Block (LAB_1407bd222)
   * This block is used to reset/initialize camera parameters.
   *
   * Expected offsets for Game Version 1.58:
   * - Head X:        0x490
   * - Limit Up:      0x594
   * - Limit Down:    0x598
   * - Head Z:        0x498
   * - Def Horiz:     0x58C
   * - Def Vert:      0x59C
   *
   * Signature breakdown:
   * 0F B6 81 ?? ?? ?? ??           | MOVZX EAX, byte ptr [RCX + offset] (530)
   * F2 0F 10 81 ?? ?? ?? ??        | MOVSD XMM0, [RCX + 490] -> Head X
   * F3 0F 10 91 ?? ?? ?? ??        | MOVSS XMM2, [RCX + 594] -> Limit Up
   * F3 0F 10 A1 ?? ?? ?? ??        | MOVSS XMM4, [RCX + 598] -> Limit Down
   * 88 81 ?? ?? ?? ??              | MOV [RCX + 558], AL
   * 8B 81 ?? ?? ?? ??              | MOV EAX, [RCX + 498]    -> Head Z
   * F2 0F 11 81 ?? ?? ?? ??        | MOVSD [RCX + 55C], XMM0
   * F3 0F 10 81 ?? ?? ?? ??        | MOVSS XMM0, [RCX + 58C] -> Def Horiz
   * 89 81 ?? ?? ?? ??              | MOV [RCX + 564], EAX
   * 8B 81 ?? ?? ?? ??              | MOV EAX, [RCX + 59C]    -> Def Vert
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
      owner.SetInteriorSeatXOffset(headX);
      owner.SetInteriorSeatYOffset(headX + 4);
      logger->Debug("Anchor #1: HeadX=0x{:X}, HeadY=0x{:X}", headX, headX + 4);
    } else { logger->Error("Anchor #1: HeadX INVALID (0x{:X})", headX); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(limitUp)) {
      owner.SetInteriorLimitUpOffset(limitUp);
      logger->Debug("Anchor #1: LimitUp=0x{:X}", limitUp);
    } else { logger->Error("Anchor #1: LimitUp INVALID (0x{:X})", limitUp); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(limitDown)) {
      owner.SetInteriorLimitDownOffset(limitDown);
      logger->Debug("Anchor #1: LimitDown=0x{:X}", limitDown);
    } else { logger->Error("Anchor #1: LimitDown INVALID (0x{:X})", limitDown); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(headZ)) {
      owner.SetInteriorSeatZOffset(headZ);
      logger->Debug("Anchor #1: HeadZ=0x{:X}", headZ);
    } else { logger->Error("Anchor #1: HeadZ INVALID (0x{:X})", headZ); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(defHoriz)) {
      owner.SetInteriorMouseLRDefaultOffset(defHoriz);
      logger->Debug("Anchor #1: DefHoriz=0x{:X}", defHoriz);
    } else { logger->Error("Anchor #1: DefHoriz INVALID (0x{:X})", defHoriz); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(defVert)) {
      owner.SetInteriorMouseUDDefaultOffset(defVert);
      logger->Debug("Anchor #1: DefVert=0x{:X}", defVert);
    } else { logger->Error("Anchor #1: DefVert INVALID (0x{:X})", defVert); all_found = false; }
  } else {
    logger->Error("FAILED to find Anchor #1 (Initialization block) in UpdateInteriorCamera");
    all_found = false;
  }

  /*
   * ANCHOR #2: Limits Left/Right Block (LAB_1407bd2c8)
   * This block handles the selection of horizontal limits based on camera state.
   *
   * Expected offsets for Game Version 1.58:
   * - Limit Right:   0x588
   * - Limit Left:    0x584
   *
   * Signature breakdown:
   * F3 0F 10 89 ?? ?? ?? ??        | MOVSS XMM1, [RCX + 588] -> Limit Right
   * 0F 57 CD                       | XORPS XMM1, XMM5
   * EB 08                          | JMP ...
   * F3 0F 10 89 ?? ?? ?? ??        | MOVSS XMM1, [RCX + 584] -> Limit Left
   */
  const char* p_limits_lr = "F3 0F 10 89 ?? ?? ?? ?? 0F 57 CD EB 08 F3 0F 10 89 ?? ?? ?? ??";
  addr = Utils::PatternFinder::Find(pfnUpdateInteriorCamera, SEARCH_RANGE, p_limits_lr);
  if (addr) {
    int32_t limitRight = Utils::PatternFinder::ReadInt32(addr + 4);
    int32_t limitLeft = Utils::PatternFinder::ReadInt32(addr + 17);

    if (Utils::PatternFinder::IsSaneOffset(limitRight)) {
      owner.SetInteriorLimitRightOffset(limitRight);
      logger->Debug("Anchor #2: LimitRight=0x{:X}", limitRight);
    } else { logger->Error("Anchor #2: LimitRight INVALID (0x{:X})", limitRight); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(limitLeft)) {
      owner.SetInteriorLimitLeftOffset(limitLeft);
      logger->Debug("Anchor #2: LimitLeft=0x{:X}", limitLeft);
    } else { logger->Error("Anchor #2: LimitLeft INVALID (0x{:X})", limitLeft); all_found = false; }
  } else {
    logger->Error("FAILED to find Anchor #2 (Limit L/R block) in UpdateInteriorCamera");
    all_found = false;
  }

  /*
   * ANCHOR #3: Live Yaw/Pitch (UpdateInteriorCameraOrientation)
   * This function updates the live orientation of the camera based on input.
   *
   * Expected offsets for Game Version 1.58:
   * - Live Yaw:      0x578
   * - Live Pitch:    0x57C
   *
   * Signature breakdown:
   * F3 0F 59 D3                    | MULSS XMM2, XMM3
   * F3 0F 11 83 ?? ?? ?? ??        | MOVSS [RBX + 578], XMM0 -> Live Yaw
   * 0F 2F D1                       | COMISS XMM2, XMM1
   * ?? ??                          | JC ... (72 07)
   * 0F 28 CC                       | MOVAPS XMM1, XMM4
   * F3 0F 5D CA                    | MINSS XMM1, XMM2
   * 48 8D 8B ?? ?? ?? ??           | LEA RCX, [RBX + 320]
   * F3 0F 11 8B ?? ?? ?? ??        | MOVSS [RBX + 57C], XMM1 -> Live Pitch
   */
  const char* p_yaw_pitch = "F3 0F 59 D3 F3 0F 11 83 ?? ?? ?? ?? 0F 2F D1 ?? ?? 0F 28 CC F3 0F 5D CA 48 8D 8B ?? ?? ?? ?? F3 0F 11 8B ?? ?? ?? ??";
  addr = Utils::PatternFinder::Find(pfnUpdateInteriorCameraOrientation, SEARCH_RANGE, p_yaw_pitch);
  if (addr) {
    int32_t liveYaw = Utils::PatternFinder::ReadInt32(addr + 8);
    int32_t livePitch = Utils::PatternFinder::ReadInt32(addr + 35);

    if (Utils::PatternFinder::IsSaneOffset(liveYaw)) {
      owner.SetInteriorYawOffset(liveYaw);
      logger->Debug("Anchor #3: LiveYaw=0x{:X}", liveYaw);
    } else { logger->Error("Anchor #3: LiveYaw INVALID (0x{:X})", liveYaw); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(livePitch)) {
      owner.SetInteriorPitchOffset(livePitch);
      logger->Debug("Anchor #3: LivePitch=0x{:X}", livePitch);
    } else { logger->Error("Anchor #3: LivePitch INVALID (0x{:X})", livePitch); all_found = false; }
  } else {
    logger->Error("FAILED to find Anchor #3 (Yaw/Pitch chain) in UpdateInteriorCameraOrientation");
    all_found = false;
  }

  m_isReady = all_found;
  if (all_found) {
    logger->Info("Successfully found all 11 Interior Camera offsets dynamically.");
  } else {
    logger->Error("Failed to find one or more Interior Camera offsets. Plugin may not work correctly.");
  }
  return all_found;
}
// FOV offsets are shared and found by FovDataFinder, so we don't search for them here.
}  // namespace Data::GameData::Finders
SPF_NS_END
