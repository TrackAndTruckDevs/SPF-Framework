/**                                                                                               
 * @file WorldDataFinder.cpp                                                                          
 * @brief Implementation of dynamic pattern searching using EXACT Ghidra signatures.
 */ 

#include "SPF/Data/GameData/Finders/GameWorldDataFinder.hpp"
#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {

bool WorldDataFinder::TryFindOffsets(GameWorldService& owner) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
    logger->Info("Searching for GameWorld (Environment) data using provided Ghidra signatures...");

    // 1. Find the entry point of the UpdateTimeAdvance function.
    // This function is responsible for scrubbing/simulating time during events 
    // like resting, using the ferry, or changing time in Photo Mode. 
    // The signature matches the start of the function analyzed in Ghidra (UpdateTimeAdvance).
    // 48 8b c4        MOV        RAX,RSP
    // 55              PUSH       RBP
    // 48 81 ec        SUB        RSP,0x80
    // 80 00 00 00
    // 44 8b 91        MOV        R10D,dword ptr [RCX + 0x210]
    // 10 02 00 00
    // 48 8b e9        MOV        RBP,RCX    
    const char* UPDATE_TIME_ADVANCE_SIG = "48 8B ?? ?? 48 81 EC ?? ?? ?? ?? 44 8B 91 ?? ?? ?? ?? 48 8B";
    uintptr_t pfnUpdateTimeAdvance = Utils::PatternFinder::Find(UPDATE_TIME_ADVANCE_SIG);

    if (!pfnUpdateTimeAdvance) {
        logger->Error("CRITICAL: Failed to find UpdateTimeAdvance function start.");
        return false;
    }
    logger->Debug("UpdateTimeAdvance found at 0x{:X}", pfnUpdateTimeAdvance);

    bool all_found = true;
    const size_t SEARCH_RANGE = 4096; 

    /*
     * ANCHOR #1: Global Environment Base Pointer (DAT_...)
     * This signature finds the instruction that loads the global pointer to the
     * environment system.
     * Ghidra: 14147d300 48 8b 1d 39 b3 ee 01  MOV RBX, qword ptr [DAT_143368640]
     * 48 8b 1d        MOV        RBX,qword ptr [DAT_143368640]
     * 39 b3 ee 01
     * ba 80 00        MOV        EDX,0x80
     * 00 00
     */
    const char* p_global_base = "48 8B ?? ?? ?? ?? ?? BA";
    uintptr_t addr = Utils::PatternFinder::Find(pfnUpdateTimeAdvance, SEARCH_RANGE, p_global_base);
    if (addr) {
        // This is a RIP-relative instruction (MOV RBX, [RIP + displacement]).
        // The instruction is 7 bytes long, and the 32-bit displacement is at offset 3.
        uintptr_t basePtr = Utils::PatternFinder::GetRipAddress(addr, 3, 7);
        if (basePtr) {
            owner.SetEnvironmentBasePtr(basePtr);
            logger->Debug("Anchor #1: Global Environment Base Pointer found at 0x{:X}", basePtr);
        } else {
            logger->Error("Anchor #1: Failed to resolve Global Base Pointer.");
            all_found = false;
        }
    } else {
        logger->Error("Anchor #1: FAILED to find Global Base signature.");
        all_found = false;
    }

    /*
     * ANCHOR #2: Environment Object Offset
     * This signature finds the instruction that dereferences the global pointer 
     * to get the actual environment object.
     * Ghidra: 14147d313 48 8b 9b e8 07 00 00  MOV RBX, qword ptr [RBX + 0x7e8]
     *
     * 48 8b 9b        MOV        RBX,qword ptr [RBX + 0x7e8]
     * e8 07 00 00
     * 49 8b c9        MOV        RCX,R9
     * e8 0e 48        CALL       GetChildUiElementById
     * e8 fe
     */
    const char* p_obj_offset = "48 8B ?? ?? ?? ?? ?? 49 8B ?? E8";
    addr = Utils::PatternFinder::Find(pfnUpdateTimeAdvance, SEARCH_RANGE, p_obj_offset);
    if (addr) {
        // The 32-bit offset is at index 3 of the instruction "48 8B 9B [offset]".
        int32_t envOffset = Utils::PatternFinder::ReadInt32(addr + 3);
        if (Utils::PatternFinder::IsSaneOffset(envOffset)) {
            owner.SetEnvObjectOffset(envOffset);
            logger->Debug("Anchor #2: Environment Object Offset found: 0x{:X}", envOffset);
        } else {
            logger->Error("Anchor #2: Environment Object Offset (0x{:X}) is insane.", envOffset);
            all_found = false;
        }
    } else {
        logger->Error("Anchor #2: FAILED to find Object Offset signature.");
        all_found = false;
    }

    /*
     * ANCHOR #3: Time Offset (0x3E50) & Environment Update Function Call
     * This signature finds the code block responsible for writing the new time 
     * value into the environment object and then calling the function to update 
     * the visual state (skybox, lighting).
     *
     * Ghidra:
     *   14147d3ca 44 89 b3 50 3e 00 00  MOV dword ptr [RBX + 0x3e50], R14D
     *   14147d3d1 c7 83 54 3e 00 00 00 00 00 00  MOV dword ptr [RBX + 0x3e54], 0x0
     *   14147d3db e8 90 f3 fc fe        CALL UpdateEnvironmentState
     */
    const char* p_time_update = "44 89 ?? ?? ?? ?? ?? C7 83 ?? ?? ?? ?? ?? ?? ?? ?? E8";
    addr = Utils::PatternFinder::Find(pfnUpdateTimeAdvance, SEARCH_RANGE, p_time_update);
    if (addr) {
        // 1. Extract Time Offset (from MOV dword ptr [RBX + offset], R14D)
        // The 32-bit offset is at index 3 of the instruction "44 89 B3 [offset]".
        int32_t timeOffset = Utils::PatternFinder::ReadInt32(addr + 3);
        
        // 2. Extract Update Function Address (from CALL UpdateEnvironmentState)
        // The CALL instruction (E8) is at the end of this pattern match.
        // The byte pattern's "E8" is at 'addr + 17'.
        // The CALL instruction is 5 bytes long: E8 [4-byte relative displacement].
        uintptr_t pfnUpdateEnv = Utils::PatternFinder::GetRipAddress(addr + 17, 1, 5);

        if (Utils::PatternFinder::IsSaneOffset(timeOffset)) {
            owner.SetTimeOffset(timeOffset);
            logger->Debug("Anchor #3: Time Offset found: 0x{:X}", timeOffset);
        } else {
            logger->Error("Anchor #3: Time Offset (0x{:X}) is insane.", timeOffset);
            all_found = false;
        }

        if (pfnUpdateEnv) {
            owner.SetUpdateFnAddr(pfnUpdateEnv);
            logger->Debug("Anchor #3: Update function found at 0x{:X}", pfnUpdateEnv);
        } else {
            logger->Error("Anchor #3: Failed to resolve Update function address.");
            all_found = false;
        }
    } else {
        logger->Error("Anchor #3: FAILED to find Time/Update signature.");
        all_found = false;
    }

    m_isReady = all_found;
    return all_found;
}

} // namespace Data::GameData::Finders
SPF_NS_END
