#include "SPF/Data/GameData/Finders/SessionDataFinder.hpp"
#include "SPF/Data/GameData/GameObjectSessionService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {

/**
 * @brief Signature to find the active profile handle (Updated for v1.59.2).
 * 
 * Old logic (v1.58.x):
 * 48 8b 05 ...       MOV RAX, qword ptr [GamePtr]
 * 48 85 c0           TEST RAX, RAX
 * 74 08              JZ ...
 * 48 05 b0 fb ff ff  ADD RAX, -0x450
 * 4c 8b a0 98 0f...  MOV R12, qword ptr [RAX + 0xf98]
 * 
 * New logic (v1.59.2):
 * 140e9893a 48 8b 05 ...  MOV RAX, qword ptr [DAT_1433c5e78]
 * 140e98941 4c 8b a0 98 0f 00 00  MOV R12, qword ptr [RAX + 0xf98]  <-- Offset is here (+10 from start)
 * 140e98948 e8 ...        CALL FUN_14060c500
 */
const char* ACTIVE_PROFILE_SIG = "48 8B 05 ? ? ? ? 4C 8B A0 ? ? ? ? e8 ? ? ? ? 48";

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
 * @brief Signature to find where profile properties (display name and type) are accessed (v1.59+).
 */
const char* PROFILE_PROPERTIES_ACCESS_SIG = "48 8d 0d ? ? ? ? 48 8b 90 ? ? ? ? 48 8b ? ? e8 ? ? ? ? 48 8b ? ? ? 48";

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
        } else {
            logger->Error("-> Step 1: FAILED to resolve RIP address for GamePtr.");
        }

        // 1.2. Extract Profile Handle Offset (from 4C 8B A0 XX XX XX XX)
        // In v1.59.2, the instruction starts at sigAddr + 7.
        // The 32-bit offset is at (sigAddr + 7) + 3 = sigAddr + 10.
        uint32_t profileOff = PatternFinder::ReadInt32(sigAddrProfile + 10);
        if (PatternFinder::IsSaneOffset(profileOff)) {
            owner.SetProfileHandleOffset(profileOff);
            logger->Debug("-> Found ProfileHandleOffset: 0x{:X}", profileOff);
        } else {
            logger->Error("-> Step 1: ProfileHandleOffset INVALID (0x{:X})", profileOff);
        }
    } else {
        logger->Warn("-> Step 1: Could not find ACTIVE_PROFILE signature.");
    }

    // --- Step 2: Find Session Manager & Convoy Status Offset ---
    uintptr_t sigAddrSession = PatternFinder::Find(SESSION_MGR_SIG);
    if (sigAddrSession) {
        // 2.1. Extract Session Manager address
        uintptr_t sessionMgrPtr = PatternFinder::GetRipAddress(sigAddrSession, 3, 7);
        if (sessionMgrPtr) {
            owner.SetSessionMgrPtrAddr(sessionMgrPtr);
            logger->Debug("-> Found SessionMgrPtrAddr: 0x{:X}", sessionMgrPtr);
        } else {
            logger->Error("-> Step 2: FAILED to resolve RIP address for SessionMgrPtr.");
        }

        // 2.2. Extract Convoy Status Offset
        uint8_t statusOff = PatternFinder::ReadInt8(sigAddrSession + 15);
        if (PatternFinder::IsSaneOffset(statusOff)) {
            owner.SetConvoyStatusOffset(statusOff);
            logger->Debug("-> Found ConvoyStatusOffset: 0x{:X}", statusOff);
        } else {
            logger->Error("-> Step 2: ConvoyStatusOffset INVALID (0x{:X})", statusOff);
        }
    } else {
        logger->Warn("-> Step 2: Could not find SESSION_MGR signature.");
    }

    // --- Step 3: Find Profile Properties (DisplayName and Type) ---
    uintptr_t sigAddrProps = PatternFinder::Find(PROFILE_PROPERTIES_ACCESS_SIG);
    if (sigAddrProps) {
        // 3.1. Extract DisplayName Offset (from 48 8b 52 XX)
        // LEA(7) + MOV(7) + MOV_start(3) = 17
        uint8_t nameOff = PatternFinder::ReadInt8(sigAddrProps + 17);
        if (PatternFinder::IsSaneOffset(nameOff)) {
            owner.SetProfileDisplayNameOffset(nameOff);
            logger->Debug("-> Found ProfileDisplayNameOffset: 0x{:X}", nameOff);
        } else {
            logger->Error("-> Step 3: ProfileDisplayNameOffset INVALID (0x{:X})", nameOff);
        }

        // 3.2. Extract ProfileType Offset
        // Based on the new unique signature, we need to find the correct offset for type.
        // For now, let's log the byte at the old position to see what's there.
        uint8_t typeOff = PatternFinder::ReadInt8(sigAddrProps + 27); // Temporary index
        if (PatternFinder::IsSaneOffset(typeOff)) {
            owner.SetProfileTypeOffset(typeOff);
            logger->Debug("-> Found ProfileTypeOffset: 0x{:X}", typeOff);
        } else {
            logger->Error("-> Step 3: ProfileTypeOffset INVALID (0x{:X})", typeOff);
        }
    } else {
        logger->Warn("-> Step 3: Could not find PROFILE_PROPERTIES_ACCESS signature.");
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
