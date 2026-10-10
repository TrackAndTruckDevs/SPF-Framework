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

  bool m_isReady = false;
};

}  // namespace SPF::Data::GameData::Finders
