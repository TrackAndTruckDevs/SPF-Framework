/**
 * @file ConsoleDataFinder.hpp
 * @brief Dynamic pattern searcher for game console command registries.
 */

#pragma once

#include <cstdint>

namespace SPF::Data::GameData::Finders {

/**
 * @class ConsoleDataFinder
 * @brief Resolves console command registry locations at runtime.
 *
 * Discovers ConsoleCompleteCommand and extracts its three command sources:
 * console command list (g_*), script command list, and input constants array.
 */
class ConsoleDataFinder {
 public:
  /**
   * @brief Attempts to extract the console command registry locations.
   * @return true if all critical patterns were successfully resolved.
   */
  bool TryFind();

  /** @brief Returns the internal name of the finder for logging purposes. */
  const char* GetName() const { return "ConsoleDataFinder"; }

  /** @brief Global holding the head of the console command list. */
  uintptr_t GetConsoleListHeadAddr() const { return m_consoleListHeadAddr; }
  /** @brief Sentinel node terminating the console command list. */
  uintptr_t GetConsoleListSentinelAddr() const { return m_consoleListSentinelAddr; }
  /** @brief Name field offset within a console command node. */
  int32_t GetConsoleNameOffset() const { return m_consoleNameOffset; }
  /** @brief Next pointer offset within a console command node (+0x00). */
  int32_t GetConsoleNextOffset() const { return m_consoleNextOffset; }

  /** @brief Global holding the head of the script command list. */
  uintptr_t GetScriptListHeadAddr() const { return m_scriptListHeadAddr; }
  /** @brief Name field offset within a script command node (inline char*). */
  int32_t GetScriptNameOffset() const { return m_scriptNameOffset; }
  /** @brief Next pointer offset within a script command node. */
  int32_t GetScriptNextOffset() const { return m_scriptNextOffset; }

  /** @brief Global holding the size of the input constants array. */
  uintptr_t GetInputConstSizeAddr() const { return m_inputConstSizeAddr; }
  /** @brief Global holding the data pointer of the input constants array. */
  uintptr_t GetInputConstDataAddr() const { return m_inputConstDataAddr; }
  /** @brief Name field offset within an input constant element. */
  int32_t GetInputConstNameOffset() const { return m_inputConstNameOffset; }
  /** @brief Helper that copies the name out of an input constant element. */
  uintptr_t GetInputConstCopyNameFnAddr() const { return m_inputConstCopyNameFnAddr; }

  /** @brief Global holding the suggestions array data pointer. */
  uintptr_t GetSuggestionsDataAddr() const { return m_suggestionsDataAddr; }
  /** @brief Global holding the suggestions array count. */
  uintptr_t GetSuggestionsCountAddr() const { return m_suggestionsCountAddr; }

  /** @brief Base of the input constants array (assert path; optional). */
  uintptr_t GetInputConstArrayBaseAddr() const { return m_inputConstArrayBaseAddr; }

  /** @brief Cheat dispatcher function (prints subcommand list). */
  uintptr_t GetCheatDispatcherAddr() const { return m_cheatDispatcherAddr; }
  /** @brief Cheat gate function (g_developer check + dispatcher call). */
  uintptr_t GetCheatGateAddr() const { return m_cheatGateAddr; }
  /** @brief Global holding the cheat manager pointer. */
  uintptr_t GetCheatManagerPtrAddr() const { return m_cheatManagerPtrAddr; }
  /** @brief g_developer cvar structure address. */
  uintptr_t GetDeveloperCvarAddr() const { return m_developerCvarAddr; }
  /** @brief GetAndCacheValue cvar reader called by the cheat gate. */
  uintptr_t GetAndCacheValueAddr() const { return m_getAndCacheValueAddr; }
  /** @brief Cheat wrapper tokens array offset (wrapper+tokensOff). */
  int32_t GetCheatTokensOffset() const { return m_cheatTokensOff; }
  /** @brief Cheat wrapper total size (tokensOff + ctrlOff + 8). */
  int64_t GetCheatWrapperSize() const { return m_cheatWrapperSize; }
  /** @brief Shared control block inner pointer field (emptyCtrl()+N). */
  int32_t GetCheatCtrlInnerOffset() const { return m_cheatCtrlInnerOff; }
  /** @brief Cheat manager data pointer field. */
  int32_t GetCheatManagerDataOffset() const { return m_cheatManagerDataOff; }
  /** @brief Cheat manager count field. */
  int32_t GetCheatManagerCountOffset() const { return m_cheatManagerCountOff; }
  /** @brief Cheat entry stride (IMUL imm32). */
  int32_t GetCheatEntryStride() const { return m_cheatEntryStride; }
  /** @brief Temporary prism_string buffer size for token push. */
  int32_t GetCheatTempPrismSize() const { return m_cheatTempPrismSize; }
  /** @brief Decoded-string buffer pointer field. */
  int32_t GetCheatDecodedBufferOffset() const { return m_cheatDecodedBufferOff; }
  /** @brief Decoded-string length field. */
  int32_t GetCheatDecodedLenOffset() const { return m_cheatDecodedLenOff; }
  /** @brief Cheat entry name field. */
  int32_t GetCheatEntryNameOffset() const { return m_cheatEntryNameOff; }

  /** @brief prism array vtable address (array+0x00) from the command token builder. */
  uintptr_t GetArrayVtableAddr() const { return m_arrayVtableAddr; }
  /** @brief prism_string element vtable address from the command token builder. */
  uintptr_t GetElementVtableAddr() const { return m_elementVtableAddr; }
  /** @brief Array data pointer field offset (+0x08). */
  int32_t GetArrayDataOffset() const { return m_arrayDataOff; }
  /** @brief Array size field offset (+0x10). */
  int32_t GetArraySizeOffset() const { return m_arraySizeOff; }
  /** @brief Array capacity field offset (+0x18). */
  int32_t GetArrayCapOffset() const { return m_arrayCapOff; }
  /** @brief Array shared control block field offset (+0x20). */
  int32_t GetArrayCtrlOffset() const { return m_arrayCtrlOff; }
  /** @brief Refcount field offset within the shared control block (+0x58). */
  int32_t GetCtrlRefcountOffset() const { return m_ctrlRefcountOff; }
  /** @brief Refcount increment taken when acquiring the shared ctrl (0x100000000). */
  int64_t GetCtrlRefcountIncrement() const { return m_ctrlRefcountInc; }

 private:
  uintptr_t m_consoleListHeadAddr = 0;
  uintptr_t m_consoleListSentinelAddr = 0;
  int32_t m_consoleNameOffset = 0;
  int32_t m_consoleNextOffset = 0;

  uintptr_t m_scriptListHeadAddr = 0;
  int32_t m_scriptNameOffset = 0;
  int32_t m_scriptNextOffset = 0;

  uintptr_t m_inputConstSizeAddr = 0;
  uintptr_t m_inputConstDataAddr = 0;
  int32_t m_inputConstNameOffset = 0;
  uintptr_t m_inputConstCopyNameFnAddr = 0;

  uintptr_t m_suggestionsDataAddr = 0;
  uintptr_t m_suggestionsCountAddr = 0;

  uintptr_t m_inputConstArrayBaseAddr = 0;

  uintptr_t m_cheatDispatcherAddr = 0;
  uintptr_t m_cheatGateAddr = 0;
  uintptr_t m_cheatManagerPtrAddr = 0;
  uintptr_t m_developerCvarAddr = 0;
  uintptr_t m_getAndCacheValueAddr = 0;
  int32_t m_cheatTokensOff = 0;
  int64_t m_cheatWrapperSize = 0;
  int32_t m_cheatCtrlInnerOff = 0;
  int32_t m_cheatManagerDataOff = 0;
  int32_t m_cheatManagerCountOff = 0;
  int32_t m_cheatEntryStride = 0;
  int32_t m_cheatTempPrismSize = 0;
  int32_t m_cheatDecodedBufferOff = 0;
  int32_t m_cheatDecodedLenOff = 0;
  int32_t m_cheatEntryNameOff = 0;

  uintptr_t m_arrayVtableAddr = 0;
  uintptr_t m_elementVtableAddr = 0;
  int32_t m_arrayDataOff = 0;
  int32_t m_arraySizeOff = 0;
  int32_t m_arrayCapOff = 0;
  int32_t m_arrayCtrlOff = 0;
  int32_t m_ctrlRefcountOff = 0;
  int64_t m_ctrlRefcountInc = 0;

  bool m_isReady = false;
};

}  // namespace SPF::Data::GameData::Finders
