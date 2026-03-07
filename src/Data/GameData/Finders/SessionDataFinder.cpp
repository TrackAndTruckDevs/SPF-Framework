#include "SPF/Data/GameData/Finders/SessionDataFinder.hpp"
#include "SPF/Data/GameData/GameObjectSessionService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {

/**
 * @brief Signature to find the active profile handle within the global game object.
 */
const char* ACTIVE_PROFILE_SIG = "48 8b ? ? ? ? ? 4c 8b ? ? ? ? ? e8 ? ? ? ? 48 8b ? ? ? ? ? 4c 8d ? ? ? ? ? 48";

/**
 * @brief Signature to find the global session manager and its status field.
 * Matches logic at the end of FUN_140484c50:
 * 48 8b 0d ?? ?? ?? ??  MOV RCX, qword ptr [SessionMgr]
 * 48 85 c9              TEST RCX, RCX
 * ?? ??                 JZ ...
 * 0f b6 ?? ??           MOVZX EAX, byte ptr [RCX + status_offset]
 */
const char* SESSION_MGR_SIG = "48 8b 0d ? ? ? ? 48 85 c9 ? ? 0f b6";

/**
 * @brief Signature to find where profile properties (display name and type) are accessed.
 * Matches logic in ProcessProfileState:
 * 14047c46a: 48 8d 0d ?? ?? ?? ??  LEA RCX, [s_Set_profile_finished...]
 * 14047c471: 48 8b 52 ??           MOV RDX, [RDX + displayName_offset]
 * e8 ?? ?? ?? ??        CALL ...
 * 48 8b ??              MOV RAX, [REG]
 * 48 63 ?? ??           MOVSXD REG, [REG + profileType_offset]
 */
const char* PROFILE_PROPERTIES_ACCESS_SIG = "48 8d 0d ? ? ? ? 48 8b 52 ? e8 ? ? ? ? 48 8b ? 48 63";

} // namespace

bool SessionDataFinder::TryFindOffsets(GameObjectSessionService& owner) {
    if (m_isReady) return true;

    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
    logger->Info("Starting search for Session and Profile structures...");

    // --- Step 1: Find Active Profile Data ---
    uintptr_t sigAddrProfile = PatternFinder::Find(ACTIVE_PROFILE_SIG);
    if (sigAddrProfile) {
        uintptr_t gamePtr = PatternFinder::GetRipAddress(sigAddrProfile, 3, 7);
        if (gamePtr) {
            owner.SetGamePtrAddr(gamePtr);
            logger->Debug("-> Found GamePtrAddr: 0x{:X}", gamePtr);
        }

        uint32_t profileOff = PatternFinder::ReadInt32(sigAddrProfile + 10);
        if (profileOff > 0 && profileOff < 0xFFFF) {
            owner.SetProfileHandleOffset(profileOff);
            logger->Debug("-> Found ProfileHandleOffset: 0x{:X}", profileOff);
        }
    }

    // --- Step 2: Find Session Manager & Convoy Status Offset ---
    uintptr_t sigAddrSession = PatternFinder::Find(SESSION_MGR_SIG);
    if (sigAddrSession) {
        // 2.1. Extract Session Manager address (MOV RCX, [RIP + disp])
        uintptr_t sessionMgrPtr = PatternFinder::GetRipAddress(sigAddrSession, 3, 7);
        if (sessionMgrPtr) {
            owner.SetSessionMgrPtrAddr(sessionMgrPtr);
            logger->Debug("-> Found SessionMgrPtrAddr: 0x{:X}", sessionMgrPtr);
        }

        // 2.2. Extract Convoy Status Offset
        // Instruction: 0f b6 [reg_modrm] [offset_byte]
        // This instruction starts at sigAddr + 7 (MOV) + 3 (TEST) + 2 (JZ) = offset 12.
        // The offset byte is the 4th byte of the 0F B6 instruction, so sigAddr + 12 + 3 = 15.
        uint8_t statusOff = PatternFinder::ReadInt8(sigAddrSession + 15);
        if (statusOff > 0) {
            owner.SetConvoyStatusOffset(statusOff);
            logger->Debug("-> Found ConvoyStatusOffset: 0x{:X}", statusOff);
        }
    }

    // --- Step 3: Find Profile Properties (DisplayName and Type) ---
    uintptr_t sigAddrProps = PatternFinder::Find(PROFILE_PROPERTIES_ACCESS_SIG);
    if (sigAddrProps) {
        // 3.1. Extract DisplayName Offset (from 48 8b 52 ??)
        // Offset byte is at sigAddr + 7 (LEA) + 3 (MOV start) = 10
        uint8_t nameOff = PatternFinder::ReadInt8(sigAddrProps + 10);
        if (nameOff > 0) {
            owner.SetProfileDisplayNameOffset(nameOff);
            logger->Debug("-> Found ProfileDisplayNameOffset: 0x{:X}", nameOff);
        }

        // 3.2. Extract ProfileType Offset (from 48 63 ?? ??)
        // Sequence: LEA (7) + MOV (4) + CALL (5) + MOV (3) + MOVSXD (4)
        // MOVSXD starts at sigAddr + 19. Offset byte is at sigAddr + 19 + 3 = 22.
        uint8_t typeOff = PatternFinder::ReadInt8(sigAddrProps + 22);
        if (typeOff > 0) {
            owner.SetProfileTypeOffset(typeOff);
            logger->Debug("-> Found ProfileTypeOffset: 0x{:X}", typeOff);
        }
    }

    m_isReady = (owner.GetGamePtrAddr() != 0 && 
                 owner.GetProfileHandleOffset() != 0 &&
                 owner.GetSessionMgrPtrAddr() != 0 &&
                 owner.GetConvoyStatusOffset() != 0 &&
                 owner.GetProfileDisplayNameOffset() != 0 &&
                 owner.GetProfileTypeOffset() != 0);

    return m_isReady;
}

} // namespace Data::GameData::Finders
SPF_NS_END
