#include "SPF/Data/GameData/Finders/SessionDataFinder.hpp"
#include "SPF/Data/GameData/GameObjectSessionService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {

/**
 * @brief Unique error string used to locate the profile property access logic.
 * Target Function: SelectProfile
 * Verified Address (v1.60): 140fb5cf0
 */
const char* NEW_PROFILE_LOG_STR = "New profile selected: '%s'";

} // namespace

bool SessionDataFinder::TryFindOffsets(GameObjectSessionService& owner) {
    if (m_isReady) return true;

    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
    logger->Info("Starting search for Session and Profile structures...");

    // --- Step 1: Find Active Profile Data (SelectProfile Logic) ---
    /**
     * SEARCH STRATEGY:
     * We use the unique log string "New profile selected: '%s'" to find the SelectProfile function.
     * This function contains both the g_Game pointer and the ProfileHandle offset access.
     * 
     * Target: SelectProfile (v1.59: 140e988fb, v1.60: 140fb5cf0)
     * String: NEW_PROFILE_LOG_STR (14223b560)
     */
    uintptr_t addrLogRef = PatternFinder::FindFunctionByString(NEW_PROFILE_LOG_STR, false);
    uintptr_t addrSelectProfile = addrLogRef ? PatternFinder::GetFunctionStart(addrLogRef) : 0;

    if (addrSelectProfile) {
        logger->Debug("1. SelectProfile found at 0x{:X}", addrSelectProfile);

        // 1.1 & 1.2. Extract g_Game and Profile Handle Offset together
        // Strategy: These two instructions always appear in sequence:
        // MOV reg, [g_Game] -> MOV reg, [reg + ProfileHandleOffset]
        // 
        // Ghidra 1.59 Analysis: 140e9893a (48 8b 05 ...) -> 140e98941 (4c 8b a0 ...)
        // Ghidra 1.60 Analysis: 140fb5d3a (48 8b 05 ...) -> 140fb5d41 (4c 8b a0 ...)
        // 
        // Pattern: 48 8B [05-3D] (g_Game) -> 4C 8B [80-BF] (ProfileHandle)
        uintptr_t addrPair = PatternFinder::Find(addrSelectProfile, 512, "48 8B [05-3D] ?? ?? ?? ?? 4C 8B [80-BF]");
        if (addrPair) {
            // Extract g_Game from the first MOV
            uintptr_t gamePtr = PatternFinder::GetRipAddress(addrPair, 3, 7);
            if (gamePtr) {
                owner.SetGamePtrAddr(gamePtr);
                logger->Debug("1.1 [DATA: Game Pointer] Found at 0x{:X}", gamePtr);
            }

            // Extract ProfileHandleOffset from the second MOV (starts at +7 from addrPair)
            uint32_t profileOff = PatternFinder::ReadInt32(addrPair + 7 + 3);
            if (PatternFinder::IsSaneOffset(profileOff)) {
                owner.SetProfileHandleOffset(profileOff);
                logger->Debug("1.2 [OFFSET: Profile Handle] Found: 0x{:X}", profileOff);
            }
        } else { 
            logger->Error("1. Failed to find GamePtr/ProfileHandle pair in SelectProfile.");
        }
    } else { logger->Error("1. Failed to find SelectProfile via log string."); }

    // --- Step 2: Find Session Manager (Networked) ---
    /**
     * SEARCH STRATEGY:
     * We locate the Session Manager address directly from the MP session handler function.
     * This ensures we get the NETWORKED session manager, not the profile one.
     * 
     * Target: FUN_140514a80 (v1.60 verified address)
     * Context near end of function (140514de5):
     * 140514de5 48 8b 0d ...       MOV  RCX, qword ptr [SessionMgr]
     * 140514df1 0f b6 41 0b        MOVZX EAX, byte ptr [RCX + 0xb]
     */
    uintptr_t addrMpFunc = PatternFinder::FindFunctionByString("[MP] Session started.", true, "48 8B F9");
    if (addrMpFunc) {
        // Find the MOV RCX, [SessionMgr] instruction near the status check
        // Pattern matches: MOV RCX, [SessionMgr] -> TEST RCX, RCX -> JZ -> MOVZX EAX, [RCX + offset]
        uintptr_t addrMgr = PatternFinder::Find(addrMpFunc, 2048, "48 8B 0D [8-16?] 0F B6 [40-BF]");
        if (addrMgr) {
            uintptr_t sessionMgrPtr = PatternFinder::GetRipAddress(addrMgr, 3, 7);
            if (sessionMgrPtr) {
                owner.SetSessionMgrPtrAddr(sessionMgrPtr);
                logger->Debug("2.1 [DATA: Session Manager] Found at 0x{:X}", sessionMgrPtr);
            }

            // Extract Status Offset (0x0B)
            uintptr_t addrMovzx = PatternFinder::Find(addrMgr, 32, "0F B6 [40-BF]");
            if (addrMovzx) {
                uint8_t statusOff = PatternFinder::ReadInt8(addrMovzx + 3);
                if (PatternFinder::IsSaneOffset(statusOff)) {
                    owner.SetConvoyStatusOffset(statusOff);
                    logger->Debug("2.2 [OFFSET: Convoy Status] Found: 0x{:X}", statusOff);
                }
            }
        } else { logger->Error("2. Failed to find Session Manager MOV instruction in MP function."); }
    } else { logger->Error("2. Failed to find target MP session function."); }

    // --- Step 3: Find Profile Properties (DisplayName and Type) ---
    /**
     * SEARCH STRATEGY:
     * 3.1. DisplayName: Use "New profile selected" log as anchor.
     * 3.2. ProfileType: Use SetProfileBasePath function (via path strings) as anchor.
     */
    uintptr_t addrLogStr = PatternFinder::FindFunctionByString(NEW_PROFILE_LOG_STR, false);
    if (addrLogStr) {
        logger->Debug("3. Profile Log reference found at 0x{:X}", addrLogStr);

        // 3.1. Extract DisplayName Offset
        /**
         * SEARCH STRATEGY:
         * We search for the MOV instruction that loads the profile name right before the log call.
         * 
         * Ghidra 1.60 Analysis (inside SelectProfile at 140fb5f38):
         * 140fb5f38 48 8b 52 18    MOV  RDX, qword ptr [RDX + 0x18]
         * 140fb5f3c e8 0f 26 14 ff CALL log_info (E8 ...)
         * 
         * Pattern: 48 8B [40-7F] (ModRM for disp8) ?? (offset) E8 (CALL)
         */
        uintptr_t addrName = PatternFinder::Find(addrLogStr, 64, "48 8B [40-7F] ?? E8");
        if (addrName) {
            uint8_t nameOff = PatternFinder::ReadInt8(addrName + 3);
            if (PatternFinder::IsSaneOffset(nameOff)) {
                owner.SetProfileDisplayNameOffset(nameOff);
                logger->Debug("3.1 [OFFSET: Display Name] Found: 0x{:X}", nameOff);
            }
        } else { logger->Error("3.1 [OFFSET: Display Name] Failed to find MOV instruction."); }

        // 3.2. Extract ProfileType Offset
        /**
         * We locate the SetProfileBasePath function via its unique path strings AND 
         * its instruction prologue using flexible patterns to avoid hardcoding.
         * 
         * Target: SetProfileBasePath (v1.60 verified at 1405042d0)
         * String: "/home/preview_profiles/" (v1.60 at 14212ac00)
         * 
         * Ghidra 1.60 Analysis (SetProfileBasePath start):
         * 1405042e3 48 83 EC ??          SUB  RSP, <any>
         * 1405042e7 48 8B [40-7F] ??     MOV  reg, qword ptr [reg + <any disp8>]
         * 1405042eb 33 F6                XOR  ESI, ESI
         * ...
         * 1405042f3 8B 47 40             MOV  EAX, dword ptr [RDI + 0x40]
         */
        uintptr_t addrPathFunc = PatternFinder::FindFunctionByString("/home/preview_profiles/", true, "48 83 EC ?? 48 8B [40-7F] ?? 33 F6");
        if (addrPathFunc) {           
            // Find the MOV reg, [reg + offset] instruction that reads the type (8B 47 40)
            // We skip the first few bytes to avoid matching the prologue's
            uintptr_t addrTypeMov = PatternFinder::Find(addrPathFunc +15, 128, "8B [0-2?] 41");
            if (addrTypeMov) {
                uint8_t typeOff = PatternFinder::ReadInt8(addrTypeMov + 2);
                if (PatternFinder::IsSaneOffset(typeOff)) {
                    owner.SetProfileTypeOffset(typeOff);
                    logger->Debug("3.2 [OFFSET: Profile Type] Found: 0x{:X}", typeOff);
                }
            } else { logger->Error("3.2 [OFFSET: Profile Type] Failed to find MOV instruction in Path function."); }
        } else { logger->Error("3.2 [OFFSET: Profile Type] Failed to find SetProfileBasePath function."); }
    } else {
        logger->Error("3. Failed to find Profile Properties log reference.");
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
